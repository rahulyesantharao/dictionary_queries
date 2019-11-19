#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cJSON/cJSON.h"
#include "simdjson/singleheader/simdjson.cpp"
#include "simdjson/singleheader/simdjson.h"
using namespace simdjson;

///////////////////////////////////////////
// Timer
///////////////////////////////////////////
#include <chrono>

class Timer {
 private:
  std::chrono::high_resolution_clock::time_point _start;

 public:
  Timer() { reset(); }

  /*!
   * returns time since start of the timer in seconds
   * @return time in seconds
   */
  double time() {
    auto stop = std::chrono::high_resolution_clock::now();

    double duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(stop - _start)
            .count() /
        1000000000.0;
    return duration;
  }

  void reset() { _start = std::chrono::high_resolution_clock::now(); }
};

///////////////////////////////////////////
// Memory Measurement
///////////////////////////////////////////
#ifdef MEMORY
uint64_t total_mem_usage;
bool track_mem = true;
void *operator new(size_t size) {
  if (track_mem) total_mem_usage += (uint64_t)size;
  void *p = malloc(size);
  return p;
}
#endif

///////////////////////////////////////////
// Hardcoded Parameters
// Also hardcoded are
//  - SetFlattenedStructField(...)
//  - PrintFlattenedStruct(...)
///////////////////////////////////////////
const char *filename = "tweets1.json";

const std::vector<std::vector<std::string>> keys_to_keep = {
    {"created_at"},
    {"text"},
    {"user", "id"},
    {"user", "screen_name"},
    {"user", "followers_count"}};

typedef struct {
  char *_1_created_at;
  char *_1_text;
  int64_t _2_user_id;
  char *_2_user_screen_name;
  int64_t _2_user_followers_count;
} FlattenedStruct;

///////////////////////////////////////////
// Functions to build various representations
// Lot of code duplication, leave it like this so that it is easy to change any
// individual case.
///////////////////////////////////////////

// build cJSON from simdjson
cJSON *BuildCJSON(ParsedJson::Iterator &pjh);
cJSON *BuildCJSONHelper(ParsedJson::Iterator &pjh,
                        std::vector<std::string> &cur_stack);

// Build Hashmap from simdjson
// typedef the hashmap type, so we can change later
typedef std::pair<char, void *> Any;
typedef std::unordered_map<std::string, Any> Hashmap;

Hashmap *BuildHashmap(ParsedJson::Iterator &pjh);
void *BuildHashmapHelper(ParsedJson::Iterator &pjh,
                         std::vector<std::string> &cur_stack);
void PrintHashmapValue(Any val);
void PrintHashmap(Hashmap *hashmap);

// Build a flattened struct from simdjson
FlattenedStruct *BuildFlattenedStruct(ParsedJson::Iterator &pjh);
void *BuildFlattenedStructHelper(ParsedJson::Iterator &pjh,
                                 std::vector<std::string> &cur_stack,
                                 FlattenedStruct *build);
void SetFlattenedStructField(std::vector<std::string> &cur_stack, Any val,
                             FlattenedStruct *build);
void PrintFlattenedStruct(FlattenedStruct *to_print);

// Find indices of the relevant fields
std::pair<std::string, std::vector<std::pair<int, int>> *> BuildSerialIndexList(
    ParsedJson::Iterator &pjh);
std::string BuildSerialIndexListHelper(ParsedJson::Iterator &pjh,
                                       std::vector<std::string> &cur_stack,
                                       int &cur_pos, int &len,
                                       std::vector<std::pair<int, int>> *build);
void PrintSerialIndexList(
    std::pair<std::string, std::vector<std::pair<int, int>> *> indices);
///////////////////////////////////////////
// Implementations of functions
///////////////////////////////////////////

// CJSON Functions
cJSON *BuildCJSON(ParsedJson::Iterator &pjh) {
  std::vector<std::string> temp;
  return BuildCJSONHelper(pjh, temp);
}

