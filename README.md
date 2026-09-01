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
   ├ AutoTune
   ├ Wah / Auto Wah
   ├ Unison
   ├ Chorus
   ├ Delay
   ├ Reverb
   ├ Deep Voice / Robot / Radio / Bitcrusher
   ├ 外部VST3（EQ / Compressor / Noise Suppression など）
   └ 好きな順番で何個でもチェーン
   ↓
CABLE Input
   ↓
VB-CABLE
   ↓
CABLE Output
   ↓
Discord / OBS / Zoom / ブラウザ / ゲーム など
```

Discordなどから見ると、MicVSTで加工された音は普通のマイクと同じように **`CABLE Output`** として見えます。

### 主な機能

- **VST3ホスト** — DAWで使っているVST3エフェクトをマイクへリアルタイム適用
- **11種類のMicVST内蔵DSP**
- **ドラッグでエフェクト順を変更**
- **エフェクトごとのBypass**
- **ダブルクリックでVST3 / 内蔵DSPの設定画面を開く**
- **プラグインごとのレイテンシ表示**
- **Input / Outputレベルメーター**
- **現在の推定レイテンシ表示**
- **VST3フォルダの追加・管理**
- **パラメータ・並び順・Bypass状態を自動保存**
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
| **`MicVST-Setup-1.2.0.exe`** | 初めて使う人。VB-CABLEが無ければセットアップ中に自動導入します |
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

## 1. 物理マイクを選ぶ

MicVST上部の **Input** から使いたいマイクを選択します。

```text
Input  : あなたの物理マイク
Output : CABLE Input  ← MicVSTが自動管理
```

Outputは基本的に触る必要がありません。

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

声全体の音程を上下します。

- **Semitones:** -12〜+12半音
- **0.1半音単位**
- **Mix**

### Deep Voice

低い声を作るためのPitch Shift派生エフェクトです。

- **Depth:** -12〜-1半音
- **Warmth:** 低域寄りの丸さ / サチュレーション感
- **Mix**

### Unison — NEW in v1.2.0

声を複数コピーして少しずつ音程・タイミング・左右位置をずらし、**一人の声を太く広げる**エフェクトです。

単なるステレオ拡張ではなく、MicVSTのPitch Shifterを複数並列で動かします。

| パラメータ | 内容 |
| --- | --- |
| **Voices** | 2〜8声 |
| **Detune** | 各声の音程差 0〜40 cent |
| **Stereo spread** | 各声を左右へ広げる量 |
| **Voice stagger** | 各声のタイミング差 0〜30ms |
| **Mix** | 原音とのブレンド |

**向いている用途**

- 声を太くする
- 複数人っぽい質感
- Choir / Hyperpop系の広い声
- Chorusより強い広がり

8 Voicesは他の内蔵DSPよりCPU負荷が高めです。

---

## Modulation系

### Wah / Auto Wah — NEW in v1.2.0

いわゆる **「ワウワウ」するフィルター**です。

3つのモードがあります。

| Mode | 動き |
| --- | --- |
| **Envelope** | 声の大きさに合わせてフィルターが開閉。喋ると「ワァウ」と動く |
| **LFO** | 一定周期で自動的に「ワウ↑ワウ↓」する |
| **Manual** | フィルター位置を固定して手動調整 |

主なパラメータ：

- **Base frequency:** 180〜1400Hz
- **Sweep range:** 200〜3200Hz
- **Resonance:** 0.4〜8.0Q
- **LFO rate:** 0.10〜8Hz
- **Envelope sensitivity**
- **Manual position**
- **Mix**

普通の声でも変化がかなり分かりやすいエフェクトです。

### Chorus — NEW in v1.2.0

数ms〜数十msの短い遅延時間をLFOで揺らし、声に**厚み・揺れ・ステレオ感**を加えます。

- **Rate:** 0.05〜8Hz
- **Depth:** 0〜15ms
- **Base delay:** 2〜30ms
- **Feedback:** 0〜70%
- **Stereo:** 左右LFOの位相差
- **Mix**

Unisonより軽く、自然に広げたい時に向いています。

---

## Space系

### Delay — NEW in v1.2.0

声を遅れて繰り返すエコーです。

| パラメータ | 内容 |
| --- | --- |
| **Time** | 20〜1500ms |
| **Feedback** | 繰り返し回数 / 減衰量 |
| **Mode** | Stereo / Ping-Pong |
| **Feedback low cut** | エコーの低域を削る |
| **Feedback high cut** | エコーの高域を削る |
| **Mix** | 原音との割合 |

**Ping-Pong**では左右を行き来するようにFeedbackします。

### Reverb — NEW in v1.2.0

部屋やホールのような残響を加えます。軽量なSchroeder系構成をMicVST向けに調整しています。

- **Room size:** 部屋の大きさ
- **Decay:** 0.20〜8秒
- **Pre-delay:** 0〜120ms
- **Damping:** 残響の高域減衰
- **Stereo width:** 残響の左右の広さ
- **Mix**

声を自然に馴染ませる薄いReverbから、かなり長い残響まで作れます。

---

## Character系

### Robot

リングモジュレーション系処理で機械・ロボット声を作ります。

- **Carrier:** 35〜320Hz
- **Drive:** 0〜24dB
- **Mix**

### Radio / Walkie-Talkie

無線・トランシーバー・古い通信機のような音です。

- **Low Cut:** 120〜900Hz
- **High Cut:** 1.8〜9kHz
- **Crunch:** 歪み量
- **Static:** 無線ノイズ量

### Bitcrusher

ビット深度と実質サンプルレートを落として荒いデジタル音にします。

- **Bit Depth:** 2〜16bit
- **Sample Rate:** 1〜48kHz
- **Mix**

---

## Utility

### Mono → Stereo / Stereo → Mono

- **Mono → Stereo** — モノラル音声をステレオチェーンへ送る
- **Stereo → Mono** — ステレオチェーンをモノラルへ戻す

必要な場所だけステレオ化できるので、不要な区間をモノラルで処理してCPU負荷を抑えられます。

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

## 無線・ゲーム内通信風

```text
Radio / Walkie-Talkie
  ↓
