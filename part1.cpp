#include <iostream>
#include <fstream>
#include "simdjson/singleheader/simdjson.h"
#include "simdjson/singleheader/simdjson.cpp"
#include "cJSON/cJSON.h"
using namespace simdjson;

void build_cjson(const ParsedJson::Iterator &pjh);
//const char *filename = "tweets.json";
const char *filename = "simdjson/jsonexamples/twitterescaped.json";

int main() {
  padded_string p = get_corpus(filename);

  // parse with simdjson
  std::cout << "padded_string length: " << p.length() << "\n";
  ParsedJson pj = build_parsed_json(p);
  if(!pj.is_valid()) {
    std::cout << "not valid!: " << pj.get_error_message() << std::endl;
    return -1;
  }
  std::cout << "valid json!\n";
  ParsedJson::Iterator pjh(pj);
  if (!pjh.is_ok()) {
    std::cerr << " Could not iterate parsed result. " << std::endl;
    return -1;
  }
  std::cout << "simdjson parsed!\n";

  // parse with cJSON
  cJSON *test = cJSON_Parse(p.data());

  // keys to access
  std::string keys[10] = {};

  // build array based on keys
}

void build_cjson(const ParsedJson::Iterator &pjh) {
  //if (pjh.is_object()) {
    //std::cout << "{";
    //if (pjh.down()) {
      //pjh.print(std::cout); // must be a string
      //std::cout << ":";
      //pjh.next();
      //compute_dump(pjh); // let us recurse
      //while (pjh.next()) {
        //std::cout << ",";
        //pjh.print(std::cout);
        //std::cout << ":";
        //pjh.next();
        //compute_dump(pjh); // let us recurse
      //}
      //pjh.up();
    //}
    //std::cout << "}";
  //} else if (pjh.is_array()) {
    //std::cout << "[";
    //if (pjh.down()) {
      //compute_dump(pjh); // let us recurse
      //while (pjh.next()) {
        //std::cout << ",";
        //compute_dump(pjh); // let us recurse
      //}
      //pjh.up();
    //}
    //std::cout << "]";
  //} else {
    //pjh.print(std::cout); // just print the lone value
  //}
}