cJSON *BuildCJSONHelper(ParsedJson::Iterator &pjh,
                        std::vector<std::string> &cur_stack) {
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

        pjh.next();
        // get value
        cur_stack.push_back(cur_key);
        cJSON *val = BuildCJSONHelper(pjh, cur_stack);  // recurse

        // determine whether this key should be part of the final
        // representation
        bool should_insert = false;
        for (int i = 0; !should_insert && i < keys_to_keep.size(); i++) {
          if (cur_stack.size() <= keys_to_keep[i].size()) {
            bool good = true;
            for (int j = 0; good && j < cur_stack.size(); j++) {
              if (cur_stack[j] != keys_to_keep[i][j]) good = false;
            }
            if (good) should_insert = true;
          }
        }
        cur_stack.pop_back();

        // add the pair to the object if it is desired
        if (should_insert) {
          cJSON_AddItemToObject(ret, cur_key.c_str(), val);
        }
      } while (pjh.next());
      pjh.up();
    }
  } else if (pjh.is_array()) {
    ret = cJSON_CreateArray();
    if (pjh.down()) {
      do {
        cJSON *val = BuildCJSONHelper(pjh, cur_stack);  // recurse
        cJSON_AddItemToArray(ret, val);
      } while (pjh.next());
      pjh.up();
    }
  } else {  // single type
    switch (pjh.get_type()) {
      case '"':  // string
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
      case 'n':  // we have a null
        ret = cJSON_CreateNull();
        break;
      case 't':  // we have a true
        ret = cJSON_CreateTrue();
        break;
      case 'f':  // we have a false
        ret = cJSON_CreateFalse();
        break;
      default:
        std::cerr << "Error in cJSON Parsing!" << std::endl;
        ret = nullptr;
    }
  }
  return ret;
}

// Hashmap Functions
Hashmap *BuildHashmap(ParsedJson::Iterator &pjh) {
  std::vector<std::string> temp;
  return (Hashmap *)BuildHashmapHelper(pjh, temp);
}

void *BuildHashmapHelper(ParsedJson::Iterator &pjh,
                         std::vector<std::string> &cur_stack) {
  void *ret;
  if (pjh.is_object()) {
    Hashmap *hashmap = new Hashmap;
    ret = (void *)hashmap;
    if (pjh.down()) {
      do {
        // get key
        std::string cur_key(pjh.get_string());

        // above may cause issues if the string has null characters, consider
        // something like below
        // std::string cur_key(pjh.get_string_length());
        // std::copy(pjh.get_string(), pjh.get_string() +
        // pjh.get_string_length(), cur_key.begin());

        cur_stack.push_back(cur_key);
        pjh.next();
        // get type
        char type = (char)pjh.get_type();
        // get value
        void *val = BuildHashmapHelper(pjh, cur_stack);  // recurse

        // determine whether this key should be part of the final
        // representation
        bool should_insert = false;
        for (int i = 0; !should_insert && i < keys_to_keep.size(); i++) {
          if (cur_stack.size() <= keys_to_keep[i].size()) {
            bool good = true;
            for (int j = 0; good && j < cur_stack.size(); j++) {
              if (cur_stack[j] != keys_to_keep[i][j]) good = false;
            }
            if (good) should_insert = true;
          }
        }
        cur_stack.pop_back();

        // add the pair to the object if it is desired
        if (should_insert) {
          (*hashmap)[cur_key] = std::make_pair(type, val);
        }
      } while (pjh.next());
      pjh.up();
    }
  } else if (pjh.is_array()) {
    std::list<Any> *arr = new std::list<Any>;
    ret = (void *)arr;
    if (pjh.down()) {
      do {
        char type = (char)pjh.get_type();
        void *val = BuildHashmapHelper(pjh, cur_stack);  // recurse
        arr->push_back(std::make_pair(type, val));
      } while (pjh.next());
      pjh.up();
    }
  } else {  // single type
    switch (pjh.get_type()) {
      case '"': {  // string
        char *val = new char[pjh.get_string_length()];
        memcpy(val, pjh.get_string(), pjh.get_string_length());
        ret = (void *)val;
      } break;
      case 'l': {
        int64_t *val = new int64_t;
        *val = pjh.get_integer();
        ret = (void *)val;
      } break;
      case 'u': {
        uint64_t *val = new uint64_t;
        *val = pjh.get_unsigned_integer();
        ret = (void *)val;
      } break;
      case 'd': {
        double *val = new double;
        *val = pjh.get_double();
        ret = (void *)val;
      } break;
      case 'n':  // we have a null
        ret = nullptr;
        break;
      case 't':         // we have a true
        ret = nullptr;  // already stored in key string
        break;
      case 'f':         // we have a false
        ret = nullptr;  // already stored in key string
        break;
      default:
        std::cerr << "Error in cJSON Parsing!" << std::endl;
        ret = nullptr;
    }
  }
  return ret;
}

