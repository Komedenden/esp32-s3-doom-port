# ESP32-S3 Doom Port

[DoomGeneric](https://github.com/ozkl/doomgeneric) を ESP32-S3 マイコンへ移植し たESP-IDF プロジェクトです。


## 特徴

- **ST7789 LCD 出力** — RGBA8888 → RGB565 変換をチャンク分割 DMA 転送で実現
- **ダブルバッファリング描画** — FreeRTOS セマフォによる同期で CPU のピクセル変換と DMA 転送を並列実行
- **SD カードから WAD 読み込み** — SPI 接続の SD カードに FAT ファイルシステムをマウントして WAD ファイルを読み込み
- **6 ボタン物理入力** —  GPIO ボタンを Doom キーイベントにマッピング
- **PSRAM フレームバッファ** — フレームバッファを外部 PSRAM に確保し、内部 SRAM を節約

## パフォーマンス

- **フレームレート**  15FPS
- **SRAM 使用量**    104KB
- **PSRAM 使用量**   4.7MB

## ハードウェア

| コンポーネント | 部品 |
|---|---|
| MCU | ESP32-S3 (PSRAM搭載) |
| ディスプレイ | ST7789 (SPI接続) |
| ストレージ | MicroSD カード (SPI接続) |
| 入力 | タクトスイッチ × 6 |

### ピンアサイン

[`components/hal_config.h`](components/hal_config.h) で一元管理されています。

| 信号 | GPIO |
|---|---|
| SPI SCLK | 12 |
| SPI MOSI | 11 |
| SPI MISO | 13 |
| LCD CS | 10 |
| LCD DC | 9 |
| LCD RST | 14 |
| SD CS | 4 |
| ボタン UP | 41 |
| ボタン DOWN | 21 |
| ボタン LEFT | 40 |
| ボタン RIGHT | 42 |
| ボタン FIRE | 2 |
| ボタン OPEN/USE | 1 |

## アーキテクチャ

```
esp32-s3-doom-port/
├── main/
│   └── main.c                # main loop
└── components/
    └── doomgeneric/          # ozkl/doomgeneric
    │── hal_config.h          # ピンアサイン定義（単一の設定ファイル）
    ├── doomgeneric_esp32.c   # DoomGeneric HAL エントリポイント
    ├── display_esp32.c       # ST7789 LCD ドライバ + ダブルバッファリング描画
    ├── input_esp32.c         # GPIO ボタンドライバ
    └── storage_esp32.c       # SD カード (FAT/SPI) ドライバ
```

DoomGeneric が要求する 5 つの関数を以下のように実装しています。

| 関数 | 説明 |
|---|---|
| `DG_Init()` | SPI バス・LCD・SD カード・GPIO ボタンを初期化 |
| `DG_DrawFrame()` | フレームバッファを変換して LCD へ転送 |
| `DG_GetKey()` | ボタン押下・離しイベントを返す |
| `DG_GetTicksMs()` | `esp_timer_get_time` を使って起動からのミリ秒を返す |
| `DG_SleepMs()` | `vTaskDelay`でスリープ |

### 描画パイプライン

```
DG_ScreenBuffer (PSRAM, RGBA8888)
        │
        │  CPU: 色変換 + アスペクト比補正
        ▼
line_buffers[0 or 1] (内部 SRAM, RGB565, CHUNK_LINES 行分)
        │
        │  DMA → SPI → LCD
        ▼
       LCD
```

1 フレームを 40 行毎に分割して処理します。あるチャンクを DMA 転送している間に、CPU は次のチャンクを変換することで、両者を並列動作させています。

## セットアップ

### 必要なもの

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/)
- Doom WADファイル

### ビルドとフラッシュ

```bash
git clone --recursive https://github.com/Komedenden/esp32-s3-doom-port.git
cd esp32-s3-doom-port
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### WAD ファイルの配置

FAT フォーマットの MicroSD カードのルートに `doom1.wad` をコピーし、電源投入前にカードを挿入してください。ファイルは `/sdcard/doom1.wad` から読み込まれます。

## ライセンス

Doom ソースコードおよび DoomGeneric は **GPL-2.0** でライセンスされています。本ポートも同様に GPL-2.0 に従って配布されます。詳細は [LICENSE](LICENSE) を参照してください。