Delay（薄め）
```

エフェクトは順番によってかなり音が変わります。例えば **Reverb → Bitcrusher** と **Bitcrusher → Reverb** は全く違う質感になります。

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

MicVSTは主に **48kHzのリアルタイム音声処理**を想定しています。

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

有効時は `--tray` で起動するため、Windowsログイン直後に大きなウィンドウが開くことはありません。

---

# スクリーンショット

## メイン画面

![MicVST main window](assets/screenshot.png)

画面上部でInput / Output・バッファ・状態を確認でき、中央にInput / Outputメーター、下部にエフェクトチェーンが表示されます。

内蔵DSPやVST3を追加すると、チェーン上にエフェクト名・レイテンシ・Bypass・削除ボタンが並びます。

> 現在リポジトリにある実機スクリーンショットはこの1枚です。DSP設定画面やSetup画面のスクリーンショットも今後追加できます。

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
- エフェクトの順番
- VST3 / 内蔵DSPのパラメータ
- Bypass状態
- 追加したVST3フォルダ
- バッファ設定
- ウィンドウ位置 / サイズ
- 自動アップデート確認設定

一度チェーンを作れば、次回からMicVSTを起動するだけで同じ構成を再利用できます。

---

# テスト

GitHub ActionsのWindows環境で、アプリ本体・Unit Test・Setupのビルドを行っています。

v1.2.0では内蔵DSPについて次をテストします。

- 全11 DSPが登録され、生成できること
- 音声を複数ブロック処理してもNaN / Infが出ないこと
- 異常に発散しないこと
- 新DSPの主要パラメータが存在すること
- DSPパラメータのState保存 / 復元
- Delayが指定時間後にエコーを出すこと
- Reverbがインパルス後にTailを生成すること
- Unison 8 Voicesが安定して処理できること

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
WASAPI Output → CABLE Input
      ↓
VB-CABLE signed driver
      ↓
CABLE Output
```

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
installer\out\MicVST-Setup-1.2.0.exe
```

---

# ライセンス

MicVST本体は **GNU General Public License v3.0 (GPL-3.0)** です。

詳しくは [`LICENSE`](LICENSE) を参照してください。