void PrintHashmap(Hashmap *hashmap) {
  PrintHashmapValue(std::make_pair('{', (void *)hashmap));
}
void PrintHashmapValue(Any val) {
  char type = val.first;
  void *ptr = val.second;
  switch (type) {
    case '{': {
      Hashmap *hashmap = (Hashmap *)ptr;
      std::cout << "{";
      int count = 0;
      for (Hashmap::iterator it = hashmap->begin(); it != hashmap->end();
           ++it, ++count) {
        std::string key = it->first;

        if (count > 0) std::cout << ",";
        std::cout << "\"" << key << "\":";
        PrintHashmapValue(it->second);
      }
      std::cout << "}";
    } break;
    case '[': {
      std::list<Any> *arr = (std::list<Any> *)ptr;
      int count = 0;
      for (std::list<Any>::iterator it = arr->begin(); it != arr->end();
           ++it, ++count) {
        if (count > 0) std::cout << ",";
        PrintHashmapValue(*it);
      }
    } break;
    case '"':  // string
      std::cout << "\"" << (const char *)ptr << "\"";
      break;
    case 'l':
      std::cout << *(int64_t *)ptr;
      break;
    case 'u':
      std::cout << *(uint64_t *)ptr;
      break;
    case 'd':
      std::cout << *(double *)ptr;
      break;
    case 'n':  // we have a null
      std::cout << "null";
      break;
    case 't':  // we have a true
      std::cout << "true";
      break;
    case 'f':  // we have a false
      std::cout << "false";
      break;
  }
}

// Flattened Struct
FlattenedStruct *BuildFlattenedStruct(ParsedJson::Iterator &pjh) {
  FlattenedStruct *ret = new FlattenedStruct;
  std::vector<std::string> temp;
  BuildFlattenedStructHelper(pjh, temp, ret);
  return ret;
}

void *BuildFlattenedStructHelper(ParsedJson::Iterator &pjh,
                                 std::vector<std::string> &cur_stack,
                                 FlattenedStruct *build) {
  void *ret;
  if (pjh.is_object()) {
    ret = nullptr;
    if (pjh.down()) {
      do {
        // get key
        std::string cur_key(pjh.get_string());

        // above may cause issues if the string has null characters, consider
        // something like below
        // std::string cur_key(pjh.get_string_length());
        // std::copy(pjh.get_string(), pjh.get_string() +
        // pjh.get_string_length(), cur_key.begin());

        cur_stack.push_back(cur_key);
        pjh.next();
        // get type
        char type = (char)pjh.get_type();
        // get value
        void *val =
            BuildFlattenedStructHelper(pjh, cur_stack, build);  // recurse
        // set the appropriate field of the struct if needed
        SetFlattenedStructField(cur_stack, std::make_pair(type, val), build);
        cur_stack.pop_back();
      } while (pjh.next());
      pjh.up();
    }
  } else if (pjh.is_array()) {
    // treat an array like a dict with integer indices
    ret = nullptr;
    if (pjh.down()) {
      int index = 0;
      do {
        cur_stack.push_back(std::to_string(index));
        char type = (char)pjh.get_type();
        void *val =
            BuildFlattenedStructHelper(pjh, cur_stack, build);  // recurse
        SetFlattenedStructField(cur_stack, std::make_pair(type, val), build);
        cur_stack.pop_back();
        index++;
      } while (pjh.next());
      pjh.up();
    }
  } else {  // single type
    switch (pjh.get_type()) {
      case '"': {  // string
        char *val = new char[pjh.get_string_length()];
        memcpy(val, pjh.get_string(), pjh.get_string_length());
        ret = (void *)val;
      } break;
      case 'l': {
        int64_t *val = new int64_t;
        *val = pjh.get_integer();
        ret = (void *)val;
      } break;
      case 'u': {
        uint64_t *val = new uint64_t;
        *val = pjh.get_unsigned_integer();
        ret = (void *)val;
      } break;
      case 'd': {
        double *val = new double;
        *val = pjh.get_double();
        ret = (void *)val;
      } break;
      case 'n':  // we have a null
        ret = nullptr;
        break;
      case 't':         // we have a true
        ret = nullptr;  // already stored in type char
        break;
      case 'f':         // we have a false
        ret = nullptr;  // already stored in type char
        break;
      default:
        std::cerr << "Error in Struct Parsing!" << std::endl;
        ret = nullptr;
    }
  }
  return ret;
}

