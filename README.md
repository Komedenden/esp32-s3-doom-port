# ESP32-S3 Doom Port

[DoomGeneric](https://github.com/ozkl/doomgeneric)をESP32-S3 N16R8マイコンへ移植したESP-IDFプロジェクトです。

## Demo

https://github.com/user-attachments/assets/6750d7ab-35be-4be8-bf3c-ca42f4fd12c5


## 特徴

- **ILI9341液晶出力** — RGBA8888 → RGB565変換, 内部SRAMの節約のためチャンク分割してDMA転送
- **ダブルバッファリング描画** — セマフォによる同期でCPUのピクセル変換とDMA転送を並列実行
- **SDカードからWAD読み込み** — SPI接続のSDカードにFATファイルシステムをマウントしてWADファイルを読み込み
- **6ボタン物理入力** —  GPIOボタンをDoomキーイベントにマッピング
- **PSRAMフレームバッファ** — フレームバッファを外部PSRAMに確保し、内部SRAMを節約

## パフォーマンス

- **フレームレート**  15FPS
- **SRAM使用量**    104KB
- **PSRAM使用量**   4.7MB

## ハードウェア

| コンポーネント | 部品 |
|---|---|
| MCU | ESP32-S3 N16R8 |
| ディスプレイ | ILI9341 (SPI接続) |
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
| BUTTON UP | 41 |
| BUTTON DOWN | 21 |
| BUTTON LEFT | 40 |
| BUTTON RIGHT | 42 |
| BUTTON FIRE | 2 |
| BUTTON OPEN | 1 |

## アーキテクチャ

```
esp32-s3-doom-port/
├── main/
│   └── main.c                # main loop
└── components/
    └── doomgeneric/          # ozkl/doomgeneric
    │── hal_config.h          # ピンアサイン定義
    ├── doomgeneric_esp32.c   # DoomGenericの要求する関数を定義
    ├── display_esp32.c       # Display HAL：描画と並列転送
    ├── input_esp32.c         # Input HAL：物理ボタンとキーイベントを抽象化
    └── storage_esp32.c       # Storage HAL：ファイルシステムとストレージを統合
```

DoomGenericが要求する6つの関数を以下のように実装しています。

| 関数 | 説明 |
|---|---|
| `DG_Init()` | SPI バス・LCD・SD カード・GPIO ボタンを初期化 |
| `DG_DrawFrame()` | フレームバッファを変換して LCD へ転送 |
| `DG_GetKey()` | ボタン押下・離しイベントを返す |
| `DG_GetTicksMs()` | `esp_timer_get_time` を使って起動からのミリ秒を返す |
| `DG_SleepMs()` | `vTaskDelay`でスリープ |
| `DG_SetWindowTitle()` | 不要なため何もしない |

### 描画パイプライン

```
DG_ScreenBuffer (PSRAM, RGBA8888)
        │
        │  CPU: 色変換 + アスペクト比補正
        ▼
line_buffers[0 or 1] (内部SRAM, RGB565, CHUNK_LINES行分)
        │
        │  DMA → SPI → LCD
        ▼
       LCD
```

1フレーム全部を一度LCDに送信するとSRAMがパンクするため、1フレームを40行毎に分割して処理します。あるチャンクをDMA転送している間にCPUは次のチャンクを変換することで、両者を並列動作させています。

## セットアップ

FATフォーマットのMicroSDカードのルートにdoom1.wadをコピーし、電源投入前にカードを挿入してください。ファイルは /sdcard/doom1.wad から読み込まれます。
ESP-IDFで本プロジェクトを開き、ビルド後、ESP32-S3にフラッシュをしてください。

## ライセンス

DoomソースコードおよびDoomGenericは**GPL-2.0** ライセンスされています。本ポートも同様にGPL-2.0に従って配布されます。詳細は[LICENSE](LICENSE)を参照してください。
