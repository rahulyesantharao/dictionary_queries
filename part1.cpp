#include "cJSON/cJSON.h"
#include "simdjson/singleheader/simdjson.cpp"
#include "simdjson/singleheader/simdjson.h"
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace simdjson;

cJSON *build_cjson(ParsedJson::Iterator &pjh);

const char *filename = "single_tweet.json";
const std::vector<std::vector<std::string>> keys_to_keep = {{"a", "b"}, {"c"}};
int main() {
  std::ifstream in(filename);

  // parse the input objects one by one
  std::string line;
  while (std::getline(in, line)) {
    // simdjson to validate and parse
    ParsedJson pj = build_parsed_json(line);
    if (!pj.is_valid()) {
      std::cerr << "Invalid JSON: " << pj.get_error_message() << std::endl;
      return -1;
    }
    ParsedJson::Iterator pjh(pj);
    if (!pjh.is_ok()) {
      std::cerr << "Parsing Error." << std::endl;
      return -1;
    }

    // build the four representations
    // 1: cJSON tree
    cJSON *test = build_cjson(pjh);
    printf("%s\n", cJSON_PrintUnformatted(test));
    // 2: nested hashmap
    // std::unordered_map<std::string, void *> hashmap = build_hashmap(pjh);
  }
}

cJSON *build_cjson(ParsedJson::Iterator &pjh) {
  cJSON *ret = NULL;
  if (pjh.is_object()) {
    ret = cJSON_CreateObject();
    if (pjh.down()) {
      do {
        // get key
        std::string cur_key(pjh.get_string());

        // above may cause issues if the string has null characters, consider
        // something like below
        // std::string cur_key(pjh.get_string_length());
        // std::copy(pjh.get_string(), pjh.get_string() +
        // pjh.get_string_length(), cur_key.begin());

        // get value
        pjh.next();
        cJSON *val = build_cjson(pjh); // recurse
        // add the pair to the object
        cJSON_AddItemToObject(ret, cur_key.c_str(), val);
      } while (pjh.next());
      pjh.up();
    }
  } else if (pjh.is_array()) {
    ret = cJSON_CreateArray();
    if (pjh.down()) {
      do {
        cJSON *val = build_cjson(pjh); // recurse
        cJSON_AddItemToArray(ret, val);
      } while (pjh.next());
      pjh.up();
    }
  } else { // single type
    switch (pjh.get_type()) {
    case '"': // string
      ret = cJSON_CreateString(pjh.get_string());
      break;
    case 'l':
      ret = cJSON_CreateNumber((double)pjh.get_integer());
      break;
    case 'u':
      ret = cJSON_CreateNumber((double)pjh.get_unsigned_integer());
      break;
    case 'd':
      ret = cJSON_CreateNumber(pjh.get_double());
      break;
    case 'n': // we have a null
      ret = cJSON_CreateNull();
      break;
    case 't': // we have a true
      ret = cJSON_CreateTrue();
      break;
    case 'f': // we have a false
      ret = cJSON_CreateFalse();
      break;
    default:
      std::cerr << "Error in cJSON Parsing!" << std::endl;
      ret = nullptr;
    }
  }
  return ret;
}
