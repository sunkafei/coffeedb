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
    if result.text:
        return json.loads(result.text)
send({
    "operation": "clear"
})
print("Start Insert ...")
for i in range(32768):
    val = ''.join(chr(random.randint(ord('a'), ord('z'))) for i in range(32768))
    send({
        "operation": "insert",
        "data": {
            "id": i,
            "val": val
        }
    })
    print(i)
print("Start Build ...")
send({
    "operation": "build"
})
print("Start Query ...")
total = 0
for _ in range(10000):
    val = ''.join(chr(random.randint(ord('a'), ord('z'))) for i in range(5))
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
    print(_)
print(total, total / 10000)
# 1G: 12.011453866958618 0.0012011453866958618
# 2G: 13.251314878463745 0.0013251314878463744 
# 4G: 15.354285717010498 0.0015354285717010497
# 8G: 19.76681089401245 0.001976681089401245
