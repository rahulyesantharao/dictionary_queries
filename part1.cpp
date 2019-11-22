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
bool track_mem;

// struct mem_list_node {
// void *p;
// uint64_t size;
// struct mem_list_node *next;
// struct mem_list_node *prev;
//};

// struct mem_list_node *mem_list;

// void mem_list_delete(struct mem_list_node *node) {
// if (node->prev) node->prev->next = node->next;
// if (node->next) node->next->prev = node->prev;
// if (node == mem_list) mem_list = node->next;
// free(node);
//}

// void mem_list_insert(void *p, size_t size) {
// struct mem_list_node *node =
//(struct mem_list_node *)malloc(sizeof(struct mem_list_node));

// node->p = p;
// node->size = (uint64_t)size;
// node->prev = nullptr;
// node->next = mem_list;

// if (mem_list) {
// mem_list->prev = node;
//}
// mem_list = node;
//}

// uint64_t mem_list_find(void *p) {
// struct mem_list_node *cur = mem_list;
// while (cur) {
// if (p == cur->p) {
// uint64_t ret = cur->size;
// mem_list_delete(cur);
//// std::cout << "FOUND!\n";
// return ret;
//}
// cur = cur->next;
//}
// return 0;
//}

// for cjson
void *track_malloc(size_t size) {
  void *p = malloc(size + sizeof(uint64_t));
  if (track_mem) {
    total_mem_usage += (uint64_t)size;
    *(uint64_t *)p = (uint64_t)size;
  } else {
    *(uint64_t *)p = 0ULL;
  }
  return (void *)((char *)p + sizeof(uint64_t));
}

void track_free(void *p) {
  if (!p) return;
  uint64_t *size = (uint64_t *)((char *)p - sizeof(uint64_t));
  total_mem_usage -= *size;
  free((void *)size);
}

void *operator new(size_t size) { return track_malloc(size); }

void operator delete(void *p) { return track_free(p); }

#endif

///////////////////////////////////////////
// Hardcoded Parameters
// Also hardcoded are
//  - SetFlattenedStructField(...)
//  - PrintFlattenedStruct(...)
//  - ReadSerialIndexListField(...) (and to a lesser extent, the other Read
//  functions)
///////////////////////////////////////////
// Twitter
#ifdef TWITTER
const char *filename = "single_tweet.json";

const std::vector<std::vector<std::string>> keys_to_keep = {
    {"created_at"},
    {"text"},
    {"user", "id_str"},
    {"user", "screen_name"},
    {"user", "followers_count"},
    {"quoted_status", "user", "id_str"}};
const int key1_to_read_index = 2;
const int key2_to_read_index = 5;
const std::vector<std::string> key1_to_read = keys_to_keep[key1_to_read_index];
const std::vector<std::string> key2_to_read = keys_to_keep[key2_to_read_index];
typedef char *read_key1_type;
typedef char *read_key2_type;

typedef struct {
  char *_1_created_at;
  char *_1_text;
  char *_2_user_id_str;
  char *_2_user_screen_name;
  int64_t _2_user_followers_count;
  char *_3_quoted_status_user_id_str;
} FlattenedStruct;
std::unordered_map<std::string, int> offsets;

#else
// Yelp
const char *filename = "yelp_tip_test.json";

const std::vector<std::vector<std::string>> keys_to_keep = {
    {"user_id"}, {"business_id"}, {"text"}, {"date"}, {"compliment_count"},
};
const int key1_to_read_index = 1;
const int key2_to_read_index = 2;
const std::vector<std::string> key1_to_read = keys_to_keep[key1_to_read_index];
const std::vector<std::string> key2_to_read = keys_to_keep[key2_to_read_index];
typedef char *read_key1_type;
typedef char *read_key2_type;

typedef struct {
  char *_1_user_id;
  char *_1_business_id;
  char *_1_text;
  char *_1_date;
  int64_t _1_compliment_count;
} FlattenedStruct;
std::unordered_map<std::string, int> offsets;
#endif

///////////////////////////////////////////
// Functions to build various representations
// Lot of code duplication, leave it like this so that it is easy to change any
// individual case.
///////////////////////////////////////////

// build cJSON from simdjson
cJSON *BuildCJSON(ParsedJson::Iterator &pjh);
cJSON *BuildCJSONHelper(ParsedJson::Iterator &pjh,
                        std::vector<std::string> &cur_stack, int &num_found);
