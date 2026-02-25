import json
import asyncio
import time
from collections import deque

latest_messages = deque()
MESSAGE_TTL = 3.0

def update(data):
    global latest_messages
    data["_timestamp"] = time.time()
    latest_messages.append(data)
    
    cutoff = time.time() - MESSAGE_TTL
    while latest_messages and latest_messages[0]["_timestamp"] < cutoff:
        latest_messages.popleft()

async def get_variable(ws, name: str, timeout: float = 5.0):
    start_time = asyncio.get_event_loop().time()
    
    # 送信
    payload = {
        "target": "var",
        "cmd": "get",
        "name": name
    }
    await ws.send(json.dumps(payload))

    while True:
        now = time.time()
        for data in reversed(latest_messages):
            # C++側の返信 cmd が 'get_response' であることを確認
            res_name = data.get("name")
            res_cmd = data.get("cmd")
            
            if res_cmd == "get_response" and res_name == name:
                return data.get("value")
        
        if asyncio.get_event_loop().time() - start_time > timeout:
            raise TimeoutError(f"Variable '{name}' not received within {timeout}s")
        
        await asyncio.sleep(0.01)

async def set_variable(ws, name: str, value):
    payload = {
        "target": "var",
        "cmd": "set",
        "name": name,
        "value": value
    }
    await ws.send(json.dumps(payload))

async def trigger_update(ws):
    await set_variable(ws, "update", True)