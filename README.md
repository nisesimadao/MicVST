# MicVST

**Windowsのマイク音声に、内蔵DSPやVST3をリアルタイムでかけて、そのままDiscord・OBS・Zoom・ゲームなどの「マイク」として使える軽量アプリです。**

[![Latest Release](https://img.shields.io/github/v/release/nisesimadao/MicVST?label=Release)](https://github.com/nisesimadao/MicVST/releases/latest)
[![Windows](https://img.shields.io/badge/Windows-10%20%2F%2011-blue)](#対応環境)
[![License](https://img.shields.io/github/license/nisesimadao/MicVST)](LICENSE)

> **VoiceMeeterは不要です。**  
> MicVSTが処理後の音声を **VB-CABLE** に自動で送り、`CABLE Output` を仮想マイクとして使います。

<p align="center">
  <img src="assets/screenshot.png" alt="MicVSTのメイン画面" width="760">
</p>
<p align="center"><sub>MicVST メイン画面。Inputを選び、下のラックへDSPやVST3を追加して使います。</sub></p>

---

## MicVSTで何ができる？

普段使っているUSBマイクやヘッドセットの音を、リアルタイムで好きな順番に加工できます。

```text
物理マイク
   ↓
MicVST
   ├ AutoTune / Pitch Shift / Deep Voice
   ├ Wah / Auto Wah
   ├ Unison / Chorus
   ├ Delay / Reverb
   ├ Robot / Radio / Bitcrusher
   ├ 外部VST3（EQ / Compressor / Noise Suppression など）
   └ 好きな順番で何個でもチェーン
   ↓
   ├─ Output  → CABLE Input → CABLE Output → Discord / OBS / Zoom / ゲーム
   │
   └─ Output 2（任意）→ ヘッドホン / スピーカー / Audio Interface
```

**Output** は仮想マイク用としてMicVSTが自動管理します。  
**Output 2** はv1.3.0から追加された自由なモニター出力で、加工後の自分の声を別の再生デバイスでも聞けます。

### 主な機能

- **VST3ホスト** — DAWで使っているVST3エフェクトをマイクへリアルタイム適用
- **11種類のMicVST内蔵DSP**
- **ドラッグでエフェクト順を変更**
- **エフェクトごとのBypass**
- **ダブルクリックでVST3 / 内蔵DSPの設定画面を開く**
- **プラグインごとのレイテンシ表示**
- **Input / Outputレベルメーター**
- **現在の推定レイテンシ表示**
- **Output 2による加工後音声のローカルモニター**
- **VST3フォルダの追加・管理**
- **パラメータ・並び順・Bypass・Output2設定を自動保存**
- **壊れたVST3があっても本体を巻き込みにくい別プロセススキャン**
- **システムトレイ常駐**
- **Windows起動時の自動起動**
- **GitHub Releaseのアップデート確認**
- **VB-CABLEへの自動ルーティング**

---

# ダウンロード

最新バージョンは **[GitHub Releases](https://github.com/nisesimadao/MicVST/releases/latest)** からダウンロードできます。

## どっちを使えばいい？

| ファイル | おすすめする人 |
| --- | --- |
| **`MicVST-Setup-1.3.0.exe`** | 初めて使う人。VB-CABLEが無ければセットアップ中に自動導入します |
| **`MicVST-portable.exe`** | **すでにVB-CABLEが入っている人**。インストール不要でそのまま起動できます |

### Setup版の動作

- **すでにVB-CABLEがある** → 既存のVB-CABLEをそのまま利用。再インストールしません
- **VB-CABLEがない** → VB-Audio公式サーバーから取得し、署名済みドライバカタログを確認してからインストール
- セットアップ中に **Windows起動時に自動起動するか** 選択可能
- 自動起動時は `--tray` で起動し、最初からシステムトレイへ入ります
- MicVSTをアンインストールしても、他アプリが使っている可能性があるためVB-CABLE自体は削除しません

VB-CABLEを初めて導入した直後は、Windowsの再起動が必要になる場合があります。

---

# 使い方 — 3ステップ

## 1. デバイスを選ぶ

MicVST上部でデバイスを設定します。

```text
Input    : あなたの物理マイク
Output   : CABLE Input         ← MicVSTが自動管理・変更不可
Output 2 : Off / ヘッドホン等  ← 任意
```

通常は **Inputだけ選べば動きます**。Outputは触る必要がありません。

加工後の自分の声を聞きたい場合だけ **Output 2** を設定してください。

## 2. DSP / VST3を追加する

**`+ Effect / VST`** から内蔵DSPまたはインストール済みVST3を追加します。

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

- **左のハンドルをドラッグ** → 並び替え
- **Bypass** → そのエフェクトだけ一時OFF
- **ダブルクリック** → パラメータ画面を開く
- **ゴミ箱** → チェーンから削除
- 並び順・パラメータ・Bypassは自動保存

## 3. Discordなどで `CABLE Output` を選ぶ

```text
入力デバイス: CABLE Output
```

OBS / Zoom / ブラウザ / ゲーム内VCでも同じです。

---

# Output 2 — 加工後の声を自分でも聞く

v1.3.0から、通常の **Output** とは別に **Output 2** を追加しました。

Outputは常にVB-CABLEへ送るための内部ルートです。一方Output2はユーザーが自由に選べます。

### 例

```text
Input    : HyperX QuadCast
Output   : CABLE Input
Output 2 : Headphones (USB DAC)
```

この場合：

```text
HyperX QuadCast
      ↓
MicVST DSP / VST3
      ├→ CABLE Input → Discord
      └→ USB DAC → 自分のヘッドホン
```

### 選べるもの

Windowsの再生デバイスであれば、例えば次を選択できます。

- ヘッドホン
- USB DAC
- オーディオインターフェース
- モニタースピーカー
- HDMI / DisplayPort Audio
- Bluetooth Audio

`CABLE Input` 自体はPrimary Outputですでに使うため、Output2一覧からは除外しています。

### Off

ローカルモニターが不要なら **Output 2 = Off** にしてください。これがデフォルトです。

### OutputとOutput2のサンプルレートが違ってもOK

Output2はPrimary Outputとは別のWASAPIデバイスとして動作します。

例えば：

```text
MicVST / CABLE Input : 48000 Hz
Headphones Output2  : 44100 Hz
```

でも動作できるよう、Output2側で軽量なリサンプリングを行います。また別々のオーディオデバイスはクロック速度が微妙に違うため、バッファ量を見ながらごく小さく読み取り速度を補正します。

### Output2のレイテンシ

Output2は仮想マイク経路を止めないことを優先し、独立した小さな安全バッファを持っています。目安は **約20ms + Output2デバイス自身のバッファ**です。

つまりOutput2は主に「加工結果を確認する」「自分の声をモニターする」用途です。仮想マイク本体のCABLE Output経路にこの追加バッファは入りません。

> **注意:** Output2にスピーカーを選び、その音が物理マイクへ戻るとハウリングする場合があります。自声モニターにはヘッドホン推奨です。

Output2の選択も自動保存されます。USB DACなどを一時的に抜いても設定名は保持され、UIでは `unavailable` と表示されます。

---

# MicVST独自の内蔵DSP

外部VST3を1つも入れなくても声を加工できます。

これらはプリセットではなく、MicVST内に実装されたリアルタイムDSPです。すべてVST3と同じラックへ追加でき、**並び替え / Bypass / 設定保存**に対応します。

## Pitch / Voice系

### AutoTune

入力音声の基本周波数を検出し、指定したキー / スケール上の近い音へピッチを補正します。

| パラメータ | 内容 |
| --- | --- |
| **Strength** | 補正の強さ 0〜100% |
| **Retune speed** | 目標音程へ追従する速さ 0〜250ms |
| **Key** | C〜B |
| **Scale** | Chromatic / Major / Minor |
| **Mix** | 原音と補正音の割合 |

- 声向けの約 **70〜500Hz** を中心に音程検出
- 補正量は最大 **±7半音**
- 低レイテンシのボイスチャット用途を優先した実装

> DAW向けの高価なピッチ補正ソフトと同じマスタリング品質を狙ったものではありません。

### Pitch Shift

- **Semitones:** -12〜+12半音
- **0.1半音単位**
- **Mix**

### Deep Voice

- **Depth:** -12〜-1半音
- **Warmth:** 低域寄りの丸さ / サチュレーション感
- **Mix**

### Unison

声を複数コピーして少しずつ音程・タイミング・左右位置をずらし、一人の声を太く広げます。

| パラメータ | 内容 |
| --- | --- |
| **Voices** | 2〜8声 |
| **Detune** | 各声の音程差 0〜40 cent |
| **Stereo spread** | 左右へ広げる量 |
| **Voice stagger** | タイミング差 0〜30ms |
| **Mix** | 原音とのブレンド |

8 Voicesは他の内蔵DSPよりCPU負荷が高めです。

---

## Modulation系

### Wah / Auto Wah

いわゆる **「ワウワウ」するフィルター**です。

| Mode | 動き |
| --- | --- |
| **Envelope** | 声量に合わせてフィルターが開閉 |
| **LFO** | 一定周期で自動的にワウワウする |
| **Manual** | フィルター位置を固定 |

- **Base frequency:** 180〜1400Hz
- **Sweep range:** 200〜3200Hz
- **Resonance:** 0.4〜8.0Q
- **LFO rate:** 0.10〜8Hz
- **Envelope sensitivity**
- **Manual position**
- **Mix**

### Chorus

数ms〜数十msの短い遅延をLFOで揺らし、声に厚み・揺れ・ステレオ感を加えます。

- **Rate:** 0.05〜8Hz
- **Depth:** 0〜15ms
- **Base delay:** 2〜30ms
- **Feedback:** 0〜70%
- **Stereo**
- **Mix**

---

## Space系

### Delay

| パラメータ | 内容 |
| --- | --- |
| **Time** | 20〜1500ms |
| **Feedback** | 繰り返し量 |
| **Mode** | Stereo / Ping-Pong |
| **Feedback low cut** | エコーの低域を削る |
| **Feedback high cut** | エコーの高域を削る |
| **Mix** | 原音との割合 |

### Reverb

- **Room size**
- **Decay:** 0.20〜8秒
- **Pre-delay:** 0〜120ms
- **Damping**
- **Stereo width**
- **Mix**

---

## Character系

### Robot

- **Carrier:** 35〜320Hz
- **Drive:** 0〜24dB
- **Mix**

### Radio / Walkie-Talkie

- **Low Cut:** 120〜900Hz
- **High Cut:** 1.8〜9kHz
- **Crunch:** 歪み量
- **Static:** 無線ノイズ量

### Bitcrusher

- **Bit Depth:** 2〜16bit
- **Sample Rate:** 1〜48kHz
- **Mix**

---

## Utility

### Mono → Stereo / Stereo → Mono

- **Mono → Stereo** — モノラル音声をステレオチェーンへ送る
- **Stereo → Mono** — ステレオチェーンをモノラルへ戻す

---

# おすすめチェーン例

## 普通に聞きやすいVC

```text
Noise Suppression (VST3)
  ↓
EQ (VST3)
  ↓
Compressor (VST3)
```

## 太くて広い声

```text
Deep Voice
  ↓
Unison
  ↓
Chorus
  ↓
Reverb
```

## Hyperpop / 強めの加工

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

## ネタVC

```text
Wah / Auto Wah
  ↓
Robot
  ↓
Bitcrusher
  ↓
Delay
```

エフェクトは順番によってかなり音が変わります。

---

# VST3ホスト機能

MicVSTは通常のWindows VST3エフェクトも読み込めます。

たとえば：

- EQ
- Compressor
- Gate
- Noise Suppression
- De-Esser
- Distortion
- Voice Changer
- その他VST3エフェクト

## 検索して追加

`+ Effect / VST` を押すと、内蔵DSPと検出済みVST3を同じ一覧から検索できます。

## カスタムVST3フォルダ

標準フォルダ以外に置いている場合は **Manage VST3 Folders** から追加できます。

## 安全寄りのプラグインスキャン

VST3の中には、スキャンしただけでクラッシュしたり固まるものがあります。

MicVSTではプラグインスキャンを**メインアプリとは別プロセス**で行います。

- 壊れたプラグインが落ちてもMicVST本体を巻き込みにくい
- スキャン中のプラグイン名と進捗を表示
- 固まったプラグインを **Skip** 可能
- スキップされたプラグインだけ後から再試行可能
- 結果をキャッシュして次回起動を高速化

---

# レベルメーター / レイテンシ

メイン画面にはリアルタイムの **Input / Outputメーター** があります。

現在のオーディオデバイス・バッファサイズ・各プラグインが報告するレイテンシを元に、推定レイテンシも表示します。

各エフェクト行にも、そのプラグイン単体のレイテンシをms単位で表示します。

MicVSTは主に **48kHzのリアルタイム音声処理**を想定しています。Output2だけ別サンプルレートでも利用できます。

---

# システムトレイ / 自動起動

MicVSTは常駐利用に対応しています。

### ×ボタンを押したとき

終了せず、**システムトレイへ収納**されます。音声処理はそのまま継続します。

### トレイアイコン

- **左クリック:** ウィンドウを表示 / 非表示
- **右クリック:** `Run at Windows startup` / `Quit`

完全終了はトレイメニューの **Quit** から行います。

### Windows起動時に自動起動

次の2か所から設定できます。

1. Setup版インストール時の **Start MicVST with Windows**
2. MicVST本体 / トレイメニューの **Run at Windows startup**

有効時は `--tray` で起動します。

---

# スクリーンショット

## メイン画面

![MicVST main window](assets/screenshot.png)

v1.3.0では上部のデバイス欄に **Output 2** が追加されます。スクリーンショットは旧UIのため、実際の最新版ではInput / Output / Output 2の3行になります。

---

# VB-CABLEについて

MicVSTの通常版は仮想マイクの転送部分に **VB-Audio SoftwareのVB-CABLE** を利用します。

```text
MicVSTの処理済み音声
        ↓
CABLE Input
        ↓
VB-CABLE Driver
        ↓
CABLE Output
        ↓
Discordなど
```

MicVSTが `CABLE Input` を自動管理するため、VoiceMeeterのようなルーティングソフトを別途設定する必要はありません。

VB-CABLEの公式署名済みドライバをそのまま利用するため、MicVSTの通常利用で **Secure Bootを無効化する必要はありません**。

> VB-CABLEはMicVSTとは別のサードパーティ製ソフトウェアで、VB-Audio SoftwareによるDonationwareです。

- 公式サイト: https://vb-cable.com/
- ライセンス / 配布条件: https://vb-audio.com/Services/licensing.htm
- MicVST内の通知: [`installer/VB-CABLE-NOTICE.txt`](installer/VB-CABLE-NOTICE.txt)

---

# 設定の保存

以下は自動保存されます。

- 使用する物理マイク
- **Output2の選択**
- エフェクトの順番
- VST3 / 内蔵DSPのパラメータ
- Bypass状態
- 追加したVST3フォルダ
- バッファ設定
- ウィンドウ位置 / サイズ
- 自動アップデート確認設定

---

# テスト

GitHub ActionsのWindows環境で、アプリ本体・Unit Test・Setupのビルドを行っています。

主なテスト：

- 全11 DSPの登録・生成・安定性
- DSP State保存 / 復元
- Delay echo / Reverb tail
- Unison 8 Voices
- **Output2が安全バッファ未満では部分音声を出さないこと**
- **Output2でステレオ加工音が正しく複製されること**
- **48kHz → 44.1kHzなど異なるSample Rateでも有限値・音声が出ること**
- **Output2設定のState保存 / 復元**

---

# 対応環境

- **Windows 10 / 11 x64**
- WASAPI
- VST3
- 48kHz推奨

通常利用ではVB-CABLEが必要です。Setup版なら未導入時に自動セットアップされます。

---

# 開発者向け

MicVSTは **C++17 + JUCE 8.0.13** で実装されています。

```text
WASAPI Mic Input
      ↓
JUCE AudioProcessorGraph
      ↓
Built-in DSP / VST3 AudioPluginInstance
      ↓
      ├→ Primary WASAPI Output → CABLE Input → VB-CABLE → CABLE Output
      │
      └→ MonitorBuffer → Secondary WASAPI Device (Output2)
```

Output2はPrimary callbackから加工済みサンプルをSPSCリングへ書き込み、別のAudioDeviceManagerのcallbackが読み出します。2つのデバイスのクロック差はキュー量に応じた小さなresampling ratio補正で吸収します。

通常版ではVB-CABLEを使用しますが、将来用の実験的な独自Virtual Audio Driver実装も [`driver/`](driver/) に残しています。

## ビルド

必要なもの：

- Visual Studio 2022以降
- Desktop development with C++
- Windows SDK
- CMake
- Git

JUCE 8.0.13はCMakeから自動取得されます。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MicVST MicVSTTests
.\build\MicVSTTests_artefacts\Release\MicVSTTests.exe
```

アプリ：

```text
build\MicVST_artefacts\Release\MicVST.exe
```

## Setup版をビルド

Inno Setup 6をインストールした状態で：

```powershell
.\installer\build-installer.ps1
```

生成物：

```text
installer\out\MicVST-Setup-1.3.0.exe
```

---

# ライセンス

MicVST本体は **GNU General Public License v3.0 (GPL-3.0)** です。

詳しくは [`LICENSE`](LICENSE) を参照してください。
