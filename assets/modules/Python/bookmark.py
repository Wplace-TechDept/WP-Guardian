import json
import asyncio
import websockets
import os
from functools import partial

# 自作ライブラリのインポート
from ImWSWindow import Window, handle_event
import WSVariable
from settings_loader import load_module_settings

BOOKMARK_FILE = os.path.join(os.path.dirname(__file__), "bookmarks.json")

class Bookmark:
    def __init__(self, name, tx, ty, px, py, path):
        self.name = name
        self.tx = tx; self.ty = ty
        self.px = px; self.py = py
        self.path = path

    def to_dict(self):
        return {"Name": self.name, "TileX": self.tx, "TileY": self.ty, 
                "PixelX": self.px, "PixelY": self.py, "Path": self.path}

    @classmethod
    def from_dict(cls, d):
        return cls(d["Name"], d["TileX"], d["TileY"], d["PixelX"], d["PixelY"], d["Path"])

bookmarkList = [Bookmark("選択してください", 0, 0, 0, 0, "")]

def save_bookmarks():
    with open(BOOKMARK_FILE, "w", encoding="utf-8") as f:
        json.dump([b.to_dict() for b in bookmarkList], f, ensure_ascii=False, indent=2)

def load_bookmarks():
    global bookmarkList
    if os.path.exists(BOOKMARK_FILE):
        try:
            with open(BOOKMARK_FILE, "r", encoding="utf-8") as f:
                data = json.load(f)
                bookmarkList = [Bookmark.from_dict(d) for d in data]
        except:
            bookmarkList = [Bookmark("選択してください", 0, 0, 0, 0, "")]

def clear_and_sync(win):
    ids = [w.id for w in win.widgets] + [w.id for w in win.new_widgets]
    for wid in ids:
        win.delete_widget(wid)

def create_main_page(ws, win):
    clear_and_sync(win)
    names = [b.name for b in bookmarkList]
    win.add_text("txt1", "ブックマークを選択して復元・編集・削除ができます")
    win.add_combo("combo1", "##ブックマークリスト", 0, names)
    win.add_button("btn1", "ブックマークを復元", event=partial(restore_action, ws, win))
    win.add_button("btn2", "編集", event=partial(edit_page, ws, win))
    win.add_button("btn3", "削除", event=partial(delete_page, ws, win))
    win.add_button("btn4", "現在の状態を新規保存", event=partial(save_page_request, ws, win))

async def restore_action(ws, win):
    idx = win.get_widget("combo1").value
    if idx <= 0: return
    b = bookmarkList[idx]
    
    # C++側の構造に合わせたキーで送信
    await WSVariable.set_variable(ws, "settings.temp", {
        "tile_x": b.tx, "tile_y": b.ty,
        "pixel_x": b.px, "pixel_y": b.py,
        "imgPath": b.path
    })
    await WSVariable.trigger_update(ws)

async def save_page_request(ws, win):
    try:
        cur = await WSVariable.get_variable(ws, "settings.temp")
        
        clear_and_sync(win)
        win.add_input_text("in_name", "保存名", "")
        
        async def do_save():
            name = win.get_widget("in_name").value
            new_b = Bookmark(
                name, 
                cur.get('tile_x', 0), cur.get('tile_y', 0), 
                cur.get('pixel_x', 0), cur.get('pixel_y', 0), 
                cur.get('imgPath', "")
            )
            bookmarkList.append(new_b)
            save_bookmarks()
            create_main_page(ws, win)

        win.add_button("btn_do_save", "保存実行", event=do_save)
        win.add_button("btn_back", "戻る", event=lambda: create_main_page(ws, win))
    except TimeoutError:
        print("C++側からのデータ取得に失敗しました")

def edit_page(ws, win):
    idx = win.get_widget("combo1").value
    if idx <= 0: return
    b = bookmarkList[idx]

    clear_and_sync(win)
    win.add_text("ed_info", f"編集: {b.name}")
    win.add_input_text("ed_name", "名前", b.name)
    win.add_input_int("ed_tx", "TileX", b.tx)
    win.add_input_int("ed_ty", "TileY", b.ty)
    win.add_input_int("ed_px", "PixelX", b.px) # PixelX 追加
    win.add_input_int("ed_py", "PixelY", b.py) # PixelY 追加
    win.add_input_text("ed_path", "Path", b.path)

    async def do_edit():
        # ウィジェットから値を取得してブックマークを更新
        b.name = win.get_widget("ed_name").value
        b.tx = win.get_widget("ed_tx").value
        b.ty = win.get_widget("ed_ty").value
        b.px = win.get_widget("ed_px").value
        b.py = win.get_widget("ed_py").value
        b.path = win.get_widget("ed_path").value
        
        save_bookmarks()
        create_main_page(ws, win)

    win.add_button("btn_save", "上書き保存", event=do_edit)
    win.add_button("btn_back", "キャンセル", event=lambda: create_main_page(ws, win))

def delete_page(ws, win):
    idx = win.get_widget("combo1").value
    if idx <= 0: return
    
    clear_and_sync(win)
    win.add_text("warn", f"ブックマーク '{bookmarkList[idx].name}' を削除しますか？")
    
    async def do_delete():
        del bookmarkList[idx]
        save_bookmarks()
        create_main_page(ws, win)

    win.add_button("btn_del", "はい、削除します", event=do_delete)
    win.add_button("btn_cancel", "キャンセル", event=lambda: create_main_page(ws, win))

async def main():
    # --- ここで設定を読み込む ---
    ip, port = load_module_settings()
    uri = f"ws://{ip}:{port}"
    print(f"Connecting to {uri}...")
    async with websockets.connect(uri) as ws:
        win = Window("ブックマーク管理")
        create_main_page(ws, win)

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