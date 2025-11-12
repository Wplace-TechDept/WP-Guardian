import json
import asyncio
import inspect

__all__ = [
    "Window",
    "textWidget",
    "buttonWidget",
    "sliderWidget",
    "checkboxWidget",
    "inputTextWidget",
    "comboBoxWidget",
    "inputIntWidget",
    "inputFloatWidget",
    "handle_event",
    "windows"
]

def handle_event(data):
    # print("Received:", data)

    if "event" not in data or "window" not in data or "id" not in data:
        return
    
    event_type = data["event"]
    win_name = data["window"]
    widget_id = data["id"]
    value = data.get("value")

    window = windows.get(win_name)
    if not window:
        print(f"[Warning] Window '{win_name}' not found.")
        return

    widget = window.getWidget(widget_id)
    if not widget:
        print(f"[Warning] Widget '{widget_id}' not found in window '{win_name}'.")
        return

    # --- イベントタイプ別処理 ---
    if event_type == "button":
        print(f"Button '{widget_id}' clicked.")
        widget.click()
        
    elif event_type in ("slider", "checkbox", "inputtext", "combobox", "inputint", "inputfloat"):
        # 値を持つウィジェット
        if hasattr(widget, "update_value"):
            widget.update_value(value)
        elif hasattr(widget, "value"):
            widget.value = value
        print(f"Updated {event_type} '{widget_id}' to {value}")
    else:
        print(f"[Info] Unknown event type: {event_type}")

# ウィンドウ管理用の辞書
windows = {}

# ===============================
# ウィンドウクラス
# ===============================
class Window:
    def __init__(self, name):
        self.name = name
        self.is_first = True
        self.widgets = []
        self.new_widgets = []
        self.updated_widgets = []
        self.deleted_widgets = []
        self.marked_for_delete = False
        windows[self.name] = self

    def getWidget(self, widget_id):
        """IDからウィジェットを取得（存在しなければ None）"""
        return next((w for w in self.widgets if w.id == widget_id), None)



    # --- 各ウィジェット追加メソッド ---
    def textWidget(self, ID, CONTEXT):
        w = textWidget(ID, CONTEXT)
        self.new_widgets.append(w)
        return w

    def buttonWidget(self, ID, LABEL, EVENT=None):
        w = buttonWidget(ID, LABEL, EVENT)
        self.new_widgets.append(w)
        return w

    def sliderWidget(self, ID, LABEL, VALUE, MIN_VAL, MAX_VAL):
        w = sliderWidget(ID, LABEL, VALUE, MIN_VAL, MAX_VAL)
        self.new_widgets.append(w)
        return w

    def checkboxWidget(self, ID, VALUE, LABEL):
        w = checkboxWidget(ID, VALUE, LABEL)
        self.new_widgets.append(w)
        return w

    def inputTextWidget(self, ID, VALUE, LABEL):
        w = inputTextWidget(ID, VALUE, LABEL)
        self.new_widgets.append(w)
        return w

    def comboBoxWidget(self, ID, VALUE, ITEMS, LABEL):
        w = comboBoxWidget(ID, VALUE, ITEMS, LABEL)
        self.new_widgets.append(w)
        return w

    def inputIntWidget(self, ID, LABEL, VALUE):
        w = inputIntWidget(ID, LABEL, VALUE)
        self.new_widgets.append(w)
        return w

    def inputFloatWidget(self, ID, LABEL, VALUE):
        w = inputFloatWidget(ID, LABEL, VALUE)
        self.new_widgets.append(w)
        return w

    # --- 更新対象を追加 ---
    def updateWidget(self, widget):
        # widget.id がすでに self.widgets にある場合のみ更新対象に追加
        if any(w.id == widget.id for w in self.widgets):
            self.updated_widgets.append(widget)
        else:
            raise ValueError(f"Widget ID '{widget.id}' does not exist. Use createWidget to add it.")


    # --- 削除対象を追加 ---
    def deleteWidget(self, widget_id):
        self.deleted_widgets.append(widget_id)

    # --- ウィンドウ削除 ---
    def deleteWindow(self):
        self.marked_for_delete = True

    # --- JSON生成 ---
    def to_json(self):
        msgs = []

        # ウィンドウ削除（これだけは最優先でreturnしてOK）
        if self.marked_for_delete:
            msgs.append(json.dumps({"deleteWindow": self.name}, indent=4))
            return msgs

        # --- 新規ウィジェット ---
        if self.new_widgets:
            msg_type = "createWindow" if self.is_first else "createWidget"
            self.is_first = False
            msgs.append(json.dumps({
                msg_type: self.name,
                "widgets": [w.to_json() for w in self.new_widgets]
            }, indent=4))
            self.widgets += self.new_widgets
            self.new_widgets = []

        # --- 更新 ---
        if self.updated_widgets:
            for uw in self.updated_widgets:
                for i, w in enumerate(self.widgets):
                    if w.id == uw.id:
                        for attr, val in uw.__dict__.items():
                            setattr(self.widgets[i], attr, val)
                        break
            msgs.append(json.dumps({
                "updateWindow": self.name,
                "widgets": [w.to_json() for w in self.updated_widgets]
            }, indent=4))
            self.updated_widgets = []

        # --- 削除 ---
        if self.deleted_widgets:
            msgs.append(json.dumps({
                "deleteWidget": self.name,
                "widgets": [{"id": wid} for wid in self.deleted_widgets]
            }, indent=4))
            self.widgets = [w for w in self.widgets if w.id not in self.deleted_widgets]
            self.deleted_widgets = []

        return msgs
    async def send_json(self,ws):
        msgs = self.to_json()
        if msgs:
            for msg in msgs:
                await ws.send(msg)



