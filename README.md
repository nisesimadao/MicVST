# MicVST

**Windowsのマイク音声に、VST3や内蔵エフェクトをリアルタイムでかけて、そのままDiscord・OBS・Zoom・ゲームなどの「マイク」として使える軽量アプリです。**

[![Latest Release](https://img.shields.io/github/v/release/nisesimadao/MicVST?label=Release)](https://github.com/nisesimadao/MicVST/releases/latest)
[![Windows](https://img.shields.io/badge/Windows-10%20%2F%2011-blue)](#対応環境)
[![License](https://img.shields.io/github/license/nisesimadao/MicVST)](LICENSE)

> **VoiceMeeterは不要です。**  
> MicVSTが処理後の音声を **VB-CABLE** に自動で送り、`CABLE Output` を仮想マイクとして使います。

<p align="center">
  <img src="assets/screenshot.png" alt="MicVSTのメイン画面" width="760">
</p>
<p align="center"><sub>MicVST メイン画面。UIは開発中のため細部が変わる場合があります。</sub></p>

---

## 何ができる？

たとえば、普段使っているUSBマイクやヘッドセットの音を次のように加工できます。

```text
物理マイク
   ↓
MicVST
   ├ AutoTune
   ├ Deep Voice
   ├ Radio
   ├ Bitcrusher
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

MicVSTの中でエフェクトを並べるだけで、Discordなどからは普通のマイクと同じように **`CABLE Output`** を選べます。

### 主な機能

- **VST3ホスト** — 普段DAWで使っているVST3エフェクトをマイクにリアルタイム適用
- **MicVST独自の内蔵DSP** — AutoTune / Robot / Radio / Bitcrusher / Pitch Shift / Deep Voice
- **ドラッグでエフェクト順を変更**
- **エフェクトごとのBypass**
- **ダブルクリックでVST3 / 内蔵DSPのエディタを開く**
- **プラグインごとのレイテンシ表示**
- **Input / Outputレベルメーター**
- **現在の推定レイテンシ表示**
- **VST3フォルダの追加・管理**
- **プラグイン設定・並び順・Bypass状態を自動保存**
- **壊れたVST3があっても本体を巻き込みにくい別プロセススキャン**
- **システムトレイ常駐**
- **Windows起動時の自動起動**
- **任意のアップデート確認**
- **VB-CABLEへの自動ルーティング**

---

## ダウンロード

最新バージョンは **[GitHub Releases](https://github.com/nisesimadao/MicVST/releases/latest)** からダウンロードできます。

### どっちを使えばいい？

| ファイル | おすすめする人 |
| --- | --- |
| **`MicVST-Setup-1.1.1.exe`** | 初めて使う人。VB-CABLEが無ければセットアップ中に自動で導入します |
| **`MicVST-portable.exe`** | **すでにVB-CABLEが入っている人**。インストール不要でそのまま起動できます |

### Setup版の動作

Setup版はMicVST本体をインストールしたあと、PCにVB-CABLEがあるか確認します。

- **すでにVB-CABLEがある** → 既存のVB-CABLEをそのまま利用。再インストールしません
- **VB-CABLEがない** → VB-Audio公式サーバーからbase VB-CABLEを取得し、署名済みドライバカタログを確認してからインストール
- **Windows起動時に自動起動**するかをセットアップ画面で選択可能
- 自動起動を有効にすると、Windowsログイン時に `--tray` で静かに起動
- MicVSTをアンインストールしても、他アプリが使っている可能性があるためVB-CABLE自体は削除しません

VB-CABLEを初めて導入した直後は、Windowsの再起動が必要になる場合があります。

---

## 使い方 — 3ステップ

### 1. MicVSTで物理マイクを選ぶ

MicVSTを起動し、上部の **Input** から使いたいマイクを選択します。

```text
Input  : あなたの物理マイク
Output : CABLE Input  ← MicVSTが自動管理
```

Outputは基本的に触る必要がありません。MicVSTがVB-CABLEの `CABLE Input` を自動で使用します。

### 2. エフェクトを追加する

**`+ Effect / VST`** から内蔵DSPまたはインストール済みVST3を追加します。

追加したエフェクトは、左側のドラッグハンドルを掴んで好きな順番へ移動できます。

例：

```text
AutoTune
  ↓
EQ (VST3)
  ↓
Compressor (VST3)
  ↓
Radio
```

- **Bypass** — そのエフェクトだけ一時的に無効化
- **ダブルクリック** — エフェクトの設定画面を開く
- **ゴミ箱** — チェーンから削除
- 並び順・パラメータ・Bypass状態は自動保存

### 3. Discordなどで `CABLE Output` を選ぶ

Discordの場合は、入力デバイスを次のようにします。

```text
入力デバイス: CABLE Output
```

これだけで、MicVSTで加工された音声がDiscordへ送られます。

OBS / Zoom / ブラウザ / ゲーム内VCなどでも、同じように **`CABLE Output`** をマイクとして選択してください。

---

# MicVST独自の内蔵DSP

外部VST3を1つも入れなくても、MicVST単体で声を加工できます。

これらは単なるプリセットではなく、MicVST内にC++ / JUCEで実装された**独自のリアルタイムDSP**です。VST3と同じチェーン内に追加でき、ドラッグ・Bypass・設定保存にも対応しています。

## AutoTune

入力音声の基本周波数を検出し、指定したキー / スケール上の近い音へピッチを補正します。

**主なパラメータ**

| パラメータ | 内容 |
| --- | --- |
| **Strength** | 補正の強さ。0〜100% |
| **Retune speed** | 目標音程へ追従する速さ。0〜250ms |
| **Key** | C〜B |
| **Scale** | Chromatic / Major / Minor |
| **Mix** | 原音と補正音の割合 |

音程検出は主に声向けの約70〜500Hzを対象とし、補正量は最大±7半音に制限しています。

> AutoTuneは**ボイスチャット向けの低レイテンシ実装**です。DAW用の高価なピッチ補正ソフトと同じマスタリング品質を狙ったものではありません。

## Pitch Shift

声全体の音程をリアルタイムで上下させます。

- **Pitch:** -12〜+12 semitones
- **0.1半音単位**で調整
- **Mix**で原音とブレンド

少しだけ声を高くする用途から、完全に別人っぽい声まで調整できます。

## Deep Voice

Pitch Shiftを低い声向けに調整し、さらに暖かさを加えるエフェクトです。

- **Depth:** -12〜-1 semitones
- **Warmth:** 低域寄りの質感・サチュレーション感を調整
- **Mix:** 原音とのブレンド

単純に音程だけを下げるより、低い声として聞きやすい方向へ寄せます。

## Robot

リングモジュレーション系の処理で、機械・ロボットのような声を作ります。

- **Carrier:** 35〜320Hz
- **Drive:** 0〜24dB
- **Mix**

軽い機械感から、かなり強いロボットボイスまで調整できます。

## Radio / Walkie-Talkie

無線・トランシーバー・古い通信機のような音を作ります。

- **Low Cut:** 120〜900Hz
- **High Cut:** 1.8〜9kHz
- **Crunch:** 歪み感
- **Static:** 無線ノイズ量

単なるEQだけでなく、帯域制限・歪み・ノイズを組み合わせています。

## Bitcrusher

音声のビット深度と実質サンプルレートを落とし、ゲーム機・古いデジタル機器のような荒い音にします。

- **Bit Depth:** 2〜16bit
- **Sample Rate:** 1kHz〜48kHz
- **Mix**

## Mono → Stereo / Stereo → Mono

チャンネル構成を変換するユーティリティノードです。

- **Mono → Stereo** — モノラル音声をステレオチェーンへ送る
- **Stereo → Mono** — ステレオチェーンをモノラルへ戻す

必要な場所だけステレオ化できるため、不要な区間をモノラルで処理してCPU負荷を抑える用途にも使えます。

---

# VST3ホスト機能

MicVSTは内蔵DSPだけでなく、通常のWindows VST3エフェクトも読み込めます。

たとえば以下のようなプラグインを利用できます。

- EQ
- Compressor
- Gate
- Noise Suppression
- Reverb
- Distortion
- De-Esser
- Voice Changer
- その他VST3エフェクト

## プラグイン管理

### 検索して追加

`+ Effect / VST` を押すと、内蔵エフェクトと検出済みVST3を同じ一覧から検索できます。

VST3はメーカーごとに整理されるため、大量にインストールしていても探しやすくなっています。

### カスタムVST3フォルダ

標準のVST3フォルダ以外にプラグインを置いている場合は、**Manage VST3 Folders** から追加できます。

### 安全寄りのプラグインスキャン

VST3の中には、スキャンしただけでクラッシュしたり長時間固まるものもあります。

MicVSTではプラグインスキャンを**メインアプリとは別プロセス**で行います。

- 壊れたプラグインが落ちてもMicVST本体を巻き込みにくい
- スキャン中のプラグイン名と進捗を表示
- 固まったプラグインを **Skip** 可能
- スキップされたプラグインだけ後から再試行可能
- スキャン結果をキャッシュして次回起動を高速化

---

# レベルメーター / レイテンシ

メイン画面にはリアルタイムの **Input / Outputメーター** があります。

さらに、現在のオーディオデバイス・バッファサイズ・各プラグインが報告しているレイテンシを元に、推定レイテンシを表示します。

各エフェクト行にも、そのプラグイン単体が報告しているレイテンシをms単位で表示します。Lookaheadを使うコンプレッサーやノイズ除去プラグインを探すときにも便利です。

MicVSTは主に **48kHzのリアルタイム音声処理**を想定しています。

---

# システムトレイ / 自動起動

MicVSTは常駐利用を想定しています。

### ×ボタンを押したとき

アプリは終了せず、**システムトレイへ収納**されます。音声処理もそのまま継続します。

### トレイアイコン

- **左クリック:** ウィンドウを表示 / 非表示
- **右クリック:** `Run at Windows startup` / `Quit`

完全に終了したい場合はトレイアイコンの **Quit** を使用します。

### Windows起動時に自動起動

次の2か所から設定できます。

1. Setup版インストール時の **Start MicVST with Windows**
2. MicVST本体 / トレイメニューの **Run at Windows startup**

有効にするとWindowsログイン時に `--tray` で起動するため、毎回ウィンドウが開くことはありません。

---

# スクリーンショット

## メイン画面

![MicVST main window](assets/screenshot.png)

画面上部ではInput / Output・バッファ・状態を確認でき、中央にInput / Outputメーター、下部にエフェクトチェーンが表示されます。

内蔵DSPやVST3を追加すると、チェーン上にエフェクト名・レイテンシ・Bypass・削除ボタンが並びます。

---

# VB-CABLEについて

MicVSTの通常版は、仮想マイクの転送部分に **VB-Audio SoftwareのVB-CABLE** を利用します。

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

MicVST側では `CABLE Input` への出力を自動管理するため、VoiceMeeterのようなルーティングソフトを別途設定する必要はありません。

VB-CABLEのドライバは署名済みの公式パッケージをそのまま利用するため、MicVSTのために **Secure Bootを無効化する必要はありません**。

> VB-CABLEはMicVSTとは別のサードパーティ製ソフトウェアで、VB-Audio SoftwareによるDonationwareです。

- 公式サイト: https://vb-cable.com/
- ライセンス / 配布条件: https://vb-audio.com/Services/licensing.htm
- MicVST内の通知: [`installer/VB-CABLE-NOTICE.txt`](installer/VB-CABLE-NOTICE.txt)

---

# 設定の保存

以下の内容は自動的に保存されます。

- 使用する物理マイク
- エフェクトの順番
- VST3 / 内蔵DSPのパラメータ
- Bypass状態
- 追加したVST3フォルダ
- バッファ設定
- ウィンドウ位置 / サイズ
- 自動アップデート確認設定

そのため、普段は一度チェーンを作れば、次回からMicVSTを起動するだけで同じ構成を再利用できます。

---

# 対応環境

- **Windows 10 / 11 x64**
- WASAPI
- VST3
- 48kHz推奨

通常利用ではVB-CABLEが必要です。Setup版を使えば、未導入時に自動でセットアップされます。

---

# 開発者向け

MicVSTは **C++17 + JUCE 8.0.13** で実装されています。

主な構成：

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

生成物：

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
installer\out\MicVST-Setup-1.1.1.exe
```

GitHub ActionsでもWindows上で **Build → Unit Test → Inno Setup** まで実行しています。

---

# ライセンス

MicVST本体は **GNU General Public License v3.0 (GPL-3.0)** です。

詳しくは [`LICENSE`](LICENSE) を参照してください。

- JUCE — アプリ / オーディオ / VST3ホスト基盤
- VB-CABLE — VB-Audio Softwareのサードパーティ製Donationware
- 外部VST3 — 各プラグインそれぞれのライセンスに従います
- `driver/` の実験的ドライバ — [`driver/THIRD_PARTY_NOTICES.md`](driver/THIRD_PARTY_NOTICES.md) を参照

VB-CABLEはMicVSTのGPLコードへ再ライセンスされるものではなく、独立したサードパーティソフトウェアとして扱います。