void SetFlattenedStructField(std::vector<std::string> &cur_stack, Any val,
                             FlattenedStruct *build) {
  // determine whether this key should be part of the final
  // representation
  bool should_insert = false;
  int i;
  for (i = 0; !should_insert && i < keys_to_keep.size(); i++) {
    if (cur_stack.size() == keys_to_keep[i].size()) {
      bool good = true;
      for (int j = 0; good && j < cur_stack.size(); j++) {
        if (cur_stack[j] != keys_to_keep[i][j]) good = false;
      }
      if (good) {
        should_insert = true;
        break;
      }
    }
  }

  // add the pair to the object if it is desired
  if (should_insert) {
    switch (i) {
      case 0:
        build->_1_created_at = (char *)val.second;
        break;
      case 1:
        build->_1_text = (char *)val.second;
      case 2:
        build->_2_user_id = *(int64_t *)val.second;
      case 3:
        build->_2_user_screen_name = (char *)val.second;
      case 4:
        build->_2_user_followers_count = *(int64_t *)val.second;
    }
  }
}

void PrintFlattenedStruct(FlattenedStruct *to_print) {
  std::cout << "{";
  std::cout << "\"created_at\":\"" << to_print->_1_created_at << "\",";
  std::cout << "\"text\":\"" << to_print->_1_text << "\",";
  std::cout << "\"user_id\":" << to_print->_2_user_id << ",";
  std::cout << "\"user_screen_name\":\"" << to_print->_2_user_screen_name
            << "\",";
  std::cout << "\"user_followers_count\":" << to_print->_2_user_followers_count;
  std::cout << "}";
}

std::pair<std::string, std::vector<std::pair<int, int>> *> BuildSerialIndexList(
    ParsedJson::Iterator &pjh) {
  std::vector<std::pair<int, int>> *ret = new std::vector<std::pair<int, int>>;
  ret->resize(keys_to_keep.size());
  std::vector<std::string> temp;
  int pos = 0;
  int len;
  std::string final_str = BuildSerialIndexListHelper(pjh, temp, pos, len, ret);
  return std::make_pair(final_str, ret);
}

std::string BuildSerialIndexListHelper(
    ParsedJson::Iterator &pjh, std::vector<std::string> &cur_stack,
    int &cur_pos, int &len, std::vector<std::pair<int, int>> *build) {
  std::string ret;
  len = cur_pos;
  if (pjh.is_object()) {
    ret += "{";
    cur_pos += 1;
    if (pjh.down()) {
      int index = 0;
      do {
        // deal with commas
        if (index > 0) {
          ret += ",";
          cur_pos += 1;
        }
        index++;
        // get key
        std::string cur_key(pjh.get_string());
        ret += "\"";
        ret += cur_key;
        ret += "\"";
        cur_pos += cur_key.length();
        ret += ":";
        cur_pos += 3;

        // determine whether to use current key
        cur_stack.push_back(cur_key);
        bool should_insert = false;
        int i;
        for (i = 0; !should_insert && i < keys_to_keep.size(); i++) {
          if (cur_stack.size() == keys_to_keep[i].size()) {
            bool good = true;
            for (int j = 0; good && j < cur_stack.size(); j++) {
              if (cur_stack[j] != keys_to_keep[i][j]) good = false;
            }
            if (good) {
              should_insert = true;
              break;
            }
          }
        }
        if (should_insert) {
          (*build)[i].first = cur_pos;
        }
        // get value
        pjh.next();
        int child_len;
        ret += BuildSerialIndexListHelper(pjh, cur_stack, cur_pos, child_len,
                                          build);  // let us recurse
        if (should_insert) {
          (*build)[i].second = child_len;
        }
        cur_stack.pop_back();
      } while (pjh.next());
      pjh.up();
    }
    ret += "}";
    cur_pos += 1;
  } else if (pjh.is_array()) {
    ret += "[";
    cur_pos += 1;
    if (pjh.down()) {
      int index = 0;
      do {
        if (index > 0) {
          ret += ",";
          cur_pos += 1;
        }
        index++;
        int child_len;
        ret += BuildSerialIndexListHelper(pjh, cur_stack, cur_pos, child_len,
                                          build);  // let us recurse
      } while (pjh.next());
      pjh.up();
    }
    ret += "]";
    cur_pos += 1;
  } else {
    switch (pjh.get_type()) {
      case '"': {  // string
        std::string val(pjh.get_string());
        ret += "\"";
        ret += val;
        ret += "\"";
        cur_pos += val.length() + 2;
      } break;
      case 'l': {
        std::string val = std::to_string((int64_t)pjh.get_integer());
        ret += val;
        cur_pos += val.length();
      } break;
      case 'u': {
        std::string val = std::to_string((uint64_t)pjh.get_integer());
        ret += val;
        cur_pos += val.length();
      } break;
      case 'd': {
        std::string val = std::to_string((double)pjh.get_integer());
        ret += val;
        cur_pos += val.length();
      } break;
      case 'n': {  // we have a null
        std::string val = "null";
        ret += val;
        cur_pos += val.length();
      } break;
      case 't': {  // we have a true
        std::string val = "true";
        ret += val;
        cur_pos += val.length();
      } break;
      case 'f': {  // we have a false
        std::string val = "false";
        ret += val;
        cur_pos += val.length();
      } break;
      default:
        std::cerr << "Error in List Index Parsing!" << std::endl;
        ret = "";
    }
  }
  len = cur_pos - len;
  return ret;
}