# ===============================
# 各ウィジェットクラス
# ===============================
class textWidget:
    def __init__(self, ID, CONTEXT):
        self.id = ID
        self.context = CONTEXT

    def to_json(self):
        return {"type": "text", "id": self.id, "content": self.context}


class buttonWidget:
    def __init__(self, ID, LABEL, EVENT):
        self.id = ID
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL
        self.event = EVENT

    def to_json(self):
        return {"type": "button", "id": self.id, "label": self.label}
    
    def click(self):
        if self.event:
            # 関数が非同期かどうかチェック
            if inspect.iscoroutinefunction(self.event):
                asyncio.create_task(self.event())  # 非同期ならタスクとして実行
            else:
                self.event()  # 同期ならそのまま呼ぶ


class sliderWidget:
    def __init__(self, ID, LABEL, VALUE, MIN_VAL, MAX_VAL):
        self.id = ID
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL
        self.value = VALUE
        self.min_val = MIN_VAL
        self.max_val = MAX_VAL

    def to_json(self):
        return {
            "type": "slider",
            "id": self.id,
            "value": self.value,
            "label": self.label,
            "min": self.min_val,
            "max": self.max_val
        }

    def update_value(self,VALUE):
        self.value = VALUE


class checkboxWidget:
    def __init__(self, ID, VALUE, LABEL):
        self.id = ID
        self.value = VALUE
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL

    def to_json(self):
        return {"type": "checkbox", "id": self.id, "value": self.value, "label": self.label}
    def update_value(self,VALUE):
        self.value = VALUE


class inputTextWidget:
    def __init__(self, ID, VALUE, LABEL):
        self.id = ID
        self.value = VALUE
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL

    def to_json(self):
        return {"type": "inputtext", "id": self.id, "value": self.value, "label": self.label}
    def update_value(self,VALUE):
        self.value = VALUE


class comboBoxWidget:
    def __init__(self, ID, VALUE, ITEMS, LABEL):
        self.id = ID
        self.value = VALUE
        self.items = ITEMS
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL

    def to_json(self):
        return {"type": "combobox", "id": self.id, "value": self.value, "items": self.items, "label": self.label}
    def update_value(self,VALUE):
        self.value = VALUE

class inputIntWidget:
    def __init__(self, ID, LABEL, VALUE):
        self.id = ID
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL
        self.value = VALUE

    def to_json(self):
        return {"type": "inputint", "id": self.id, "value": self.value, "label": self.label}
    def update_value(self,VALUE):
        self.value = VALUE

class inputFloatWidget:
    def __init__(self, ID, LABEL, VALUE):
        self.id = ID
        if not LABEL:
            self.label = f"##{ID}"
        else:
            self.label = LABEL
        self.value = VALUE

    def to_json(self):
        return {"type": "inputfloat", "id": self.id, "value": self.value, "label": self.label}
    def update_value(self,VALUE):
        self.value = VALUE