read_key2_type ReadCJSONField(cJSON *, read_key1_type &ret_key);
read_key2_type ReadCJSONFieldRT(cJSON *, read_key1_type &ret_key,
                                std::vector<std::string> key1,
                                std::vector<std::string> key2);

// Build Hashmap from simdjson
// typedef the hashmap type, so we can change later
typedef std::pair<char, void *> Any;
typedef std::unordered_map<std::string, Any> Hashmap;

Hashmap *BuildHashmap(ParsedJson::Iterator &pjh);
void *BuildHashmapHelper(ParsedJson::Iterator &pjh,
                         std::vector<std::string> &cur_stack, int &num_found);
void PrintHashmapValue(Any val);
void PrintHashmap(Hashmap *hashmap);
read_key2_type ReadHashmapField(Hashmap *hashmap, read_key1_type &ret_key);
read_key2_type ReadHashmapFieldRT(Hashmap *hashmap, read_key1_type &ret_key,
                                  std::vector<std::string> key1,
                                  std::vector<std::string> key2);
void RecursiveAnyDelete(Any val);

// Build a flattened struct from simdjson
FlattenedStruct *BuildFlattenedStruct(ParsedJson::Iterator &pjh);
void *BuildFlattenedStructHelper(ParsedJson::Iterator &pjh,
                                 std::vector<std::string> &cur_stack,
                                 FlattenedStruct *build, int &num_found);
bool SetFlattenedStructField(std::vector<std::string> &cur_stack, Any val,
                             FlattenedStruct *build, int &num_found);
void DeleteFlattenedStruct(FlattenedStruct *fs);
void PrintFlattenedStruct(FlattenedStruct *to_print);
read_key2_type ReadFlattenedStructField(FlattenedStruct *fs,
                                        read_key1_type &ret_key);
read_key2_type ReadFlattenedStructFieldRT(FlattenedStruct *fs,
                                          read_key1_type &ret_key,
                                          std::vector<std::string> key1,
                                          std::vector<std::string> key2);

// Find indices of the relevant fields
typedef std::pair<std::string *, std::vector<std::pair<int, int>> *>
    SerialIndexList;
SerialIndexList BuildSerialIndexList(ParsedJson::Iterator &pjh);
bool BuildSerialIndexListHelper(ParsedJson::Iterator &pjh,
                                std::vector<std::string> &cur_stack,
                                int &cur_pos, int &len,
                                std::vector<std::pair<int, int>> *build,
                                std::string *json_string, int &num_found);
void PrintSerialIndexList(SerialIndexList indices);
read_key2_type ReadSerialIndexListField(SerialIndexList indices,
                                        read_key1_type &ret_key);
read_key2_type ReadSerialIndexListFieldRT(SerialIndexList indices,
                                          read_key1_type &ret_key,
                                          std::vector<std::string> key1,
                                          std::vector<std::string> key2);

/// Helper
bool ShouldPrune(std::vector<std::string> &cur_stack) {
  bool should_prune = true;
  for (int i = 0; should_prune && i < keys_to_keep.size(); i++) {
    int prefix_len = (cur_stack.size() <= keys_to_keep.size())
                         ? cur_stack.size()
                         : keys_to_keep.size();
    bool good = true;
    for (int j = 0; good && j < prefix_len; j++) {
      if (cur_stack[j] != keys_to_keep[i][j]) good = false;
    }
    if (good) {
      should_prune = false;
    }
  }
  return should_prune;
}

///////////////////////////////////////////
// Implementations of functions
///////////////////////////////////////////

// CJSON Functions
cJSON *BuildCJSON(ParsedJson::Iterator &pjh) {
  std::vector<std::string> temp;
  int num_found = 0;
  cJSON *ret = BuildCJSONHelper(pjh, temp, num_found);
  if (num_found != keys_to_keep.size()) {
    cJSON_Delete(ret);
    return nullptr;
  } else
    return ret;
}

