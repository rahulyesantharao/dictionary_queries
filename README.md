# dictionary_queries
To test, use `./test.sh -[b|m|a] <1-4>`. The first flag represents which of the 3 measurements we want to take: b = build time, m = memory usage, a = access time. The argument represents which of the 4 memory representations we want to use: 1 = cJSON, 2 = hashmap, 3 = flattened struct, 4 = serial + indices.

To create the test files, run (after getting and extracting the `yelp_dataset` from https://www.yelp.com/dataset/download):
```bash
# create tweets test ~140MB
cp tweets1.json tweets_test.json
cat tweets2.json >> tweets_test.json

# create yelp tips test ~190MB
head -n 1000000 yelp_dataset/tip.json > yelp_tip_test.json
```

