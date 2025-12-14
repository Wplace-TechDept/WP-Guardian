import json
import asyncio
import time
from collections import deque

__all__ = [
    "update",
    "get_variable",
    "set_variable",
]

# 時間ベースで管理するメッセージキュー
latest_messages = deque()
MESSAGE_TTL = 3.0  # メッセージを保持する最大秒数

def update(data):
    """メッセージを追加し、古いものは削除する"""
    global latest_messages
    data["_timestamp"] = time.time()
    latest_messages.append(data)
    # 古いメッセージを削除
    cutoff = time.time() - MESSAGE_TTL
    while latest_messages and latest_messages[0]["_timestamp"] < cutoff:
        latest_messages.popleft()

async def get_variable(ws, name: str, timeout: float = 5.0):
    """指定した変数の値を取得。timeout秒以内に取得できなければ例外"""
    start = asyncio.get_event_loop().time()
    await ws.send(json.dumps({
        "command": "getVariable",
        "payload": {"name": name}
    }))

    while True:
        now = time.time()
        # 最新のメッセージから検索
        for data in reversed(latest_messages):
            if data.get("type") == "variable_response" and data.get("name") == name:
                # TTL以内のメッセージなら返す
                if now - data["_timestamp"] <= MESSAGE_TTL:
                    return data.get("value")
        if asyncio.get_event_loop().time() - start > timeout:
            raise TimeoutError(f"Variable '{name}' not received within {timeout}s")
        await asyncio.sleep(0.01)

async def set_variable(ws, name: str, value):
    """変数を設定"""
    print("setVariable")
    await ws.send(json.dumps({
        "command": "setVariable",
        "payload": {"name": name, "value": value}
    }))

