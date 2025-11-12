import json
import asyncio
from collections import deque
import base64
from PIL import Image

__all__ = [
    "update",
    "get_variable",
    "set_variable",
]

latest_messages = deque(maxlen=20)

def update(data):
    global latest_messages
    latest_messages.append(data)  # 古いものは自動で消える

async def get_variable(ws, name: str, timeout: float = 3.0):
    start = asyncio.get_event_loop().time()
    await ws.send(json.dumps({"getVariable": name}))
    while True:
        for data in reversed(latest_messages):
            if data.get("type") == "variable_update" and data.get("name") == name:
                return data.get("value")
        if asyncio.get_event_loop().time() - start > timeout:
            raise TimeoutError(f"Variable '{name}' not received within {timeout}s")
        await asyncio.sleep(0.01)


async def set_variable(ws,name: str,value):
    print("setVariable")
    await ws.send(json.dumps({"setVariable": name,"value": value}))
