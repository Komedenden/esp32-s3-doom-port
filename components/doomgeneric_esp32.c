#include "doomgeneric.h"
#include "hal_config.h"
#include "input_esp32.h"
#include "display_esp32.h"
#include "storage_esp32.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "DOOM_ESP32";


// ハードウェア初期化
void DG_Init() {
    ESP_LOGI(TAG, "Initializing Hardware for DOOM...");

    // SPIバス初期化 (LCD & SD共有)
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SPI_SCLK,
        .mosi_io_num = PIN_SPI_MOSI,
        .miso_io_num = PIN_SPI_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(uint16_t)
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    InitDisplay();
    InitInput();
    InitStorage();
}

void DG_DrawFrame() {
    DrawFrame();

    // --- プロファイリング用 ---
    static uint32_t frame_count = 0;
    static uint32_t last_log_time = 0;
    frame_count++;
    uint32_t now = DG_GetTicksMs();
    if (now - last_log_time >= 5000) { // 5秒ごとに詳細ログを出力
        float fps = (float)frame_count * 1000.0f / (now - last_log_time);
        size_t free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI("BENCHMARK", "FPS: %.2f | Free SRAM: %.2f KB | Free PSRAM: %.2f KB", fps, (float)free_sram/1024.0f, (float)free_psram/1024.0f);
        frame_count = 0;
        last_log_time = now;
    }
    // ------------------------
}

int DG_GetKey(int* pressed, unsigned char* key) {
    return GetInput(pressed, key);
}

uint32_t DG_GetTicksMs() { 
    return esp_timer_get_time() / 1000; 
}

void DG_SleepMs(uint32_t ms) { 
    vTaskDelay(pdMS_TO_TICKS(ms)); 
}

void DG_SetWindowTitle(const char * title){
    return;
}