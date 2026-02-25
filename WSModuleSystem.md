# WP-Guardian WebSocket Module System Overview

このドキュメントは、WP-Guardian C++ アプリケーションと Python モジュール間の WebSocket 通信、JSON プロトコル、および内部設計の概要をまとめたものです。

## 1. 全体アーキテクチャ

WP-Guardian は、C++ アプリケーションが **WebSocket サーバー (ポート: 9002 デフォルト)** として機能し、Python スクリプト等の外部モジュールがクライアントとして接続する「サーバー・クライアント型」のアーキテクチャを採用しています。

* **リモート UI 制御**: モジュールから C++ 側の ImGui ウィンドウを構築・操作。
* **共有変数システム**: C++ 側の設定値や画像解析結果をモジュールから透過的に読み書き。
* **非同期イベント**: ボタン押下等の UI イベントを C++ からモジュールへ即時通知。

---

## 2. C++ 側主要コンポーネント

### 2.1 `Server` (WebSocket 基盤)

`websocketpp` と `asio` をベースにした非同期通信エンジン。

* **マルチクライアント**: 複数のモジュール（ブックマーク管理、Discord通知等）の同時接続を管理。

### 2.2 `ImWS` (UI リモートインターフェース)

JSON メッセージを解釈し、C++ 側の ImGui 描画命令へ変換するブリッジ。

* **プロトコル構造**:
```json
{ "target": "ui", "command": "...", "payload": { ... } }
```


* **主要コマンド**:
* `createWindow` / `deleteWindow`: ウィンドウのライフサイクル管理。
* `updateWindow`: ウィジェットの差分更新（テキストの書き換え等）。
* `createWidget` / `deleteWidget`: ウィンドウ内要素の動的な追加・削除。



### 2.3 `WSVariable` (変数共有システム)

C++ 内部のステータスを外部公開する抽象化レイヤー。

* **リクエスト形式**:
```json
{ "target": "var", "cmd": "get", "name": "変数名" }
```


* **レスポンス形式**:
```json
{ "target": "var", "cmd": "get_response", "name": "...", "success": true, "value": { ... } }
```



---

## 3. 公開変数とデータ構造

### 3.1 設定系 (`settings.temp`, `settings`)

`Settings::AppSettings` 構造体にアクセスします。

| キー | 型 | 内容 |
| --- | --- | --- |
| `tile_x`, `tile_y` | `int` | 対象タイルの座標 |
| `pixel_x`, `pixel_y` | `int` | タイル内のピクセルオフセット |
| `updateSpeed` | `float` | 画像キャプチャの頻度 |
| `imgPath` | `string` | 比較対象テンプレート画像のパス |
| `diffPattern` | `object` | 比較アルゴリズム設定 (`type`: 0=Abs, 1=Mask, 2=Overlay) |

> **Note**: `set` 操作時は、変更したいフィールドのみを含む部分的な JSON を送信することで、特定の項目のみを更新する「パッチ更新」が可能です。

### 3.2 情報・画像系 (`info`, `image.*`)

* **`info`**: 解析結果 (`totalPixels`, `diffPixels`, `diffPercent`)。
* **`image.realtime`, `image.original`, `image.diff`**:
画像データ。以下の構造を持つ辞書を返します。
```json
{ "width": 1920, "height": 1080, "data": "Base64String..." }
```
* **`image.size`**:画像サイズ単体のデータ。



---

## 4. Python サイドの実装仕様

モジュール開発を容易にするため、以下の 2 つのコアライブラリで構成されます。

### 4.1 `ImWSWindow.py` (UI 制御)

* **`Window` クラス**: 状態を保持し、変更があったウィジェットのみを `sync()` メソッドで C++ へ送信します。
* **イベントハンドラ**: `handle_event(data)` により、C++ 側でのボタンクリック等を Python の関数 (async/sync 両対応) へディスパッチします。

### 4.2 `WSVariable.py` (変数同期)

* **キャッシュ管理**: `deque` を使用し、最新の WebSocket メッセージを TTL (Time To Live) 管理で保持します。
* **非同期 Getter**: `await get_variable(ws, name)` により、WebSocket の要求/応答モデルを同期的なプログラミングモデルとして扱えます。

---

## 5. 通信シーケンスの例

1. **UI 操作**: モジュールのボタンがクリックされる。
2. **イベント通知**: C++  Python。
3. **データ取得**: Python  C++。
4. **レスポンス**: C++  Python。
5. **UI 更新**: Python  C++ (表示を更新)。

---

## 6. セットアップと構成

* **設定ファイル**: `settings.json` の `ModuleSettings` セクションでサーバーの IP/Port を管理。
