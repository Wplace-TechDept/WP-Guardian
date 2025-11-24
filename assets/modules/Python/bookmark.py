import json
import asyncio
import websockets
from ImWSWindow import *
from ImWSVariable import *
from functools import partial
import os

BOOKMARK_FILE = os.path.join(os.path.dirname(__file__), "bookmarks.json")

class bookmark:
    def __init__(self, Name:str, TileX:int, TileY:int, PixelX:int, PixelY:int, Path:str):
        self.Name = Name
        self.TileX = TileX
        self.TileY = TileY
        self.PixelX = PixelX
        self.PixelY = PixelY
        self.Path = Path

    def to_dict(self):
        return {
            "Name": self.Name,
            "TileX": self.TileX,
            "TileY": self.TileY,
            "PixelX": self.PixelX,
            "PixelY": self.PixelY,
            "Path": self.Path
        }

    @classmethod
    def from_dict(cls, d):
        return cls(d["Name"], d["TileX"], d["TileY"], d["PixelX"], d["PixelY"], d["Path"])

bookmarkList = [bookmark("選択してください",0,0,0,0,"")]

def save_bookmarks():
    with open(BOOKMARK_FILE, "w", encoding="utf-8") as f:
        json.dump([b.to_dict() for b in bookmarkList], f, ensure_ascii=False, indent=2)

def load_bookmarks():
    global bookmarkList
    path = os.path.join(os.path.dirname(__file__), "bookmarks.json")
    if os.path.exists(path):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
                bookmarkList = [bookmark.from_dict(d) for d in data]
        except Exception as e:
            print("JSON 読み込み失敗:", e)
            bookmarkList = [bookmark("選択してください",0,0,0,0,"")]
    else:
        bookmarkList = [bookmark("選択してください",0,0,0,0,"")]



def createMainPage(ws,win:Window):
    names = list(map(lambda n: n.Name, bookmarkList))
    win.textWidget("txt1","画像と場所をブックマークとして保存できます")
    win.comboBoxWidget("combo1",0,names,"##ブックマーク")
    win.buttonWidget("btn1","ブックマークを復元",partial(restore,ws,win))
    win.buttonWidget("btn2","ブックマークを編集",partial(editPage,ws,win))
    win.buttonWidget("btn8","ブックマークを削除",partial(deletePage,ws,win))
    win.buttonWidget("btn3","現在の状態を保存",partial(savePage,ws,win))


def deletePage(ws,win:Window):
    index = win.getWidget("combo1").value
    if index != 0:
        index = win.getWidget("combo1").value
        deleteAllWidget(win)
        win.textWidget("txt2",'ブックマーク"'+bookmarkList[index].Name+'"を削除しますか？')
        win.buttonWidget("btn9","削除する",partial(delete,ws,win,index))
        win.buttonWidget("btn10","戻る",partial(Return,ws,win))

def delete(ws,win:Window,index):
    del bookmarkList[index]
    save_bookmarks()
    Return(ws,win)


def editPage(ws,win:Window):
    index = win.getWidget("combo1").value
    if index != 0:
        index = win.getWidget("combo1").value
        deleteAllWidget(win)
        win.inputTextWidget("inputtxt2",bookmarkList[index].Name,"名前")
        win.inputIntWidget("inputint1","TileX",bookmarkList[index].TileX)
        win.inputIntWidget("inputint2","TileY",bookmarkList[index].TileY)
        win.inputIntWidget("inputint3","PixelX",bookmarkList[index].PixelX)
        win.inputIntWidget("inputint4","PixelY",bookmarkList[index].PixelY)
        win.inputTextWidget("inputtxt3",bookmarkList[index].Path,"Path")
        win.buttonWidget("btn6","保存",partial(edit,ws,win,index))
        win.buttonWidget("btn7","戻る",partial(Return,ws,win))

def edit(ws,win:Window,index):
    Name = win.getWidget("inputtxt2").value
    TlX = win.getWidget("inputint1").value
    TlY = win.getWidget("inputint2").value
    PxX = win.getWidget("inputint3").value
    PxY = win.getWidget("inputint4").value
    Path = win.getWidget("inputtxt3").value
    bookmarkList[index] = bookmark(Name,TlX,TlY,PxX,PxY,Path)
    save_bookmarks()
    Return(ws,win)

def deleteAllWidget(win:Window):
    for w in win.widgets:
        id = w.id
        win.deleteWidget(id)

def Return(ws,win:Window):
    deleteAllWidget(win)
    createMainPage(ws,win)

async def restore(ws,win:Window):
    index = win.getWidget("combo1").value
    if index != 0:
        index = win.getWidget("combo1").value
        TlX = bookmarkList[index].TileX
        TlY = bookmarkList[index].TileY
        PxX = bookmarkList[index].PixelX
        PxY = bookmarkList[index].PixelY
        Path = bookmarkList[index].Path
        await set_variable(ws,"tmpTile_x",TlX)
        await set_variable(ws,"tmpTile_y",TlY)
        await set_variable(ws,"tmpPixel_x",PxX)
        await set_variable(ws,"tmpPixel_y",PxY)
        await set_variable(ws,"tmpPath",Path)
        await set_variable(ws,"update",True)
    

def save(ws,win:Window,TlX,TlY,PxX,PxY,Path):
    Name = win.getWidget("inputtxt1").value
    bookmarkList.append(bookmark(Name,TlX,TlY,PxX,PxY,Path))
    print("Saved",Name,TlX,TlY,PxX,PxY,Path)
    save_bookmarks()
    Return(ws,win)

async def savePage(ws,win:Window):
    TlX = await get_variable(ws,"tile_x")
    TlY = await get_variable(ws,"tile_y")
    PxX = await get_variable(ws,"pixel_x")
    PxY = await get_variable(ws,"pixel_y")
    Path = await get_variable(ws,"path")
    deleteAllWidget(win)
    win.inputTextWidget("inputtxt1","","名前")
    win.buttonWidget("btn4","保存",partial(save,ws,win,TlX,TlY,PxX,PxY,Path))
    win.buttonWidget("btn5","戻る",partial(Return,ws,win))






    

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
    load_bookmarks()
    url = "ws://localhost:9002"
    async with websockets.connect(url,max_size=None) as ws:
        win = Window("ブックマーク")
        createMainPage(ws,win)


        await win.send_json(ws)

        await asyncio.gather(
            recv_loop(ws),
            main_loop(ws,win)
        )
if __name__ == "__main__":
    asyncio.run(main())