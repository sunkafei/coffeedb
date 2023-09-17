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
'''send({
    "operation": "clear"
})
print("Start Insert ...")
for i in range(10000):
    val = ''.join(chr(random.randint(ord('a'), ord('z'))) for i in range(10000))
    send({
        "operation": "insert",
        "data": {
            "id": i,
            "val": val
        }
    })
    print(i)
print("Start Build ...")'''
total = 0
send({
    "operation": "build"
})
print(total)