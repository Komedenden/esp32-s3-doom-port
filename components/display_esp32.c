#include "display_esp32.h"
#include "hal_config.h"
#include "doomgeneric.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "i_video.h"

static const char *TAG = "DOOM_ESP32_DISPLAY";

static esp_lcd_panel_handle_t panel_handle = NULL;

// 分割描画時のチャンク
#define CHUNK_LINES 40

// ダブルバッファリング描画用バッファ
static uint16_t *line_buffers[2] = { NULL, NULL };
static int current_buf_idx = 0;

// フレームの送信完了を待つセマフォ
static SemaphoreHandle_t display_sem; 

// 液晶への送信が1回完了するたびに呼ばれるコールバック関数
static bool notify_flush_ready(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_priority = pdFALSE;
    xSemaphoreGiveFromISR(display_sem, &high_task_priority);
    return high_task_priority == pdTRUE;
}


// ハードウェア初期化
void InitDisplay() {
    // PSRAMから描画バッファを確保
    DG_ScreenBuffer = (uint32_t *)heap_caps_malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(uint32_t), MALLOC_CAP_SPIRAM);

    if (!DG_ScreenBuffer) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM buffers!");
        return;
    }

    // 液晶初期化
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = 60 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // 画面の回転を補正
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, true);

    // 描画用セマフォの初期化
    display_sem = xSemaphoreCreateBinary();
    // 初回転送待ちを防ぐため最初にGiveしておく
    xSemaphoreGive(display_sem);
    esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, NULL);

    // 内部RAMからバッファを確保
    if (line_buffers[0] == NULL) {
        for (int i = 0; i < 2; i++) {
            line_buffers[i] = heap_caps_malloc(DOOMGENERIC_RESX * CHUNK_LINES * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        }
    }
}


// 描画処理 (32bit RGBA -> 16bit RGB565)
// SRAM容量制限を回避するためCHUNK_LINES単位で分割転送、ダブルバッファリングによりCPU変換とDMA転送を並列化
void DrawFrame() {
    uint32_t *doom_buffer = (uint32_t *)DG_ScreenBuffer;

    for (int y = 0; y < DOOMGENERIC_RESY; y += CHUNK_LINES) {
        // 前のチャンクの送信が終わるのを待つ
        xSemaphoreTake(display_sem, portMAX_DELAY);

        // はみ出し防止
        int lines_to_draw = (y + CHUNK_LINES > DOOMGENERIC_RESY) ? (DOOMGENERIC_RESY - y) : CHUNK_LINES;

        uint16_t *dest = line_buffers[current_buf_idx];
        for (int j = 0; j < lines_to_draw; j++) {
            // ディスプレイに合わせてアスペクト比補正
            int screen_y = y + j;
            int doom_y = (screen_y * SCREENHEIGHT) / DOOMGENERIC_RESY;
            uint32_t *src_ptr = &doom_buffer[doom_y * DOOMGENERIC_RESX];

            for (int x = 0; x < DOOMGENERIC_RESX; x++) {
                uint32_t p = *src_ptr++;
                // RGBA8888 -> RGB565
                uint16_t color = ((p >> 8) & 0xF800) | ((p >> 5) & 0x07E0) | ((p >> 3) & 0x001F);
                // エンディアン変換
                *dest++ = __builtin_bswap16(color);
            }
        }

        // 送信開始
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, DOOMGENERIC_RESX, y + lines_to_draw, line_buffers[current_buf_idx]);

        // バッファを切り替えて並列化
        current_buf_idx = 1 - current_buf_idx;
    }
}
