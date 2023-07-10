# CoffeeDB needs to be started before running this script.
import requests
import json
url = "http://127.0.0.1:14920/coffeedb"
def send(data):
    data = json.dumps(data, indent=4)
    result = requests.post(url, data)
    print(data)
    print("\033[1;32m=>", result.text, "\033[0m", sep='')
    assert result.status_code == 200
send({
    "operation": "insert",
    "data": {
        "number": 123,
        "name": "sunkafei",
        "secret": "01010"
    }
})
send({
    "operation": "insert",
    "data": {
        "number": 234,
        "name": "yulemao",
        "position": 1.7724,
        "secret": "010"
    }
})
send({
    "operation": "build",
})
send({
    "operation": "query",
    "data": {
        "number": "[100,200]"
    }
})
send({
    "operation": "query",
    "data": {
        "number": "[100,900]"
    },
    "select": ["name"]
})
send({
    "operation": "query",
    "data": {
        "secret": "010"
    }
})
send({
    "operation": "query",
    "data": {
        "secret": "010",
        "number": "[200,900]"
    },
    "select": ["name"]
})