void PrintSerialIndexList(
    std::pair<std::string, std::vector<std::pair<int, int>> *> indices) {
  for (int i = 0; i < indices.second->size(); ++i) {
    int pos = (*indices.second)[i].first;
    int len = (*indices.second)[i].second;
    std::string key;
    for (int j = 0; j < keys_to_keep[i].size(); j++) {
      if (j > 0) key += "_";
      key += keys_to_keep[i][j];
    }
    std::cout << "\"" << key << "\": " << indices.first.substr(pos, len)
              << "\n";
  }
}

int main(int argc, char *argv[]) {
#ifndef TESTING
#error Must define a value for TESTING
#endif
#ifdef TIME
  Timer timer;
#endif
  std::ifstream in(filename);
  uint64_t num_lines = 0;
  std::string line;
  while (std::getline(in, line)) {
    num_lines++;
  }
  std::cout << "num lines: " << num_lines << "\n";

  in.clear();
  in.seekg(0, std::ios::beg);

  // Allocate space in memory for the data

#if TESTING == 1
  cJSON **arr;
  arr = new cJSON *[num_lines];
#elif TESTING == 2
  Hashmap **arr;
  arr = new Hashmap *[num_lines];
#elif TESTING == 3
  FlattenedStruct **arr;
  arr = new FlattenedStruct *[num_lines];
#elif TESTING == 4
  std::pair<std::string, std::vector<std::pair<int, int>> *> *arr;
  arr =
      new std::pair<std::string, std::vector<std::pair<int, int>> *>[num_lines];
#else
#error "Unexpected value of TESTING"
#endif

  // parse the input objects one by one and read them
  int index = 0;
#ifdef TIMEBUILD
  timer.reset();
#endif
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
#if TESTING == 1
    // 1: cJSON tree
    cJSON *test = BuildCJSON(pjh);
    arr[index] = test;
#elif TESTING == 2
    // 2: nested hashmap
    Hashmap *hashmap = BuildHashmap(pjh);
    arr[index] = hashmap;
#elif TESTING == 3
    // 3: flattened struct
    FlattenedStruct *fs = BuildFlattenedStruct(pjh);
    arr[index] = fs;
#elif TESTING == 4
    // 4: indices in serialized rep
    std::pair<std::string, std::vector<std::pair<int, int>> *> indices =
        BuildSerialIndexList(pjh);
    arr[index] = indices;
#else
#error "Invalid TESTING value"
#endif
    index++;
  }
#ifdef TIMEBUILD
  std::cout << "Time: " << timer.time() << "s\n";
#endif
#ifdef MEMORY
  std::cout << "Memory: " << (total_mem_usage / 1000000000.0) << "GB\n";
#endif

  // Iterate over the data afterwards
#ifdef VERBOSE
  for (int i = 0; i < num_lines; i++) {
#if TESTING == 1
    std::cout << "\ncJSON tree:\n";
    std::cout << cJSON_PrintUnformatted(arr[i]) << "\n";
#elif TESTING == 2
    std::cout << "\nHashmap\n";
    PrintHashmap(((Hashmap **)arr)[i]);
    std::cout << "\n";
#elif TESTING == 3
    std::cout << "\nFlattened Struct\n";
    PrintFlattenedStruct(arr[i]);
    std::cout << "\n";
#elif TESTING == 4
    std::cout << "\nSerial Index List\n";
    PrintSerialIndexList(arr[i]);
    std::cout << "\n";
#else
#error "Invalid TESTING value"
#endif
  }
#endif
}
