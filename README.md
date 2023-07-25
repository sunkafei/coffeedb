![](docs/logo.png)
![](https://github.com/sunkafei/coffeedb/actions/workflows/main.yml/badge.svg?event=push)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/sunkafei/coffeedb/main/LICENSE.MIT)
[![GitHub Downloads](https://img.shields.io/github/downloads/sunkafei/coffeedb/total)](https://github.com/sunkafei/coffeedb/releases)
[![GitHub stars](https://img.shields.io/github/stars/sunkafei/coffeedb)](https://github.com/sunkafei/coffeedb/stargazers)

**CoffeeDB** is an out-of-the-box string/keyword search database. CoffeeDB builds indexes for numeric data and suffix arrays for text data to speed up query operations. Unlike most existing databases, CoffeeDB maintains these data structures in memory. On the one hand, this significantly improves the query speed, on the other hand, it limits its ability to handle very large-scale data. If the data you need to retrieve does not exceed tens of gigabytes, then CoffeeDB may be the best practice you want.

- [Get Started](#get-started)
- [Run CoffeeDB](#run-coffeedb)
- [Supported Operations](#supported-operations)
  - [Insert](#insert)
  - [Remove](#remove)
  - [Build](#build)
  - [Query](#query)
  - [Count](#count)
- [Benchmark](#benchmark)

## Get Started
[Download](https://github.com/sunkafei/coffeedb/releases) CoffeeDB, put it under any folder, run `./coffeedb`, then CoffeeDB will create a database under this folder and start the service. By default, the service address is http://127.0.0.1:14920/coffeedb. All database operations are handled by the `Post` method of this address.

For example, you can insert a piece of data into the database by sending the following json to http://127.0.0.1:14920/coffeedb.
```javascript
{
    "operation": "insert",
    "data": {
        "number": 123,
        "name": "sunkafei",
        "secret": "3010103"
    }
}
```
Note that the corresponding value of `data` is the inserted object, which can contain any number of key-value pairs, and the type of value can be boolean, integer, real number or string.

Next, we insert another piece of data into the database:
```javascript
{
    "operation": "insert",
    "data": {
        "number": 234,
        "name": "yulemao",
        "position": 1.7724,
        "secret": "301022"
    }
}
```
As we can seen, different objects can contain different fields, but the same fields must be of the same type. After we finish modifying the database, we need to call the build operation to create the corresponding data structure and make the modification take effect:
```javascript
{
    "operation": "build"
}
```
Next we can query the database:
```javascript
{
    "operation": "query",
    "constraints": {
        "number": "[100,200]"
    }
}
```
It returns all objects whose *number* is between $100$ and $200$. The result may look like this:
```javascript
[{"number":123,"name":"sunkafei","secret":"301022"}]
```
Note that we can adjust the query to open interval by writing "(100,200)". If you only want to see some keys of the object but not all, you can use `fields` to specify. Here's an example:
```javascript
{
    "operation": "query",
    "constraints": {
        "number": "[100,900]"
    },
    "fields": ["name"]
}
```
The query result is:
```javascript
[{"name":"sunkafei"},{"name":"yulemao"}]
```
In the case of string/keyword searches, an additional key named `$correlation` is added to the object to indicate the number of occurrences of the keyword. For example:
```javascript
{
    "operation": "query",
    "constraints": {
        "secret": "010"
    }
}
```
The query result is:
```javascript
[{"$correlation":2,"number":123,"name":"sunkafei","secret":"3010103"},{"$correlation":1,"number":234,"name":"yulemao","position":1.7724,"secret":"301022"}]
```
A more complex query is shown below. Both constraints `"secret":"010"` and `"number":"[0,900]"` must be met.
The *name* and *secret* fields of objects that meet the constraints will be selected. Then, all `010` in the secret field will be replaced with `<b>010</b>`. Finally, only the first object is returned, as indicated by the `span` field.
```javascript
{
    "operation": "query",
    "constraints": {
        "secret": "010",
        "number": "[0,900]"
    },
    "fields": ["name", "secret"],
    "highlight": ["<b>", "</b>"],
    "span": "[0,1)"
}
```
The query result is:
```javascript
[{"name":"sunkafei","secret":"3<b>01010</b>3"}]
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
```javascript
{
    "operation": "insert",
    "data": {
        ...
    }
}
```
where the `data` field contains the object to be inserted. The inserted object can contain any number of key-value pairs. Supported data types include integers, floats, and strings. Different objects can have different fields, but the value type of fields with the same name must be the same. Note that all `insert` operations will be cached and will not take effect immediately. To make `insert` operations effective, you need to call the [build](#build) operation.

### Remove
The `remove` operation has the following general format:
```javascript
{
    "operation": "remove",
    "constraints": {
        ...
    }
}
```
where all objects satisfy the `constraints` will be removed from the database. The format of constraints is the same as that in [query](#query) operation. Note that all `remove` operations will be cached and will not take effect immediately. To make `remove` operations effective, you need to call the [build](#build) operation.

### Build
The `build` operation has the following format:
```javascript
{
    "operation": "build"
}
```
It makes all database modification operations take effect. As the `build` operation is time-consuming, you should call `build` once after all modifications are completed, rather than calling `build` after each modification.

### Query
The `query` operation has the following general format:
```javascript
{
    "operation": "query",
    "constraints": {
        ...
    },
    "fields": [
        ...
    ],
    "highlight": [
        left-padding,
        right-padding
    ],
    "span": interval
}
```
Only the `operation` field is required, and the rest are optional.
All obejcts that meet the constraints in `constraints` will be selected, and then their fields in `fields` will be filtered out. All objects will be sorted according to the matching correlation. If the `highlight` field is specified, all occurrences of the query keywords will be surrounded by *left-padding* and *right-padding*. If the `span` field is specified, only objects whose indices are in *interval* will be returned.

You can get all objects in the database by omitting `constraints`, and you can get all fields in objects by omitting `fields`.

For different types of fields, `constraints` have different formats:

- For fields of string type, the constraint can be a substring that must appear. In this case, an additional field named `$correlation` will be added to the returned object to indicate the number of occurrences of this substring.

- For fields of boolean type, the constraint must be "true" or "false".

- For fields of type integer and float, the constraint can be an interval indicating the range of numbers. For example:

    |Value|Explanation|
    |-|-|
    |[1,100]|Values between $1$ and $100$.|
    |[1,inf)|Values greater than $1$ (inclusive).|
    |[-inf,1)|Values less than $1$ (exclusive).|

Multiple constraints can be specified for each field. There is an **OR** relationship between conditions in the same field, and an **AND** relationship between different fields. 
The following example means: select all objects whose `name` contains "coffee" as a substring and whose `age` is between $[10,20]$ or $[30,40]$.
```javascript
{
    "operation": "query",
    "constraints": {
        "name": "coffee",
        "age": ["[10,20]", "[30,40]"]
    }
}
```

You can find more use cases of the `query` operation in [Get Started](#get-started).

### Count
The `count` operation has the following general format:
```javascript
{
    "operation": "count",
    "constraints": {
        ...
    }
}
```
It returns a json, where the `count` field indicates how many objects satisfy the `constraints` in the database. If no constraints are specified, the total number of objects in the database is returned. The format of `constraints` is the same as that in [query](#query) operation. 


## Benchmark
We tested the running time of a single keyword query under different data scales. The testing environment consists of 32 CPUs(Intel Cooper Lake 3.0GHz) and 128GB memory. Each character of all strings is randomly generated from `'a'` to `'z'`. In order to ensure that the query results will not be too many or too few, we fix the length of the query string to $5$. As shown in the table below, query times are on the millisecond level, and most of the time is spent on data transfer rather than string retrieval.
|Number of objects|String length for each object|Total amount of data|Query time|
|-|-|-|-|
|32768|32768|1GB|1.2ms|
|32768|65536|2GB|1.3ms|
|65536|65536|4GB|1.5ms|
|65536|131072|8GB|2.0ms|

The test script can be found [here](test/benchmark.py).