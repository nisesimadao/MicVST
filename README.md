# MicVST

**Windowsのマイク音声に内蔵DSPやVST3をリアルタイムでかけ、そのままDiscord・OBS・Zoom・ゲームなどの「マイク」として使える軽量アプリです。**

[![Latest Release](https://img.shields.io/github/v/release/nisesimadao/MicVST?label=Release)](https://github.com/nisesimadao/MicVST/releases/latest)
[![Windows](https://img.shields.io/badge/Windows-10%20%2F%2011-blue)](#対応環境)
[![License](https://img.shields.io/github/license/nisesimadao/MicVST)](LICENSE)

> **VoiceMeeterは不要です。** MicVSTが加工後の音をVB-CABLEへ自動で送り、Discordなどでは `CABLE Output` をマイクとして選ぶだけです。

<p align="center">
  <img src="assets/screenshot.png" alt="MicVSTのメイン画面" width="760">
</p>
<p align="center"><sub>既存スクリーンショット。v1.4.0ではこの画面にOutput 2と折りたたみ式Audio Padsが追加されています。</sub></p>

---

# 何ができる？

```text
物理マイク
   ↓
MicVST
   ├ AutoTune / Pitch Shift / Deep Voice
   ├ Wah / Auto Wah
   ├ Unison / Chorus
   ├ Delay / Reverb
   ├ Robot / Radio / Bitcrusher
   ├ 外部VST3
   └ Audio Pads / Soundboard
   ↓
   ├─ Output  → CABLE Input → CABLE Output → Discord / OBS / Zoom / ゲーム
   │
   └─ Output 2（任意）→ ヘッドホン / USB DAC / Audio Interface など
```

### 主な機能

- **VST3ホスト** — DAWで使うVST3エフェクトをマイクへリアルタイム適用
- **11種類のMicVST独自DSP**
- **Audio Pads / Soundboard** — 16パッド、ドラッグ&ドロップ、Loop、Fade、グローバルHotkey
- **Padごとのルーティング** — `Post FX` / `Pre FX` / `Output2 only`
- エフェクトのドラッグ並び替え / Bypass / 個別Editor
- Input / Outputレベルメーター、Plugin単位と全体のレイテンシ表示
- **Output 2** — 加工後の音を別のヘッドホン等でもモニター
- VST3の別プロセススキャン、Skip、キャッシュ
- パラメータ・並び順・Pad・Output2などを自動保存
- システムトレイ常駐
- Windows起動時の自動起動
- GitHub Releaseの更新確認
- VB-CABLEへの自動ルーティング

---

# ダウンロード

最新バージョンは **[GitHub Releases](https://github.com/nisesimadao/MicVST/releases/latest)** から取得できます。

| ファイル | どんな人向け？ |
| --- | --- |
| **`MicVST-Setup-1.4.0.exe`** | 初めて使う人。VB-CABLEがなければセットアップ中に導入します |
| **`MicVST-portable.exe`** | **すでにVB-CABLEが入っている人**。インストール不要 |

### Setup版

- VB-CABLEが既にある → **再インストールせず既存のものを利用**
- VB-CABLEがない → VB-Audio公式配布物を取得して導入
- `Start MicVST with Windows` でWindows起動時の自動起動を選択可能
- 自動起動時は `--tray` で起動し、最初からトレイへ収納
- MicVSTをアンインストールしても共有コンポーネントのVB-CABLEは削除しません

---

# 基本的な使い方

## 1. デバイスを選ぶ

```text
Input    : 物理マイク
Output   : CABLE Input          ← 自動管理・変更不可
Output 2 : Off / Headphones等   ← 任意
```

普通に仮想マイクとして使うだけなら **Inputだけ選べばOK** です。

## 2. DSP / VST3を追加

`+ Effect / VST` から内蔵DSPまたはVST3を追加します。

```text
Noise Suppression (VST3)
  ↓
AutoTune
  ↓
Unison
  ↓
Chorus
  ↓
Reverb
```

- 左のハンドル → ドラッグで並び替え
- Bypass → そのエフェクトだけOFF
- ダブルクリック → 設定画面
- ゴミ箱 → チェーンから削除

## 3. 必要ならAudio Padsを使う

`Audio Pads >` をクリックすると16パッドのSoundboardが開きます。

## 4. Discordなどで `CABLE Output` を選ぶ

```text
入力デバイス: CABLE Output
```

OBS / Zoom / ブラウザ / ゲーム内VCでも同じです。

---

# Audio Pads / Soundboard — NEW in v1.4.0

MicVSTだけで効果音・ボイスクリップ・ジングル・BGMを鳴らせます。

普段は折りたたまれているため、Soundboardを使わないときにVSTラックを圧迫しません。

## 16パッド

4×4の16個です。

```text
┌────────┬────────┬────────┬────────┐
│ Pad 1  │ Pad 2  │ Pad 3  │ Pad 4  │
├────────┼────────┼────────┼────────┤
│ Pad 5  │ Pad 6  │ Pad 7  │ Pad 8  │
├────────┼────────┼────────┼────────┤
│ Pad 9  │ Pad 10 │ Pad 11 │ Pad 12 │
├────────┼────────┼────────┼────────┤
│ Pad 13 │ Pad 14 │ Pad 15 │ Pad 16 │
└────────┴────────┴────────┴────────┘
```

### 音声を入れる

次のどちらでも登録できます。

- 音声ファイルを**パッドへドラッグ&ドロップ**
- Padを選んで **Load**

複数ファイルをまとめてドロップすると、ドロップしたPad以降へ順番に入ります。

### 対応形式

- WAV
- MP3
- FLAC
- OGG
- AIFF / AIF

ファイルはSoundboard向けにメモリへデコードして再生します。極端に巨大なファイルによるメモリ消費を防ぐため、**1ファイルあたりデコード後512MBまで**です。

## 3つのRoute

これがMicVSTのAudio Padsで重要な部分です。

| Route | どう流れる？ | 用途 |
| --- | --- | --- |
| **Post FX** | DSP/VSTの**後**でミックス → Discordにも送る | 効果音、BGM、普通のSoundboard |
| **Pre FX** | マイクと一緒にDSP/VSTの**前**へ入れる | ボイスクリップをRobot/AutoTune/Reverb等で加工 |
| **Output2 only** | CABLE Inputへ送らず**Output2だけ** | 自分だけに聞こえるCue、Preview、メトロノーム等 |

### Post FX

```text
Mic → AutoTune → Chorus ─┐
                          ├→ CABLE Input → Discord
Audio Pad ────────────────┘
```

効果音にマイク用AutoTune等をかけたくない場合のデフォルトです。

### Pre FX

```text
Mic ──────┐
          ├→ Robot → Delay → CABLE Input
Audio Pad ┘
```

Padのボイスクリップにも同じエフェクトをかけられます。

### Output2 only

```text
Audio Pad → Output2 → Headphones
                    
CABLE Input / Discord には送られない
```

## Padごとの設定

| 設定 | 内容 |
| --- | --- |
| **Name** | パッド名 |
| **Vol** | Pad単体の音量。最大150% |
| **Loop** | 停止するまで繰り返す |
| **Route** | Post FX / Pre FX / Output2 only |
| **Retrigger** | 再度押したときの動作 |
| **Hotkey** | Windows全体で反応するショートカット |
| **In ms** | Fade In |
| **Out ms** | Fade Out / Stop時Fade |
| **Color** | Padの見分け用の控えめな色分け |

パッド下部には再生位置のProgressが表示されます。

### Retrigger

| Mode | 再生中にもう一度押すと |
| --- | --- |
| **Restart** | 頭から再生し直す |
| **Stop** | Fade Outして停止 |
| **Ignore** | 何もしない |

## グローバルHotkey

MicVSTがトレイに隠れていてもWindows全体で反応します。

例：

```text
F8
F12
Numpad1
Ctrl+Shift+1
Alt+Q
```

通常の文字キーを単独で設定すると、チャット入力中などにも反応するので、**FキーかCtrl/Alt付きがおすすめ**です。

Hotkeyは押しっぱなしで連打されず、押した瞬間だけTriggerされます。

## Master Volume / Stop all

- `Master` → 16Pad全体の音量
- `Stop all` → 再生中のPadをまとめてFade停止

## Pad設定の保存

以下は再起動後も復元されます。

- ファイルパス
- 名前
- 音量
- Loop
- Route
- Retrigger
- Hotkey
- Fade In / Out
- Color
- Master Volume

元ファイルが一時的に見つからなくても保存設定自体は消しません。

---

# Output 2 — 加工後の音を自分でも聞く

Output 2は自由に選べるローカルモニター出力です。

```text
Input    : USB Microphone
Output   : CABLE Input
Output 2 : Headphones (USB DAC)
```

```text
USB Microphone
      ↓
MicVST DSP / VST3
      ├→ CABLE Input → Discord
      └→ Output2 → 自分のヘッドホン
```

### 選択できる例

- ヘッドホン
- USB DAC
- オーディオインターフェース
- スピーカー
- HDMI / DisplayPort Audio
- Bluetooth Audio

`CABLE Input` はPrimary側で使用するためOutput2一覧から除外されます。

### 異なるSample Rateにも対応

例：

```text
MicVST / CABLE : 48000 Hz
Output2 DAC    : 44100 Hz
```

Output2側で軽量リサンプリングとクロック差補正を行います。

Output2は仮想マイク経路を止めないため、小さな安全バッファを持ちます。目安は **約20ms + Output2デバイス自身のバッファ**です。この追加分はDiscordへ行くPrimary経路には入りません。

> スピーカー出力が物理マイクへ戻るとハウリングする可能性があります。モニターにはヘッドホン推奨です。

---

# MicVST独自の内蔵DSP

外部VST3を入れなくても、以下をMicVST単体で使えます。

| DSP | できること | 主な設定 |
| --- | --- | --- |
| **AutoTune** | 指定Key/Scaleへピッチ補正 | Strength / Retune / Key / Scale / Mix |
| **Pitch Shift** | 声の高さ変更 | -12〜+12 semitone / Mix |
| **Deep Voice** | 低く太い声 | Depth / Warmth / Mix |
| **Wah / Auto Wah** | ワウワウするFilter | Envelope / LFO / Manual / Frequency / Q / Mix |
| **Unison** | 2〜8声へ多重化 | Voices / Detune / Spread / Stagger / Mix |
| **Chorus** | 揺れ・厚み・Stereo感 | Rate / Depth / Delay / Feedback / Stereo / Mix |
| **Delay** | Echo / Ping-Pong | Time / Feedback / Low Cut / High Cut / Mix |
| **Reverb** | Room/Hall系残響 | Room / Decay / Pre-delay / Damping / Width / Mix |
| **Robot** | 機械・Robot声 | Carrier / Drive / Mix |
| **Radio** | 無線・Walkie-Talkie | Low Cut / High Cut / Crunch / Static |
| **Bitcrusher** | 荒いDigital音 | Bit Depth / Sample Rate / Mix |

Utilityとして `Mono → Stereo` / `Stereo → Mono` もあります。

### AutoTune

- Key: C〜B
- Scale: Chromatic / Major / Minor
- Strength: 0〜100%
- Retune: 0〜250ms
- 声向け約70〜500Hzを中心に検出
- 最大補正量 ±7半音

低レイテンシVC用途を優先した実装で、DAW向け高級Pitch Correctionと同等のMastering品質を狙ったものではありません。

### Unison

- 2〜8 Voices
- Detune 0〜40 cent
- Stereo Spread
- Voice Stagger 0〜30ms
- Mix

8 Voicesは他の内蔵DSPよりCPU負荷が高めです。

### Wah / Auto Wah

- Envelope — 声量に合わせてFilter開閉
- LFO — 一定周期でワウワウ
- Manual — 位置固定

### Delay / Reverb

Delayは20〜1500ms、Stereo / Ping-Pong。ReverbはDecay 0.2〜8秒、Pre-delay 0〜120msなどを調整できます。

---

# おすすめ例

### 普通に聞きやすいVC

```text
Noise Suppression (VST3)
 ↓
EQ (VST3)
 ↓
Compressor (VST3)
```

### 太く広い声

```text
Deep Voice
 ↓
Unison
 ↓
Chorus
 ↓
Reverb
```

### Hyperpop / 強加工

```text
AutoTune
 ↓
Pitch Shift
 ↓
Unison
 ↓
Delay
 ↓
Reverb
```

### Soundboardの声まで加工

Pad Route = Pre FX

```text
Audio Pad
 ↓
AutoTune
 ↓
Robot
 ↓
Delay
 ↓
Discord
```

---

# VST3ホスト

通常のWindows VST3エフェクトを読み込めます。

例：

- EQ
- Compressor
- Gate
- Noise Suppression
- De-Esser
- Distortion
- Voice Changer

`+ Effect / VST` では内蔵DSPと検出済みVST3を同じ検索画面から追加できます。

標準場所以外は **Manage VST3 Folders** から追加できます。

### 安全寄りのPlugin Scan

VST3スキャンはメインアプリとは別プロセスで行います。

- スキャン対象PluginがCrashしても本体を巻き込みにくい
- 進捗とPlugin名を表示
- 固まったPluginをSkip
- SkipしたものだけRetry
- 結果をCacheして次回起動を高速化

---

# システムトレイ / 自動起動

×ボタンでは終了せず、**トレイへ収納**されます。音声処理とAudio PadのグローバルHotkeyはそのまま動き続けます。

トレイアイコン：

- 左クリック → Window表示 / 非表示
- 右クリック → `Run at Windows startup` / `Quit`

完全終了は `Quit` を使います。

Windows自動起動はSetup時またはトレイから設定できます。有効時は `--tray` で静かに起動します。

---

# 設定の保存

`%APPDATA%\MicVST\config.xml` に保存されます。

- 物理マイク
- Output2
- Plugin / DSPの順番・パラメータ・Bypass
- VST3追加Folder
- Buffer設定
- **Audio Pads全設定**
- Window位置 / サイズ
- Update Check設定

---

# テスト

GitHub ActionsのWindows環境でアプリ本体・Unit Test・Setupをビルドしています。

主なテスト：

- 全11 DSPの生成・NaN/Inf・State復元
- Delay echo / Reverb tail
- Unison 8 Voices
- Output2 safety buffer / Stereo / 48k→44.1k
- **一時WAVを実際に生成してAudio Padへ読ませるテスト**
- **Audio Pad Post FX / Pre FX / Output2-onlyの3Bus確認**
- Audio Pad Loop
- Audio Pad Fade Stop
- Audio Pad設定のState保存 / 復元

---

# VB-CABLEについて

通常版は仮想マイク転送に **VB-Audio SoftwareのVB-CABLE** を利用します。

```text
MicVST
 ↓
CABLE Input
 ↓
VB-CABLE Driver
 ↓
CABLE Output
 ↓
Discordなど
```

公式署名済みVB-CABLEをそのまま利用するため、通常利用で **Secure Bootを無効化する必要はありません**。

VB-CABLEはMicVSTとは別のDonationwareです。

- 公式サイト: https://vb-cable.com/
- ライセンス / 配布条件: https://vb-audio.com/Services/licensing.htm
- MicVST内の通知: [`installer/VB-CABLE-NOTICE.txt`](installer/VB-CABLE-NOTICE.txt)

---

# 対応環境

- Windows 10 / 11 x64
- WASAPI
- VST3
- 48kHz推奨

通常利用ではVB-CABLEが必要です。Setup版なら未導入時に自動セットアップします。

---

# 開発者向け

MicVSTは **C++17 + JUCE 8.0.13** です。

```text
Physical Mic ───────────────┐
                            │
Audio Pads (Pre FX) ────────┤
                            ↓
                  AudioProcessorGraph
                   Built-in DSP / VST3
                            ↓
Audio Pads (Post FX) ───────┤
                            ↓
                  CABLE Input / Discord
                            │
                            └→ MonitorBuffer → Output2

Audio Pads (Output2 only) ─────────────────→ Output2
```

Audio PadsはPrimary Audio callback内で3つの独立Busへレンダリングします。Pad素材のSample Rateが異なる場合は線形補間でPrimary clockへ変換します。

Output2はPrimary callbackからSPSCリングへ加工済みSampleを書き、独立したWASAPI callbackが読み出します。

## ビルド

必要：Visual Studio 2022以降 / Desktop development with C++ / Windows SDK / CMake / Git

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MicVST MicVSTTests
.\build\MicVSTTests_artefacts\Release\MicVSTTests.exe
```

アプリ：

```text
build\MicVST_artefacts\Release\MicVST.exe
```

### Setup版

Inno Setup 6導入後：

```powershell
.\installer\build-installer.ps1
```

生成物：

```text
installer\out\MicVST-Setup-1.4.0.exe
```

---

# ライセンス

MicVST本体は **GNU General Public License v3.0 (GPL-3.0)** です。

詳しくは [`LICENSE`](LICENSE) を参照してください。
