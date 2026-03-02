# フォルダ内のすべての .c ファイルを取得
file(GLOB_RECURSE SOURCES "doomgeneric/*.c" "doomgeneric_esp32.c" "display_esp32.c" "storage_esp32.c" "input_esp32.c")

# ESP32ではビルドできない（SDLやAllegroに依存する）ファイルを除外リストに入れる
set(exclude_files
    "doomgeneric/i_sdl.*"
    "doomgeneric/i_allegro.*"
    "doomgeneric/i_x.c"
    "doomgeneric/i_win32.c"
    "doomgeneric/i_main.c"  # doomgeneric_esp32.c または main.c で代用するため
)

# リストから除外を実行
foreach(excl ${exclude_files})
    list(FILTER SOURCES EXCLUDE REGEX ${excl})
endforeach()

# コンポーネントの登録
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "doomgeneric" "."
    REQUIRES "driver" "esp_lcd" "fatfs" "esp_partition" "esp_timer" "esp_driver_i2s"
)

# 警告をすべて無視してビルドを通す設定
target_compile_options(${COMPONENT_LIB} PRIVATE "-w" "-Wno-error")