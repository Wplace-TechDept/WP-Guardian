import json
import asyncio
import websockets
import base64
import io
import requests
from PIL import Image
from functools import partial

# 自作ライブラリ
from ImWSWindow import Window, handle_event
import WSVariable
from settings_loader import load_module_settings

async def on_send_click(ws, win):
    # UIからWebhook URLを取得
    webhook_url = win.get_widget("webhook_url").value
    if not webhook_url.startswith("http"):
        print("Invalid Webhook URL")
        return

    try:
        # C++側からリアルタイム画像情報を一括取得
        # image.realtime は {'width': 1920, 'height': 1080, 'data': 'base64...'} を返す
        img_info = await WSVariable.get_variable(ws, "image.realtime")
        info = await WSVariable.get_variable(ws, "info")
        print(info)
        width = img_info['width']
        height = img_info['height']
        base64_str = img_info['data']

        # デコードして画像化
        raw_bytes = base64.b64decode(base64_str)
        # C++側がRGBAで送っている場合の処理
        img = Image.frombytes("RGBA", (width, height), raw_bytes)
        
        # Discord用にPNGへ変換
        buffered = io.BytesIO()
        img.save(buffered, format="PNG")
        buffered.seek(0)

        # Discord Webhook送信
        payload = {
            "content": f"**【現在の状態】**\n現在の差分率: {info['diffPercent']}%"
        }
        files = {
            "file": ("capture.png", buffered, "image/png")
        }
        
        # requests.postはブロッキング処理なので、本来は実行スレッドを分けるのが理想ですが
        # 簡易的にこのまま実装します
        response = requests.post(webhook_url, data=payload, files=files)
        
        if response.status_code == 204 or response.status_code == 200:
            print("Successfully sent to Discord")
        else:
            print(f"Failed to send: {response.status_code}")

    except Exception as e:
        print(f"Error in on_send_click: {e}")

async def main():
    # --- ここで設定を読み込む ---
    ip, port = load_module_settings()
    uri = f"ws://{ip}:{port}"
    print(f"Connecting to {uri}...")
    # 画像データは巨大になるため max_size=None は必須
    async with websockets.connect(uri, max_size=None) as ws:
        win = Window("Discord通知設定")
        
        # UI構築
        win.add_text("label1", "Discord Webhookを介して現在の状況を送信します")
        win.add_input_text("webhook_url", "Webhook URL", "")
        
        # ボタンイベントに現在のウィンドウ情報を渡せるよう調整
        win.add_button("btn_send", "現在の画像をDiscordへ送信", 
                       event=partial(on_send_click, ws, win))

        # 送信・受信ループ
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