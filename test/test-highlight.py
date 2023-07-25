import requests
import json
import random
from time import time
url = "http://127.0.0.1:14920/coffeedb"
def send(data):
    global total
    data = json.dumps(data, indent=4)
    result = requests.post(url, data)
    return json.loads(result.text)
send({
    "operation": "clear"
})
vals = []
print("Start Insert ...")
for i in range(5000):
    val = ''.join(chr(random.randint(ord('a'), ord('z'))) for i in range(5000))
    vals.append(val)
    send({
        "operation": "insert",
        "data": {
            "id": i,
            "val": val
        }
    })
print("Start Build ...")
send({
    "operation": "build"
})
print("Start Query ...")
chars = [chr(ord('a') + i) for i in range(26)]
for _ in range(100):
    random.shuffle(chars)
    val = ''.join(chars)
    val = [val[i:i+4] for i in range(0, 20, 4)]
    feedback = send({
        "operation": "query",
        "constraints": {
            "val": val
        },
        "fields": [
            "id",
            "val"
        ],
        "highlight": [
            "<b>",
            "</b>"
        ]
    })
    result = {}
    for L in feedback:
        result[int(L["id"])] = L["val"]
    answer = {}
    for i in range(len(vals)):
        text = vals[i]
        for keyword in val:
            text = text.replace(keyword, f"<b>{keyword}</b>")
        if text != vals[i]:
            answer[i] = text
    assert result == answer