cJSON *BuildCJSONHelper(ParsedJson::Iterator &pjh,
                        std::vector<std::string> &cur_stack, int &num_found) {
  if (ShouldPrune(cur_stack)) return nullptr;
  cJSON *ret = nullptr;
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
        cJSON *val = BuildCJSONHelper(pjh, cur_stack, num_found);  // recurse

        // determine whether this key should be part of the final
        // representation
        bool should_insert = false;
        for (int i = 0; !should_insert && i < keys_to_keep.size(); i++) {
          if (cur_stack.size() <= keys_to_keep[i].size()) {
            bool good = true;
            for (int j = 0; good && j < cur_stack.size(); j++) {
              if (cur_stack[j] != keys_to_keep[i][j]) good = false;
            }
            if (good) {
              if (cur_stack.size() == keys_to_keep[i].size()) num_found++;
              should_insert = true;
            }
          }
        }
        cur_stack.pop_back();

        // add the pair to the object if it is desired
        if (should_insert) {
          cJSON_AddItemToObject(ret, cur_key.c_str(), val);
        } else {
          if (val) cJSON_Delete(val);
        }
      } while (pjh.next());
      pjh.up();
    }
  } else if (pjh.is_array()) {
    ret = cJSON_CreateArray();
    if (pjh.down()) {
      do {
        cJSON *val = BuildCJSONHelper(pjh, cur_stack, num_found);  // recurse
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

read_key2_type ReadCJSONField(cJSON *dict, read_key1_type &ret_key) {
  // key 1
  cJSON *top = dict;
  for (int i = 0; i < key1_to_read.size(); i++) {
    top = cJSON_GetObjectItemCaseSensitive(top, key1_to_read[i].c_str());
  }
  ret_key = (read_key1_type)top->valuestring;
  // key 2
  top = dict;
  for (int i = 0; i < key2_to_read.size(); i++) {
    top = cJSON_GetObjectItemCaseSensitive(top, key2_to_read[i].c_str());
  }
  return (read_key2_type)top->valuestring;
}

read_key2_type ReadCJSONFieldRT(cJSON *dict, read_key1_type &ret_key,
                                std::vector<std::string> key1,
                                std::vector<std::string> key2) {
  // key 1
  cJSON *top = dict;
  for (int i = 0; i < key1.size(); i++) {
    top = cJSON_GetObjectItemCaseSensitive(top, key1[i].c_str());
  }
  ret_key = (read_key1_type)top->valuestring;
  // key 2
  top = dict;
  for (int i = 0; i < key2.size(); i++) {
    top = cJSON_GetObjectItemCaseSensitive(top, key2[i].c_str());
  }
  return (read_key2_type)top->valuestring;
}

// Hashmap Functions
Hashmap *BuildHashmap(ParsedJson::Iterator &pjh) {
  std::vector<std::string> temp;
  int num_found = 0;
  Hashmap *ret = (Hashmap *)BuildHashmapHelper(pjh, temp, num_found);
  if (num_found != keys_to_keep.size()) {
    RecursiveAnyDelete(std::make_pair('{', (void *)ret));
    return nullptr;
  } else
    return ret;
}

void RecursiveAnyDelete(Any val) {
  char type = val.first;
  void *ptr = val.second;
  if (!ptr) return;
  switch (type) {
    case '{': {
      Hashmap *h = (Hashmap *)ptr;
      for (Hashmap::iterator it = h->begin(); it != h->end(); ++it) {
        RecursiveAnyDelete(it->second);
      }
      delete h;
    } break;
    case '[': {
      std::list<Any> *l = (std::list<Any> *)ptr;
      for (std::list<Any>::iterator it = l->begin(); it != l->end(); ++it) {
        RecursiveAnyDelete(*it);
      }
      delete l;
    } break;
    case '"':
      delete[]((char *)ptr);
      break;
    case 'l':
      delete (int64_t *)ptr;
      break;
    case 'u':
      delete (uint64_t *)ptr;
      break;
    case 'd':
      delete (double *)ptr;
      break;
    default:
      if (ptr != nullptr)
        std::cerr << "Something is wrong in Recursive Any Delete: Type = "
                  << type << "\n";
  }
}

void *BuildHashmapHelper(ParsedJson::Iterator &pjh,
                         std::vector<std::string> &cur_stack, int &num_found) {
  if (ShouldPrune(cur_stack)) return nullptr;
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
        void *val = BuildHashmapHelper(pjh, cur_stack, num_found);  // recurse

        // determine whether this key should be part of the final
        // representation
        bool should_insert = false;
        for (int i = 0; !should_insert && i < keys_to_keep.size(); i++) {
          if (cur_stack.size() <= keys_to_keep[i].size()) {
            bool good = true;
            for (int j = 0; good && j < cur_stack.size(); j++) {
              if (cur_stack[j] != keys_to_keep[i][j]) good = false;
            }
            if (good) {
              if (cur_stack.size() == keys_to_keep[i].size()) num_found++;
              should_insert = true;
            }
          }
        }
        cur_stack.pop_back();

        // add the pair to the object if it is desired
        if (should_insert) {
          (*hashmap)[cur_key] = std::make_pair(type, val);
        } else {
          RecursiveAnyDelete(std::make_pair(type, val));
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
        void *val = BuildHashmapHelper(pjh, cur_stack, num_found);  // recurse
        arr->push_back(std::make_pair(type, val));
      } while (pjh.next());
      pjh.up();
    }
  } else {  // single type
    switch (pjh.get_type()) {
      case '"': {  // string
        char *val = new char[pjh.get_string_length() + 1];
        memcpy(val, pjh.get_string(), pjh.get_string_length() + 1);
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

read_key2_type ReadHashmapField(Hashmap *hashmap, read_key1_type &ret_key) {
  // key 1
  void *cur_ptr = (void *)hashmap;
  for (int i = 0; i < key1_to_read.size(); i++) {
    cur_ptr = (*(Hashmap *)cur_ptr)[key1_to_read[i]].second;
  }
  ret_key = (read_key1_type)cur_ptr;
  // key 2
  cur_ptr = (void *)hashmap;
  for (int i = 0; i < key2_to_read.size(); i++) {
    cur_ptr = (*(Hashmap *)cur_ptr)[key2_to_read[i]].second;
  }
  return (read_key2_type)cur_ptr;
}

read_key2_type ReadHashmapFieldRT(Hashmap *hashmap, read_key1_type &ret_key,
                                  std::vector<std::string> key1,
                                  std::vector<std::string> key2) {
  // key 1
  void *cur_ptr = (void *)hashmap;
  for (int i = 0; i < key1.size(); i++) {
    cur_ptr = (*(Hashmap *)cur_ptr)[key1[i]].second;
  }
  ret_key = (read_key1_type)cur_ptr;
  // key 2
  cur_ptr = (void *)hashmap;
  for (int i = 0; i < key2.size(); i++) {
    cur_ptr = (*(Hashmap *)cur_ptr)[key2[i]].second;
  }
  return (read_key2_type)cur_ptr;
}

void DeleteFlattenedStruct(FlattenedStruct *fs) {
  // delete[] fs->_1_created_at;
  // delete[] fs->_1_text;
  // delete[] fs->_2_user_screen_name;
  delete fs;
}

// Flattened Struct
FlattenedStruct *BuildFlattenedStruct(ParsedJson::Iterator &pjh) {
  FlattenedStruct *ret = new FlattenedStruct;
  int num_found = 0;
  std::vector<std::string> temp;
  BuildFlattenedStructHelper(pjh, temp, ret, num_found);
  if (num_found != keys_to_keep.size()) {
    DeleteFlattenedStruct(ret);
    return nullptr;
  } else
    return ret;
}

void *BuildFlattenedStructHelper(ParsedJson::Iterator &pjh,
                                 std::vector<std::string> &cur_stack,
                                 FlattenedStruct *build, int &num_found) {
  if (ShouldPrune(cur_stack)) return nullptr;
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
        void *val = BuildFlattenedStructHelper(pjh, cur_stack, build,
                                               num_found);  // recurse
        // set the appropriate field of the struct if needed
        if (!SetFlattenedStructField(cur_stack, std::make_pair(type, val),
                                     build, num_found)) {
          switch (type) {
            case '"':
              delete[]((char *)val);
              break;
            case 'l':
              delete (int64_t *)val;
              break;
            case 'u':
              delete (uint64_t *)val;
              break;
            case 'd':
              delete (double *)val;
              break;
          }
        }
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
        void *val = BuildFlattenedStructHelper(pjh, cur_stack, build,
                                               num_found);  // recurse
        if (!SetFlattenedStructField(cur_stack, std::make_pair(type, val),
                                     build, num_found)) {
          switch (type) {
            case '"':
              delete[]((char *)val);
              break;
            case 'l':
              delete (int64_t *)val;
              break;
            case 'u':
              delete (uint64_t *)val;
              break;
            case 'd':
              delete (double *)val;
              break;
          }
        }
        cur_stack.pop_back();
        index++;
      } while (pjh.next());
      pjh.up();
    }
  } else {  // single type
    switch (pjh.get_type()) {
      case '"': {  // string
        char *val = new char[pjh.get_string_length() + 1];
        memcpy(val, pjh.get_string(), pjh.get_string_length() + 1);
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

bool SetFlattenedStructField(std::vector<std::string> &cur_stack, Any val,
                             FlattenedStruct *build, int &num_found) {
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
        num_found++;
        should_insert = true;
        break;
      }
    }
  }

  // add the pair to the object if it is desired
  if (should_insert) {
#ifdef TWITTER

    switch (i) {
      case 0:
        build->_1_created_at = (char *)val.second;
        break;
      case 1:
        build->_1_text = (char *)val.second;
        break;
      case 2:
        build->_2_user_id_str = (char *)val.second;
        break;
      case 3:
        build->_2_user_screen_name = (char *)val.second;
        break;
      case 4:
        build->_2_user_followers_count = *(int64_t *)val.second;
        break;
      case 5:
        build->_3_quoted_status_user_id_str = (char *)val.second;
        break;
      default:
        std::cerr << "Error creating flattened struct field\n";
    }
#else
    switch (i) {
      case 0:
        build->_1_user_id = (char *)val.second;
        break;
      case 1:
        build->_1_business_id = (char *)val.second;
        break;
      case 2:
        build->_1_text = (char *)val.second;
        break;
      case 3:
        build->_1_date = (char *)val.second;
        break;
      case 4:
        build->_1_compliment_count = *(int64_t *)val.second;
        break;
      default:
        std::cerr << "Error creating flattened struct field\n";
    }
#endif
  }
  return should_insert;
}

void PrintFlattenedStruct(FlattenedStruct *to_print) {
#ifdef TWITTER
  std::cout << "{";
  std::cout << "\"created_at\":\"" << to_print->_1_created_at << "\",";
  std::cout << "\"text\":\"" << to_print->_1_text << "\",";
  std::cout << "\"user_id_str\":\"" << to_print->_2_user_id_str << "\",";
  std::cout << "\"user_screen_name\":\"" << to_print->_2_user_screen_name
            << "\",";
  std::cout << "\"user_followers_count\":" << to_print->_2_user_followers_count
            << ",";
  std::cout << "\"quoted_status_user_id_str\":\""
            << to_print->_3_quoted_status_user_id_str << "\"";
  std::cout << "}";
#else
  std::cout << "{";
  std::cout << "\"user_id\":\"" << to_print->_1_user_id << "\",";
  std::cout << "\"business_id\":\"" << to_print->_1_business_id << "\",";
  std::cout << "\"text\":\"" << to_print->_1_text << "\",";
  std::cout << "\"date\":\"" << to_print->_1_date << "\",";
  std::cout << "\"compliment_count\":" << to_print->_1_compliment_count << "}";
#endif
}

read_key2_type ReadFlattenedStructField(FlattenedStruct *fs,
                                        read_key1_type &ret_key) {
#ifdef TWITTER
  ret_key = fs->_2_user_id_str;
  return fs->_3_quoted_status_user_id_str;
#else
  ret_key = fs->_1_business_id;
  return fs->_1_text;
#endif
}

read_key2_type ReadFlattenedStructFieldRT(FlattenedStruct *fs,
                                          read_key1_type &ret_key,
                                          std::vector<std::string> key1,
                                          std::vector<std::string> key2) {
  std::string key1_str = "_" + std::to_string(key1.size());
  for (int i = 0; i < key1.size(); i++) {
    key1_str += "_";
    key1_str += key1[i];
  }
  std::string key2_str = "_" + std::to_string(key2.size());
  for (int i = 0; i < key2.size(); i++) {
    key2_str += "_";
    key2_str += key2[i];
  }
  ret_key = *(read_key1_type *)((char *)fs + offsets[key1_str]);
  return *(read_key2_type *)((char *)fs + offsets[key2_str]);
}

// Serial Index List
SerialIndexList BuildSerialIndexList(ParsedJson::Iterator &pjh) {
  std::vector<std::pair<int, int>> *ret = new std::vector<std::pair<int, int>>;
  std::string *ret_str = new std::string;
  ret->resize(keys_to_keep.size());
  std::vector<std::string> temp;
  int pos = 0;
  int len;
  int num_found = 0;
  BuildSerialIndexListHelper(pjh, temp, pos, len, ret, ret_str, num_found);
  if (num_found != keys_to_keep.size()) {
    delete ret;
    delete ret_str;
    return std::make_pair(nullptr, nullptr);
  } else {
    return std::make_pair(ret_str, ret);
  }
}

bool BuildSerialIndexListHelper(ParsedJson::Iterator &pjh,
                                std::vector<std::string> &cur_stack,
                                int &cur_pos, int &len,
                                std::vector<std::pair<int, int>> *build,
                                std::string *json_str, int &num_found) {
  if (ShouldPrune(cur_stack)) return false;
  len = cur_pos;
  if (pjh.is_object()) {
    *json_str += "{";
    cur_pos += 1;
    if (pjh.down()) {
      int index = 0;
      do {
        // deal with commas
        if (index > 0) {
          *json_str += ",";
          cur_pos += 1;
        }
        index++;
        // get key
        std::string cur_key(pjh.get_string());
        *json_str += "\"";
        *json_str += cur_key;
        *json_str += "\"";
        cur_pos += cur_key.length();
        *json_str += ":";
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
              num_found++;
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
        bool keep = BuildSerialIndexListHelper(pjh, cur_stack, cur_pos,
                                               child_len, build, json_str,
                                               num_found);  // let us recurse
        if (should_insert) {
          (*build)[i].second = child_len;
        }
        if (!keep) {
          // undo stuff
          int to_del = cur_key.length() + 3;
          if (index > 1) to_del++;
          cur_pos -= to_del;
          json_str->resize(cur_pos);
          index--;
        }
        cur_stack.pop_back();
      } while (pjh.next());
      pjh.up();
    }
    *json_str += "}";
    cur_pos += 1;
  } else if (pjh.is_array()) {
    *json_str += "[";
    cur_pos += 1;
    if (pjh.down()) {
      int index = 0;
      do {
        if (index > 0) {
          *json_str += ",";
          cur_pos += 1;
        }
        index++;
        int child_len;
        BuildSerialIndexListHelper(pjh, cur_stack, cur_pos, child_len, build,
                                   json_str, num_found);  // let us recurse
      } while (pjh.next());
      pjh.up();
    }
    *json_str += "]";
    cur_pos += 1;
  } else {
    switch (pjh.get_type()) {
      case '"': {  // string
        std::string val(pjh.get_string());
        *json_str += "\"";
        *json_str += val;
        *json_str += "\"";
        cur_pos += val.length() + 2;
      } break;
      case 'l': {
        std::string val = std::to_string((int64_t)pjh.get_integer());
        *json_str += val;
        cur_pos += val.length();
      } break;
      case 'u': {
        std::string val = std::to_string((uint64_t)pjh.get_integer());
        *json_str += val;
        cur_pos += val.length();
      } break;
      case 'd': {
        std::string val = std::to_string((double)pjh.get_integer());
        *json_str += val;
        cur_pos += val.length();
      } break;
      case 'n': {  // we have a null
        std::string val = "null";
        *json_str += val;
        cur_pos += val.length();
      } break;
      case 't': {  // we have a true
        std::string val = "true";
        *json_str += val;
        cur_pos += val.length();
      } break;
      case 'f': {  // we have a false
        std::string val = "false";
        *json_str += val;
        cur_pos += val.length();
      } break;
      default:
        std::cerr << "Error in List Index Parsing!" << std::endl;
        *json_str += "";
    }
  }
  len = cur_pos - len;
  return true;
}

void PrintSerialIndexList(SerialIndexList indices) {
  std::cout << "Serialized Version:\n";
  std::cout << *indices.first << "\n";
  std::cout << "Keys:\n";
  for (int i = 0; i < indices.second->size(); ++i) {
    int pos = (*indices.second)[i].first;
    int len = (*indices.second)[i].second;
    std::string key;
    for (int j = 0; j < keys_to_keep[i].size(); j++) {
      if (j > 0) key += "_";
      key += keys_to_keep[i][j];
    }
    std::cout << "\"" << key << "\": " << indices.first->substr(pos, len)
              << "\n";
  }
}

read_key2_type ReadSerialIndexListField(SerialIndexList indices,
                                        read_key1_type &ret_key) {
  // deserialize
  int pos = (*indices.second)[key1_to_read_index].first;
  int len = (*indices.second)[key1_to_read_index].second;
  char *ret1 = new char[len - 1];
  memcpy(ret1, indices.first->substr(pos + 1, len - 2).c_str(), len - 1);
  ret_key = ret1;

  pos = (*indices.second)[key2_to_read_index].first;
  len = (*indices.second)[key2_to_read_index].second;
  char *ret2 = new char[len - 1];
  memcpy(ret2, indices.first->substr(pos + 1, len - 2).c_str(), len - 1);
  return ret2;
}

read_key2_type ReadSerialIndexListFieldRT(SerialIndexList indices,
                                          read_key1_type &ret_key,
                                          std::vector<std::string> key1,
                                          std::vector<std::string> key2) {
  std::string key1_str = "_" + std::to_string(key1.size());
  for (int i = 0; i < key1.size(); i++) {
    key1_str += "_";
    key1_str += key1[i];
  }
  std::string key2_str = "_" + std::to_string(key2.size());
  for (int i = 0; i < key2.size(); i++) {
    key2_str += "_";
    key2_str += key2[i];
  }

  int key1_index = offsets[key1_str];
  int key2_index = offsets[key2_str];

  // deserialize
  int pos = (*indices.second)[key1_index].first;
  int len = (*indices.second)[key1_index].second;
  char *ret1 = new char[len - 1];
  memcpy(ret1, indices.first->substr(pos + 1, len - 2).c_str(), len - 1);
  ret_key = ret1;

  pos = (*indices.second)[key2_index].first;
  len = (*indices.second)[key2_index].second;
  char *ret2 = new char[len - 1];
  memcpy(ret2, indices.first->substr(pos + 1, len - 2).c_str(), len - 1);
  return ret2;
}

int main(int argc, char *argv[]) {
#ifdef MEMORY
  cJSON_Hooks hooks = {.malloc_fn = track_malloc, .free_fn = track_free};
  cJSON_InitHooks(&hooks);
#endif
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
#ifdef MEMORY
  total_mem_usage = 0;
  track_mem = true;
#endif
#if TESTING == 1
  cJSON **arr;
  arr = new cJSON *[num_lines];
#elif TESTING == 2
  Hashmap **arr;
  arr = new Hashmap *[num_lines];
#elif TESTING == 3
  FlattenedStruct **arr;
  arr = new FlattenedStruct *[num_lines];
  // define the offset maps
#ifdef TWITTER
  offsets["_1_created_at"] = ((char *)&arr[0]->_1_created_at - (char *)arr[0]);
  offsets["_1_text"] = ((char *)&arr[0]->_1_text - (char *)arr[0]);
  offsets["_2_user_id_str"] =
      ((char *)&arr[0]->_2_user_id_str - (char *)arr[0]);
  offsets["_2_user_screen_name"] =
      ((char *)&arr[0]->_2_user_screen_name - (char *)arr[0]);
  offsets["_2_user_followers_count"] =
      ((char *)&arr[0]->_2_user_followers_count - (char *)arr[0]);
  offsets["_3_quoted_status_user_id_str"] =
      ((char *)&arr[0]->_3_quoted_status_user_id_str - (char *)arr[0]);
#else
  offsets["_1_user_id"] = ((char *)&arr[0]->_1_user_id - (char *)arr[0]);
  offsets["_1_business_id"] =
      ((char *)&arr[0]->_1_business_id - (char *)arr[0]);
  offsets["_1_text"] = ((char *)&arr[0]->_1_text - (char *)arr[0]);
  offsets["_1_date"] = ((char *)&arr[0]->_1_date - (char *)arr[0]);
  offsets["_1_compliment_count"] =
      ((char *)&arr[0]->_1_compliment_count - (char *)arr[0]);
#endif
#elif TESTING == 4
  SerialIndexList *arr;
  arr = new SerialIndexList[num_lines];
#ifdef TWITTER
  offsets["_1_created_at"] = 0;
  offsets["_1_text"] = 1;
  offsets["_2_user_id_str"] = 2;
  offsets["_2_user_screen_name"] = 3;
  offsets["_2_user_followers_count"] = 4;
  offsets["_3_quoted_status_user_id_str"] = 5;
#else
  offsets["_1_user_id"] = 0;
  offsets["_1_business_id"] = 1;
  offsets["_1_text"] = 2;
  offsets["_1_date"] = 3;
  offsets["_1_compliment_count"] = 4;
#endif
#else
#error "Unexpected value of TESTING"
#endif
#ifdef MEMORY
  track_mem = false;
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

#ifdef MEMORY
    track_mem = true;
#endif
    // build the four representations
#if TESTING == 1
    // 1: cJSON tree
    cJSON *test = BuildCJSON(pjh);
    if (test) arr[index++] = test;
#elif TESTING == 2
    // 2: nested hashmap
    Hashmap *hashmap = BuildHashmap(pjh);
    if (hashmap) arr[index++] = hashmap;
#elif TESTING == 3
    // 3: flattened struct
    FlattenedStruct *fs = BuildFlattenedStruct(pjh);
    if (fs) arr[index++] = fs;
#elif TESTING == 4
    // 4: indices in serialized rep
    SerialIndexList indices = BuildSerialIndexList(pjh);
    if (indices.second) arr[index++] = indices;
#else
#error "Invalid TESTING value"
#endif
#ifdef MEMORY
    track_mem = false;
#endif
  }
#ifdef TIMEBUILD
  double time_elapsed = timer.time();
  std::cout << "Time: " << time_elapsed << "s\n";
#endif
#ifdef MEMORY
  std::cout << "Memory: " << (total_mem_usage / 1000000000.0) << "GB\n";
#endif
  std::cout << "#Normal Case Tweets: " << index << "\n";

  // Read a value from the data
#ifdef TIMEACCESSCOMPILETIME
  read_key1_type *read_keys1 = new read_key1_type[index];
  read_key2_type *read_keys2 = new read_key2_type[index];
  timer.reset();
  for (int i = 0; i < index; i++) {
#if TESTING == 1
    read_keys2[i] = ReadCJSONField(arr[i], read_keys1[i]);
#elif TESTING == 2
    read_keys2[i] = ReadHashmapField(arr[i], read_keys1[i]);
#elif TESTING == 3
    read_keys2[i] = ReadFlattenedStructField(arr[i], read_keys1[i]);
#elif TESTING == 4
    read_keys2[i] = ReadSerialIndexListField(arr[i], read_keys1[i]);
#endif
    // std::cout << "READ: " << read_keys1[i] << ": " << read_keys2[i] << "\n";
  }
  double time_elapsed = timer.time();
  std::cout << "Time: " << time_elapsed << "s\n";
#endif

#ifdef TIMEACCESSRUNTIME
  read_key1_type *read_keys1 = new read_key1_type[index];
  read_key2_type *read_keys2 = new read_key2_type[index];
  std::vector<std::string> *keys1_runtime = new std::vector<std::string>[index];
  std::vector<std::string> *keys2_runtime = new std::vector<std::string>[index];
  for (int i = 0; i < index; i++) {
    keys1_runtime[i] = key1_to_read;
    keys2_runtime[i] = key2_to_read;
  }
  timer.reset();
  for (int i = 0; i < index; i++) {
#if TESTING == 1
    read_keys2[i] = ReadCJSONFieldRT(arr[i], read_keys1[i], keys1_runtime[i],
                                     keys2_runtime[i]);
#elif TESTING == 2
    read_keys2[i] = ReadHashmapFieldRT(arr[i], read_keys1[i], keys1_runtime[i],
                                       keys2_runtime[i]);
#elif TESTING == 3
    read_keys2[i] = ReadFlattenedStructFieldRT(
        arr[i], read_keys1[i], keys1_runtime[i], keys2_runtime[i]);
#elif TESTING == 4
    read_keys2[i] = ReadSerialIndexListFieldRT(
        arr[i], read_keys1[i], keys1_runtime[i], keys2_runtime[i]);
#endif
    // std::cout << "READ: " << read_keys1[i] << ": " << read_keys2[i] << "\n";
  }
  double time_elapsed = timer.time();
  std::cout << "Time: " << time_elapsed << "s\n";
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
