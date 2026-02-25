import json
import asyncio
import websockets
import math
import webbrowser
from functools import partial

# 自作ライブラリ
from ImWSWindow import Window, handle_event
import WSVariable
from settings_loader import load_module_settings

# --- 座標・ズーム計算ロジック ---

def zoom_from_wh(W, H):
    a = 21.16849365
    bw = -0.45385241
    bh = -2.76763227
    raw = a + bw * math.log10(W) + bh * math.log10(H)
    return max(10.7, raw)

def pixel_to_lat_lon(X: float, Y: float, map_size: int = 2048):
    # メルカトル投影法の逆計算
    lon = X / map_size * 360.0 - 180.0
    lat_rad = math.atan(math.sinh(math.pi * (1.0 - 2.0 * Y / map_size)))
    lat = lat_rad * 180.0 / math.pi
    return (lat, lon)

# --- イベントハンドラ ---

async def on_open_wplace(ws):
    try:
        # C++側から現在のステータスを一括取得
        cur = await WSVariable.get_variable(ws, "settings.temp")
        size = await WSVariable.get_variable(ws, "image.size")
        # 必要な値を安全に取得
        tx = cur.get('tile_x', 0)
        ty = cur.get('tile_y', 0)
        px = cur.get('pixel_x', 0)
        py = cur.get('pixel_y', 0)
        w = size.get('width', 1)
        h = size.get('height', 1)

        # 座標計算 (タイルの左上座標 + ピクセルオフセット + 中心点補正)
        # 2048は標準的なタイリングサイズに基づく計算
        target_x = tx + px / 1000 + w / 2000
        target_y = ty + py / 1000 + h / 2000
        
        lat, lon = pixel_to_lat_lon(target_x, target_y)
        zoom = zoom_from_wh(w, h)

        # URL生成とブラウザ起動
        url = f"https://wplace.live/?lat={lat:.7f}&lng={lon:.7f}&zoom={zoom:.2f}"
        print(f"Opening Wplace: {url}")
        webbrowser.open(url)

    except TimeoutError:
        print("C++側からのデータ取得に失敗しました (Wplace)")
    except Exception as e:
        print(f"Error in Wplace calculation: {e}")

# --- メイン処理 ---

async def main():
    # --- ここで設定を読み込む ---
    ip, port = load_module_settings()
    uri = f"ws://{ip}:{port}"
    print(f"Connecting to {uri}...")
    async with websockets.connect(uri) as ws:
        win = Window("Wplaceで開く")
        
        win.add_text("info", "現在の表示位置を Wplace.live で開きます")
        win.add_button("btn_wplace", "Wplace で開く", event=partial(on_open_wplace, ws))
        
        async def send_loop():
            while True:
                await win.sync(ws)
                await asyncio.sleep(0.1)

        async def recv_loop():
            async for msg in ws:
                data = json.loads(msg)
                WSVariable.update(data)
                handle_event(data)

        await asyncio.gather(send_loop(), recv_loop())

if __name__ == "__main__":
    asyncio.run(main())