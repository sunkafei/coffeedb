#ifndef DATABASE_GUARD
#define DATABASE_GUARD
#include "config.h"
class renderer;
void insert(int64_t id, const std::vector<std::pair<std::string, var>> &object);
std::vector<std::pair<int64_t, int64_t>> query();
std::vector<std::pair<int64_t, int64_t>> query(const std::string& key, const std::string& range);
std::vector<std::vector<std::pair<const std::string, var>>> select(const std::vector<std::pair<int64_t, int64_t>>& results, 
    const std::vector<std::string> &keys, const std::vector<std::pair<std::string, std::vector<std::string>>>& constraints,
    const std::string &left, const std::string &right);
std::vector<std::pair<const std::string, int64_t>> cluster(const std::vector<std::pair<int64_t, int64_t>>& results, 
    const std::string &field);
void remove(const std::vector<std::pair<int64_t, int64_t>>& result);
void build();
void clear();
void init();
void backup();
#endif