import json
import asyncio
import inspect

__all__ = [
    "Window",
    "handle_event",
    "windows"
]

# ウィンドウ管理用の辞書
windows = {}

def handle_event(data):
    """
    WebSocketからのメッセージを解析し、Python側のウィジェット状態を更新・イベントを実行します。
    C++側の ImWS::render_widget 等から送信されるJSONに対応しています。
    """
    if data.get("target") != "ui" or "event" not in data:
        return
    
    win_name = data.get("window")
    widget_id = data.get("id")
    value = data.get("value")

    window = windows.get(win_name)
    if not window:
        return

    widget = window.get_widget(widget_id)
    if not widget:
        return

    # イベントタイプ別処理
    if data["event"] == "button":
        widget.click()
    elif hasattr(widget, "update_value"):
        widget.update_value(value)

# ===============================
# ウィジェット基底・実装クラス
# ===============================

class Widget:
    def __init__(self, widget_id, label):
        self.id = widget_id
        # IDのみ指定でラベルが空の場合のImGui用ハッシュ対策
        self.label = label if label else f"##{widget_id}"

class TextWidget(Widget):
    def __init__(self, widget_id, content):
        super().__init__(widget_id, None)
        self.content = content
    def to_json(self):
        return {"type": "text", "id": self.id, "content": self.content}

class ButtonWidget(Widget):
    def __init__(self, widget_id, label, event=None):
        super().__init__(widget_id, label)
        self.event = event
    def to_json(self):
        return {"type": "button", "id": self.id, "label": self.label}
    def click(self):
        if self.event:
            if inspect.iscoroutinefunction(self.event):
                asyncio.create_task(self.event())
            else:
                self.event()

class ValueWidget(Widget):
    """値を持つウィジェットの共通クラス"""
    def __init__(self, widget_id, label, value):
        super().__init__(widget_id, label)
        self.value = value
    def update_value(self, value):
        self.value = value

class SliderWidget(ValueWidget):
    def __init__(self, widget_id, label, value, min_val, max_val):
        super().__init__(widget_id, label, value)
        self.min_val = min_val
        self.max_val = max_val
    def to_json(self):
        return {"type": "slider", "id": self.id, "label": self.label, "value": self.value, "min": self.min_val, "max": self.max_val}

class CheckboxWidget(ValueWidget):
    def to_json(self):
        return {"type": "checkbox", "id": self.id, "label": self.label, "value": self.value}

class InputTextWidget(ValueWidget):
    def to_json(self):
        return {"type": "inputtext", "id": self.id, "label": self.label, "value": self.value}

class ComboBoxWidget(ValueWidget):
    def __init__(self, widget_id, label, value, items):
        super().__init__(widget_id, label, value)
        self.items = items
    def to_json(self):
        return {"type": "combobox", "id": self.id, "label": self.label, "value": self.value, "items": self.items}

class InputIntWidget(ValueWidget):
    def to_json(self):
        return {"type": "inputint", "id": self.id, "label": self.label, "value": self.value}

class InputFloatWidget(ValueWidget):
    def to_json(self):
        return {"type": "inputfloat", "id": self.id, "label": self.label, "value": self.value}

# ===============================
# ウィンドウクラス
# ===============================

class Window:
    def __init__(self, name):
        self.name = name
        self.is_first = True
        self.widgets = []           # C++側に反映済みのウィジェット
        self.new_widgets = []       # 次回同期で作成するウィジェット
        self.updated_widgets = []   # 次回同期で更新するウィジェット
        self.deleted_widgets = []   # 次回同期で削除するID
        self.marked_for_delete = False
        windows[self.name] = self

    def get_widget(self, widget_id):
        return next((w for w in self.widgets if w.id == widget_id), None)

    # --- ウィジェット追加メソッド群 ---
    def add_text(self, id, content):
        w = TextWidget(id, content); self.new_widgets.append(w); return w
    def add_button(self, id, label, event=None):
        w = ButtonWidget(id, label, event); self.new_widgets.append(w); return w
    def add_slider(self, id, label, val, min_v, max_v):
        w = SliderWidget(id, label, val, min_v, max_v); self.new_widgets.append(w); return w
    def add_checkbox(self, id, label, val):
        w = CheckboxWidget(id, label, val); self.new_widgets.append(w); return w
    def add_input_text(self, id, label, val):
        w = InputTextWidget(id, label, val); self.new_widgets.append(w); return w
    def add_combo(self, id, label, val, items):
        w = ComboBoxWidget(id, label, val, items); self.new_widgets.append(w); return w
    def add_input_int(self, id, label, val):
        w = InputIntWidget(id, label, val); self.new_widgets.append(w); return w
    def add_input_float(self, id, label, val):
        w = InputFloatWidget(id, label, val); self.new_widgets.append(w); return w

    def update_widget(self, widget):
        """既存ウィジェットの変更を同期対象に加える"""
        if any(w.id == widget.id for w in self.widgets):
            if widget not in self.updated_widgets:
                self.updated_widgets.append(widget)

    def delete_widget(self, widget_id):
        self.deleted_widgets.append(widget_id)

    def delete_window(self):
        self.marked_for_delete = True

    def _build_command(self, cmd, payload):
        """ImWS.cpp が解釈可能な形式にラップする"""
        return {"target": "ui", "command": cmd, "payload": payload}

    async def sync(self, ws):
        """
        現在の変更差分をWebSocket経由でC++側に送信します。
        ImWS::on_message 内の各コマンドに対応しています。
        """
        messages = []

        if self.marked_for_delete:
            messages.append(self._build_command("deleteWindow", {"title": self.name}))
        else:
            # 新規作成
            if self.new_widgets:
                cmd = "createWindow" if self.is_first else "createWidget"
                self.is_first = False
                messages.append(self._build_command(cmd, {
                    "title": self.name,
                    "widgets": [w.to_json() for w in self.new_widgets]
                }))
                self.widgets += self.new_widgets
                self.new_widgets = []

            # 更新
            if self.updated_widgets:
                messages.append(self._build_command("updateWindow", {
                    "title": self.name,
                    "widgets": [w.to_json() for w in self.updated_widgets]
                }))
                self.updated_widgets = []

            # ウィジェット削除
            if self.deleted_widgets:
                messages.append(self._build_command("deleteWidget", {
                    "title": self.name,
                    "widgets": [{"id": wid} for wid in self.deleted_widgets]
                }))
                self.widgets = [w for w in self.widgets if w.id not in self.deleted_widgets]
                self.deleted_widgets = []

        for msg in messages:
            await ws.send(json.dumps(msg))