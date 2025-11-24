import json
import asyncio
import websockets
from ImWSWindow import *
from ImWSVariable import *
import base64
from PIL import Image
from functools import partial

async def show_image(ws,imageName):
    global webhook_url
    base64img = await get_variable(ws,imageName)
    width = int(await get_variable(ws,"width"))
    height = int(await get_variable(ws,"height"))

    # 文字列 → バイト列 → 画像化
    raw = base64.b64decode(base64img)
    img = Image.frombytes("RGBA", (width, height), raw)
    img.show()
async def recv_loop(ws):
    while True:
        msg = await ws.recv()
        data = json.loads(msg)
        update(data)
        handle_event(data)
async def main_loop(ws):
    win = windows.get("SampleWindow")
    while True:
        tlX = await get_variable(ws,"tile_x")
        tlY = await get_variable(ws,"tile_y")
        pxX = await get_variable(ws,"pixel_x")
        pxY = await get_variable(ws,"pixel_y")  
        diffpercent = await get_variable(ws,"diffPercent")
        total = await get_variable(ws,"totalPixels")
        wrongpixes = await get_variable(ws,"wrongPixels")
        win.updateWidget(textWidget("tlX","TileX="+str(tlX)))
        win.updateWidget(textWidget("tlY","TileY="+str(tlY)))
        win.updateWidget(textWidget("pxX","PixelX="+str(pxX)))
        win.updateWidget(textWidget("pxY","PixelY="+str(pxY)))
        win.updateWidget(textWidget("diffpercent","差分率="+str(diffpercent)))
        win.updateWidget(textWidget("wront/total",str(wrongpixes)+"/"+str(total)))
        await win.send_json(ws)
        await asyncio.sleep(0.1)  # 100msごとにループ
async def main():
    uri = "ws://localhost:9002"
    async with websockets.connect(uri) as ws:
        win = Window("SampleWindow")
        win.textWidget("txt1","このウィンドウはPythonからWebsocket通信を使って表示しています！")
        win.textWidget("txt2","ウィジェット表示テスト")
        win.buttonWidget("btn1","ボタン")
        win.checkboxWidget("checkbox1",True,"チェックボックス")
        win.comboBoxWidget("combobox1",1,["選択肢1","選択肢2","選択肢3"],"コンボボックス")
        win.inputTextWidget("inputtxt1","デフォルト","文字列入力")
        win.inputIntWidget("inputint1","整数入力",0)
        win.inputFloatWidget("inputfloat1","Float入力",3.14)
        win.sliderWidget("sld1","スライダー",0.5,0,1)
        win.textWidget("txt3","情報取得テスト")
        win.textWidget("tlX","")
        win.textWidget("tlY","")
        win.textWidget("pxX","")
        win.textWidget("pxY","")
        win.textWidget("diffpercent","")
        win.textWidget("wront/total","")
        win.buttonWidget("btn2","オリジナル画像を表示",partial(show_image, ws, "originalImage"))
        win.buttonWidget("btn3","リアルタイム画像を表示",partial(show_image, ws, "realtimeImage"))
        win.buttonWidget("btn4","差分画像を表示",partial(show_image, ws, "diffImage"))

        await win.send_json(ws)

        await asyncio.gather(
            recv_loop(ws),
            main_loop(ws)
        )
if __name__ == "__main__":
    asyncio.run(main())