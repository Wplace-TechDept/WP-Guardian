import json
import asyncio
import websockets
from ImWSWindow import *
from ImWSVariable import *
import base64
from PIL import Image
from functools import partial
import requests
import io

webhook_url = ""

async def on_click(ws):
    global webhook_url
    base64img = await get_variable(ws,"realtimeImage")
    width = int(await get_variable(ws,"width"))
    height = int(await get_variable(ws,"height"))

    # 文字列 → バイト列 → 画像化
    raw = base64.b64decode(base64img)
    img = Image.frombytes("RGBA", (width, height), raw)
    diffPercent = str(await get_variable(ws,"diffPercent"))
    buffered = io.BytesIO()
    img.save(buffered, format="PNG")
    buffered.seek(0)
    data = {
        "content": "**現在の状態**\n差分率:"+diffPercent+"\n"
    }

    files = {
        "file": ("diff.png", buffered, "image/png")
    }
    response = requests.post(webhook_url, data=data, files=files)

async def recv_loop(ws):
    while True:
        msg = await ws.recv()
        data = json.loads(msg)
        update(data)
        handle_event(data)
async def main_loop(ws,win):
    global webhook_url
    while True:
        webhook_url = win.getWidget("inputtxt1").value
        await win.send_json(ws)
        await asyncio.sleep(0.1)  # 100msごとにループ
async def main():
    uri = "ws://localhost:9002"
    async with websockets.connect(uri) as ws:
        win = Window("DiscordBot")
        win.inputTextWidget("inputtxt1","https://","WebhookURL")
        win.buttonWidget("btn1","現在の状態を送信",partial(on_click, ws))
        
        await win.send_json(ws)

        await asyncio.gather(
            recv_loop(ws),
            main_loop(ws,win)
        )
if __name__ == "__main__":
    asyncio.run(main())