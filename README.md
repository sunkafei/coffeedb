![](docs/logo.png)
![](https://github.com/sunkafei/coffeedb/actions/workflows/main.yml/badge.svg?event=push)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/sunkafei/coffeedb/main/LICENSE.MIT)
[![GitHub Downloads](https://img.shields.io/github/downloads/sunkafei/coffeedb/total)](https://github.com/sunkafei/coffeedb/releases)
[![GitHub stars](https://img.shields.io/github/stars/sunkafei/coffeedb)](https://github.com/sunkafei/coffeedb/stargazers)

**CoffeeDB** is an out-of-the-box string/keyword search database. CoffeeDB builds indexes for numeric data and generalized suffix automata for text data to speed up query operations. Unlike most existing databases, CoffeeDB maintains these data structures in memory. On the one hand, this significantly improves the query speed, on the other hand, it also limits its ability to handle very large-scale data. If the data you need to retrieve does not exceed tens of gigabytes, then CoffeeDB may be the best practice you want.

- [Get Started](#get-started)
- [Run CoffeeDB](#run-coffeedb)
- [Supported Operations](#supported-operations)
  - [Insert](#insert)
  - [Remove](#remove)
  - [Build](#build)
  - [Query](#query)

## Get Started
[Download](https://github.com/sunkafei/coffeedb/releases) CoffeeDB, put it under any folder, run `./coffeedb`, then CoffeeDB will create a database under this folder and start the service. By default, the service address is http://127.0.0.1:14920/coffeedb. All database operations are handled by the Post method of this address.

For example, you can insert a piece of data into the database by sending the following json to http://127.0.0.1:14920/coffeedb.
```json
{
    "operation": "insert",
    "data": {
        "number": 123,
        "name": "sunkafei",
        "secret": "01010"
    }
}
```
Note that the corresponding value of "data" is the inserted object, which can contain any number of key-value pairs, and the type of value can be integer, real number or string.

Next, we insert another piece of data into the database:
```json
{
    "operation": "insert",
    "data": {
        "number": 234,
        "name": "yulemao",
        "position": 1.7724,
        "secret": "010"
    }
}
```
As we can seen, different objects can contain different fields, but the same fields must be of the same type. After we finish modifying the database, we need to call the build operation to create the corresponding data structure and make the modification take effect:
```json
{
    "operation": "build"
}
```
Next we can query the database:
```json
{
    "operation": "query",
    "constraints": {
        "number": "[100,200]"
    }
}
```
It returns all objects whose *number* is between $100$ and $200$. The result may look like this:
```json
[{"number":123,"name":"sunkafei","secret":"01010"}]
```
Note that we can adjust the query to open interval by writing "(100,200)". If you only want to see some keys of the object but not all, you can use `fields` to specify. Here's an example:
```json
{
    "operation": "query",
    "constraints": {
        "number": "[100,900]"
    },
    "fields": ["name"]
}
```
The query result is:
```json
[{"name":"sunkafei"},{"name":"yulemao"}]
```
In the case of string/keyword searches, an additional key named `$correlation` is added to the object to indicate the number of occurrences of the keyword. For example:
```json
{
    "operation": "query",
    "constraints": {
        "secret": "010"
    }
}
```
The query result is:
```json
[{"$correlation":2,"number":123,"name":"sunkafei","secret":"01010"},{"$correlation":1,"number":234,"name":"yulemao","position":1.7724,"secret":"010"}]
```
If there are multiple query constraints that need to be met, you can simply list them in `constraints`. For example:
```json
{
    "operation": "query",
    "constraints": {
        "secret": "010",
        "number": "[200,900]"
    },
    "fields": ["name"]
}
```
The query result is:
```json
[{"name":"yulemao"}]
```
You can find a sample Python code [here](examples/examples.py).

## Run CoffeeDB
[Download](https://github.com/sunkafei/coffeedb/releases) CoffeeDB, put it under any folder, run `./coffeedb`, then CoffeeDB will create a database under this folder and start the service. By default, the service address is http://127.0.0.1:14920/coffeedb. All database operations are handled by the Post method of this address. You can change CoffeeDB's default behavior with the following command-line parameters:

|Key|Value Type|Explanation|
|-|-|-|
|port|Integer|The port number to bind to.|
|clear|None|Clear all data before starting the service. Use this command with caution as it will delete all data irretrievably.|
|directory|String|The folder where the data is saved.|

For example, the following command clear the past data and start service at http://127.0.0.1:12345/coffeedb.
```bash
$ ./coffeedb --port=12345 --clear
```

## Supported Operations
### Insert
The `insert` operation has the following general format:
```json
{
    "operation": "insert",
    "data": {
        ...
    }
}
```
where the `data` field contains the object to be inserted. The inserted object can contain any number of key-value pairs. Note that all `insert` operations will be cached and will not take effect immediately. To make `insert` operations effective, you need to call the [build](#build) operation.

### Remove
The `remove` operation has the following general format:
```json
{
    "operation": "remove",
    "constraints": {
        ...
    }
}
```
