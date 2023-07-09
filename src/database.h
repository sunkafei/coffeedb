#ifndef DATABASE_GUARD
#define DATABASE_GUARD
void insert(int64_t id, const std::vector<std::pair<std::string, var>> &object);
std::vector<std::pair<int64_t, int64_t>> query(const std::string& key, const std::string& range);
std::vector<std::vector<std::pair<const std::string, var>>> select(const std::vector<std::pair<int64_t, int64_t>>& results, const std::vector<std::string> &keys);
void build();
#endif