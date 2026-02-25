import json
import asyncio
import websockets
import base64
from PIL import Image
from functools import partial
import WSVariable
from ImWSWindow import Window, handle_event
from settings_loader import load_module_settings

async def show_image(ws, image_path):
    """C++側から指定された画像オブジェクトを取得して表示する"""
    try:
        # image.original, image.realtime, image.diff などに対応
        img_info = await WSVariable.get_variable(ws, image_path)
        
        width = img_info['width']
        height = img_info['height']
        raw_data = base64.b64decode(img_info['data'])

        img = Image.frombytes("RGBA", (width, height), raw_data)
        img.show(title=image_path)
    except Exception as e:
        print(f"画像表示エラー: {e}")
async def main_loop(ws, win):
    while True:
        try:
            cur = await WSVariable.get_variable(ws, "settings.temp")
            
            # 1. ウィジェットを取得
            widget_tlX = win.get_widget("tlX")
            if widget_tlX:
                # 2. 値を書き換える
                widget_tlX.content = f"TileX = {cur.get('tile_x')}"
                # 3. 更新対象として登録する
                win.update_widget(widget_tlX)

            # 他のウィジェットも同様に...
            widget_tlY = win.get_widget("tlY")
            if widget_tlY:
                widget_tlY.content = f"TileY = {cur.get('tile_y')}"
                win.update_widget(widget_tlY)
            widget_pxX = win.get_widget("pxX")
            if widget_pxX:
                widget_pxX.content = f"PixelX = {cur.get('pixel_x')}"
                win.update_widget(widget_pxX)
            widget_pxY = win.get_widget("pxY")
            if widget_pxY:
                widget_pxY.content = f"PixelY = {cur.get('pixel_y')}"
                win.update_widget(widget_pxY)

            # 差分情報など
            info = await WSVariable.get_variable(ws, "info")
            w_diff = win.get_widget("diffpercent")
            if w_diff:
                w_diff.content = f"差分率 = {info.get('diffPercent', 0)}%"
                win.update_widget(w_diff)
            w_wrong_total = win.get_widget("wrong/total")
            if w_wrong_total:
                w_wrong_total.content = f"{info.get('diffPixels', 0)} / {info.get('totalPixels', 0)}"
                win.update_widget(w_wrong_total)


        except Exception as e:
            print(f"Update error: {e}")
        
        # 4. これで updated_widgets が C++ 側に送信される
        await win.sync(ws)
        await asyncio.sleep(0.1)
async def main():
    # --- ここで設定を読み込む ---
    ip, port = load_module_settings()
    uri = f"ws://{ip}:{port}"
    print(f"Connecting to {uri}...")
    async with websockets.connect(uri, max_size=None) as ws:
        win = Window("SampleWindow")
        
        # --- UI初期配置 ---
        win.add_text("txt1", "PythonからWebSocketで制御中")
        win.add_text("txt2", "--- ウィジェット表示テスト ---")
        
        win.add_button("btn1", "テストボタン")
        win.add_checkbox("checkbox1", "チェックボックス", True)
        win.add_combo("combobox1", "コンボボックス", 1, ["選択肢1", "選択肢2", "選択肢3"])
        win.add_input_text("inputtxt1", "文字列入力", "デフォルト")
        win.add_input_int("inputint1", "整数入力", 0)
        win.add_slider("sld1", "スライダー", 0.5, 0.0, 1.0)
        
        win.add_text("txt3", "--- 情報取得テスト ---")
        # 監視用ラベル（最初は空）
        win.add_text("tlX", "TileX = ---")
        win.add_text("tlY", "TileY = ---")
        win.add_text("pxX", "PixelX = ---")
        win.add_text("pxY", "PixelY = ---")
        win.add_text("diffpercent", "差分率 = ---")
        win.add_text("wrong/total", "--- / ---")
        
        win.add_text("txt4", "--- 画像ビューアー ---")
        win.add_button("btn2", "オリジナル画像を表示", 
                       event=partial(show_image, ws, "image.original"))
        win.add_button("btn3", "リアルタイム画像を表示", 
                       event=partial(show_image, ws, "image.realtime"))
        win.add_button("btn4", "差分画像を表示", 
                       event=partial(show_image, ws, "image.diff"))

        # 最初の状態を送信
        await win.sync(ws)

        async def recv_loop():
            async for msg in ws:
                data = json.loads(msg)
                WSVariable.update(data)
                handle_event(data)

        # 全てのタスクを並行実行
        await asyncio.gather(
            recv_loop(),
            main_loop(ws, win)
        )

if __name__ == "__main__":
    asyncio.run(main())