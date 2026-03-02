#include "input_esp32.h"
#include "hal_config.h"
#include "doomkeys.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define NUM_BTNS 6

static const char *TAG = "DOOM_ESP32_INPUT";

typedef struct {
    int pin;
    unsigned char doom_key;
    bool last_state; // 前回の状態（チャタリング・重複防止）
} btn_config_t;

static btn_config_t btns[NUM_BTNS] = {
    {PIN_BTN_UP,    KEY_UPARROW,    false},
    {PIN_BTN_DOWN,  KEY_DOWNARROW,  false},
    {PIN_BTN_LEFT,  KEY_LEFTARROW,  false},
    {PIN_BTN_RIGHT, KEY_RIGHTARROW, false},
    {PIN_BTN_FIRE,  KEY_FIRE,       false},
    {PIN_BTN_OPEN,  KEY_ENTER,      false}
};


// ハードウェア初期化
void InitInput() {
    // ボタンGPIO初期化
    uint64_t btn_mask = (1ULL<<PIN_BTN_UP) | (1ULL<<PIN_BTN_DOWN) | (1ULL<<PIN_BTN_LEFT) | 
                        (1ULL<<PIN_BTN_RIGHT) | (1ULL<<PIN_BTN_FIRE) | (1ULL<<PIN_BTN_OPEN);
    gpio_config_t btn_cfg = {.pin_bit_mask = btn_mask, .mode = GPIO_MODE_INPUT, .pull_up_en = 1};
    gpio_config(&btn_cfg);
}


// 入力処理
int GetInput(int* pressed, unsigned char* key) {
    static int current_btn_idx = 0;

    while (current_btn_idx < NUM_BTNS) {
        int i = current_btn_idx++;
        bool current_state = (gpio_get_level(btns[i].pin) == 0);

        // 状態が変化したときだけ報告する
        if (current_state != btns[i].last_state) {
            btns[i].last_state = current_state;
            *pressed = current_state ? 1 : 0;
            *key = btns[i].doom_key;
            return 1;
        }
    }
    // 全ボタンのチェックが終わったらリセットして0を返す
    current_btn_idx = 0;
    return 0;
}
