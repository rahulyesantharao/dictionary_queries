# dictionary_queries
To test, use `./test.sh -[b|m|a] <1-4>`. The first flag represents which of the 3 measurements we want to take: b = build time, m = memory usage, a = access time. The argument represents which of the 4 memory representations we want to use: 1 = cJSON, 2 = hashmap, 3 = flattened struct, 4 = serial + indices.

```bash
cp tweets1.json tweets_test.json
cat tweets2.json >> tweets_test.json
```
