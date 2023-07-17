''' 
CoffeeDB needs to be started before running this script.
'''
# pip install requests
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
        "secret": "3010103"
    }
})
send({
    "operation": "insert",
    "data": {
        "number": 234,
        "name": "yulemao",
        "position": 1.7724,
        "secret": "301022"
    }
})
send({
    "operation": "build"
})
send({
    "operation": "query",
    "constraints": {
        "number": "[100,200]"
    }
})
send({
    "operation": "query",
    "constraints": {
        "number": "[100,900]"
    },
    "fields": ["name"]
})
send({
    "operation": "query",
    "constraints": {
        "secret": "010"
    }
})
# Both conditions about "secret" and "number" must be met.
send({
    "operation": "query",
    "constraints": {
        "secret": "010",
        "number": "[0,900]"
    },
    "fields": ["name", "secret"],
    "highlight": ["<b>", "</b>"],
    "span": "[0,1)"
})