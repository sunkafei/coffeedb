import requests
import json
import random
from time import time
total = 0
url = "http://127.0.0.1:14920/coffeedb"
def send(data):
    global total
    data = json.dumps(data, indent=4)
    now = time()
    result = requests.post(url, data)
    total += time() - now
    return json.loads(result.text)
def count(str, sub):
    ret = 0
    for i in range(0, len(str) - len(sub) + 1):
        if str[i:i+len(sub)] == sub:
            ret += 1
    return ret
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
for _ in range(100):
    val = ''.join(chr(random.randint(ord('a'), ord('z'))) for i in range(3))
    result = send({
        "operation": "query",
        "constraints": {
            "val": val
        },
        "fields": [
            "id",
            "$correlation"
        ]
    })
    cnt = {}
    for L in result:
        cnt[int(L["id"])] = int(L["$correlation"])
    for i in range(5000):
        assert count(vals[i], val) == cnt.get(i, 0)
    print(_)