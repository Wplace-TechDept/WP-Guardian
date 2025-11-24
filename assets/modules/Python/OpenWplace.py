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
import webbrowser
import math

def zoom_from_wh(W, H):
    a = 21.16849365
    bw = -0.45385241
    bh = -2.76763227
    
    raw = a + bw * math.log10(W) + bh * math.log10(H)
    return max(10.7, raw)   # 下限を設定

def pixelToLatLon(X : float, Y : float, mapSize : int = 2048) :
    lon : float = X / mapSize * 360.0 - 180.0
    lat_rad : float = math.atan(math.sinh(3.14159265358979323846 * (1.0 - 2.0 * Y / mapSize)))
    lat : float = lat_rad * 180.0 / 3.14159265358979323846
    return ( lat, lon )


async def on_click(ws):
    tlX = await get_variable(ws,"tile_x")
    tlY = await get_variable(ws,"tile_y")
    pxX = await get_variable(ws,"pixel_x")
    pxY = await get_variable(ws,"pixel_y")  
    width = int(await get_variable(ws,"width"))
    height = int(await get_variable(ws,"height"))
    position = pixelToLatLon(tlX + pxX / 1000 + width / 2000,tlY + pxY / 1000 + height / 2000)
    zoom = zoom_from_wh(width,height)
    lat = position[0]
    lon = position[1]
    url = "https://wplace.live/?lat="+str(lat)+"&lng="+str(lon)+"&zoom="+str(zoom)
    webbrowser.open(url)


async def recv_loop(ws):
    while True:
        msg = await ws.recv()
        data = json.loads(msg)
        update(data)
        handle_event(data)

async def main_loop(ws,win):
    while True:
        await win.send_json(ws)
        await asyncio.sleep(0.1)  # 100msごとにループ

async def main():
    url = "ws://localhost:9002"
    async with websockets.connect(url,max_size=None) as ws:
        win = Window("Wplaceで開く")
        win.buttonWidget("btn1","Wplaceで開く",partial(on_click,ws))
        
        await win.send_json(ws)

        await asyncio.gather(
            recv_loop(ws),
            main_loop(ws,win)
        )
if __name__ == "__main__":
    asyncio.run(main())