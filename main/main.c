#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "doomgeneric.h"

void app_main(void) {
    char *argv[] = {"doom", "-iwad", "/sdcard/doom1.wad", "-mb", "4"};

    doomgeneric_Create(5, argv);

    while (1)
    {
        doomgeneric_Tick();
    }

    vTaskDelete(NULL);
}