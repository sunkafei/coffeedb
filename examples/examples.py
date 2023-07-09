import requests
import json
url = "http://127.0.0.1:14920/coffeedb"
def send(data):
    result = requests.post(url, json.dumps(data))
    print(result)
send({
    "operation": "undf"
})
