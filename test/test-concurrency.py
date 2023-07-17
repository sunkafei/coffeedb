import requests
import json
import random
import threading
url = "http://127.0.0.1:14920/coffeedb"
def send(data):
    global total
    data = json.dumps(data, indent=4)
    result = requests.post(url, data)
    # result.text
def insert():
    val = ''.join(chr(random.randint(ord('a'), ord('z'))) for i in range(256))
    send({
        "operation": "insert",
        "data": {
            "id": random.randint(1, 1000000),
            "val": val
        }
    })
def build():
    send({
        "operation": "build"
    })
def query():
    chars = [chr(ord('a') + i) for i in range(26)]
    random.shuffle(chars)
    val = ''.join(chars)
    val = [val[i:i+4] for i in range(0, 20, 4)]
    send({
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
def worker():
    for i in range(512):
        random.choice([insert, build, query])()
send({
    "operation": "clear"
})
print("Start Insert ...")
for i in range(256):
    insert()
print("Test Concurrency ...")
for i in range(7):
    thread = threading.Thread(target=worker)
    thread.start()
worker()
