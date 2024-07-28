<h1 align="center">Code obfuscation based on deep integration</h1>

<h4 align="center">
<p>
<a href=#about>About</a> |
<a href=#details>Details</a>
<p>
</h4>

## About

We present a generalized code obfuscation framework based on deep integration, and implement an integration algorithm of instructions and data flows from two programs based on intermediate representation. The integrated program preserves all functionalities of both programs. More importantly, this approach utilizes the parent program's static features and dynamic behaviors to obfuscate the target program's characteristics.


## Details

### source

Source code are in ./src directory

### matrix

We provide a matrix example as shown in ./matrix/point.json, where key is the integrate point and value is the runtime value.

```json
{
 "main----entry----0": 0,
 "main----for.cond----1": 1,
 "main----for.cond----2": "UNK",
 "main----for.body----1": 1,
 "main----for.body----5": 1,
 "main----for.body----9": 1,
 "main----for.body----12": "UNK",
 "main----for.body----13": "UNK",
 "main----for.body----14": 1,
 "main----for.body----18": 1,
 "main----for.body----20": "UNK",
}
```
main represents function, entry or for.cond represents block name, 0 or 1 represents instruction index and the value 0 or 1 represents runtime value of this instruction. If the value is not unique, identified as "UNK".

