#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <cstdint>
#include <map>
#include <filesystem>
#include <memory>
#include <format>
#include <unordered_map>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include "config.h"
#include "utility.h"
#include "index.h"
std::map<std::string, std::unique_ptr<index>> indices;
std::unordered_map<int64_t, std::map<std::string, var>> data;
std::shared_mutex mutex_data;
std::shared_mutex mutex_files;
class ac_automaton {
public:
    static constexpr int32_t alphabet_size = 256;
    ac_automaton(const ac_automaton&) = delete;
    ac_automaton(ac_automaton&&) = delete;
    ac_automaton& operator= (const ac_automaton&) = delete;
    ac_automaton(const std::vector<std::string> &keywords) {
        int32_t total = 4;
        for (const auto &keyword : keywords) {
            total += keyword.size();
        }
        this->go = new int32_t[total][alphabet_size];
        this->fail = new int32_t[total];
        this->length = new int32_t[total];
        this->size = 1;
        for (int32_t i = 0; i < total; ++i) {
            this->fail[i] = 0;
            this->length[i] = 0;
            for (int32_t j = 0; j < alphabet_size; ++j) {
                this->go[i][j] = 0;
            }
        }
        for (const auto &keyword : keywords) {
            this->insert(keyword);
        }
        this->getfail();
    }
    ~ac_automaton() {
        delete[] go;
        delete[] fail;
        delete[] length;
    }
    std::string render(const std::string &text, const std::string &left, const std::string &right) const {
        int node = 0;
        std::string ret;
        std::vector<std::pair<uint64_t, uint64_t>> spans;
        for (uint64_t i = 0; i < text.size(); ++i) {
            auto c = idx(text[i]);
            node = go[node][c];
            if (length[node]) {
                auto begin = i - length[node] + 1;
                while (spans.size() && begin <= spans.back().first) {
                    spans.pop_back();
                }
                if (spans.size() && begin <= spans.back().second) {
                    spans.back().second = i;
                }
                else {
                    spans.emplace_back(begin, i);
                }
            }
        }
        ret.reserve(text.size() + (left.size() + right.size()) * spans.size() + 1);
        auto iter = spans.begin();
        for (uint64_t i = 0; i < text.size(); ++i) {
            if (iter != spans.end() && i == iter->first) {
                ret += left;
            }
            ret += text[i];
            if (iter != spans.end() && i == iter->second) {
                ret += right;
                ++iter;
            }
        }
        return ret;
    }
private:
    int32_t size;
	int32_t (*go)[256];
	int32_t *fail;
	int32_t *length;
	int32_t idx(int32_t c) const {
		return c - std::numeric_limits<char>::min();
	}
	void insert(const std::string& str) {
		int32_t u = 0;
		for (int32_t i = 0; i < ssize(str); i++) {
			int32_t c = idx(str[i]);
			if (!go[u][c]) {
				go[u][c] = size++;
			}
			u = go[u][c];
		}
		length[u] = str.size();
	}
	void getfail() {
		std::queue<int32_t> Q;
		fail[0] = 0;
		for (int32_t c = 0; c < alphabet_size; ++c) {
			int32_t u = go[0][c];
			if (u) {
                Q.push(u);
            }
		}
		while (!Q.empty()) {
			int32_t r = Q.front(); Q.pop();
			for (int32_t c = 0; c < alphabet_size; ++c) {
				int32_t u = go[r][c];
				if (!u) {
					go[r][c] = go[fail[r]][c];
					continue;
				}
				Q.push(u);
				int32_t v = fail[r];
				while (v && !go[v][c]) {
                    v = fail[v];
                }
				fail[u] = go[v][c];
                length[u] = std::max(length[u], length[fail[u]]);
			}
		}
	}
};
class renderer {
private:
    std::map<std::string, ac_automaton> automatons;
    std::string left;
    std::string right;
public:
    renderer(const auto& container, const std::string& left, const std::string& right) : left(left), right(right) {
        for (const auto& [key, keywords] : container) {
            if (indices.contains(key) && dynamic_cast<string_index*>(indices[key].get()) != nullptr) {
                automatons.emplace(key, keywords);
            }
        }
    }
    var operator()(const std::string& key, const var &data) const {
        return std::visit([this, &key]<typename T>(const T& value) -> var {
            if constexpr (std::is_same_v<T, std::string>) {
                if (automatons.contains(key)) {
                    return automatons.at(key).render(value, left, right);
                }
            }
            return value;
        }, data);
    }
    var operator()(const auto& pair) const {
        return operator()(pair.first, pair.second);
    }
};
void init() {
    std::filesystem::create_directory(storage_location + backup_directory);
    std::filesystem::create_directory(storage_location + raw_directory);
}
void build() {
    std::map<std::string, std::unique_ptr<index>> indices;
    std::unordered_map<int64_t, std::map<std::string, var>> data;
    for (std::unique_lock lock(mutex_files); const auto &e : std::filesystem::directory_iterator(storage_location + raw_directory)) {
        auto path = e.path().lexically_normal().string();
        FILE *fp = fopen(path.c_str(), "rb");
        if (!fp) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        auto deleter = [](FILE* file){ fclose(file); };
        const std::unique_ptr<FILE, decltype(deleter)> file_guard(fp);
        bool success = true;
        int64_t id;
        int32_t size;
        success |= (fread(&id, sizeof(id), 1, fp) == sizeof(id));
        success |= (fread(&size, sizeof(size), 1, fp) == sizeof(size));
        if (size <= 0) {
            success = false;
        }
        for (int i = 0; i < size; ++i) {
            int32_t length;
            success |= (fread(&length, sizeof(length), 1, fp) == sizeof(length));
            if (length <= 0) {
                success = false;
                break;
            }
            std::string key(length, '*');
            success |= (fread(key.data(), sizeof(char), length, fp) == sizeof(char) * length);
            int8_t type;
            success |= (fread(&type, sizeof(type), 1, fp) == sizeof(type));
            if (type == integer_index::number) {
                int64_t value;
                success |= (fread(&value, sizeof(value), 1, fp) == sizeof(value));
                if (!indices.count(key)) {
                    indices[key] = std::make_unique<integer_index>();
                }
                auto *ptr = dynamic_cast<integer_index*>(indices[key].get());
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
                else {
                    data[id][key] = value;
                    ptr->add(id, value);
                }
            }
            else if (type == double_index::number) {
                double value;
                success |= (fread(&value, sizeof(value), 1, fp) == sizeof(value));
                if (!indices.count(key)) {
                    indices[key] = std::make_unique<double_index>();
                }
                auto *ptr = dynamic_cast<double_index*>(indices[key].get());
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
                else {
                    data[id][key] = value;
                    ptr->add(id, value);
                }
            }
            else if (type == string_index::number) {
                int32_t length;
                success |= (fread(&length, sizeof(length), 1, fp) == sizeof(length));
                if (length <= 0) {
                    success = false;
                    break;
                }
                std::string value(length, '*');
                success |= (fread(value.data(), sizeof(char), length, fp) == sizeof(char) * length);
                if (!indices.count(key)) {
                    indices[key] = std::make_unique<string_index>();
                }
                auto *ptr = dynamic_cast<string_index*>(indices[key].get());
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
                else {
                    value.shrink_to_fit();
                    data[id][key] = std::move(value);
                    ptr->add(id, std::get<std::string>(data[id][key]));
                }
            }
            else {
                success = false;
                break;
            }
        }
        if (!success) {
            throw std::runtime_error("Corrupted File: " + path);
        }
    }
    for (auto &[key, value] : indices) {
        value->build();
    }
    std::unique_lock lock(mutex_data);
    ::indices = std::move(indices);
    ::data = std::move(data);
}
void insert(int64_t id, const std::vector<std::pair<std::string, var>> &object) {
    for (std::unique_lock lock(mutex_data); const auto &[key, value] : object) {
        if (!indices.count(key)) {
            std::visit([&key]<typename T>(const T& value){
                if constexpr (std::is_same_v<T, int64_t>) {
                    indices[key] = std::make_unique<integer_index>();
                }
                else if constexpr (std::is_same_v<T, double>) {
                    indices[key] = std::make_unique<double_index>();
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    indices[key] = std::make_unique<string_index>();
                }
                else {
                    static_assert(false, "Non-exhaustive visitor!");
                }
            }, value);
        }
        else {
            std::visit([&key]<typename T>(const T& value){
                void *ptr = nullptr;
                if constexpr (std::is_same_v<T, int64_t>) {
                    ptr = dynamic_cast<integer_index*>(indices[key].get());
                }
                else if constexpr (std::is_same_v<T, double>) {
                    ptr = dynamic_cast<double_index*>(indices[key].get());
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    ptr = dynamic_cast<string_index*>(indices[key].get());
                }
                else {
                    static_assert(false, "Non-exhaustive visitor!");
                }
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
            }, value);
        }
    }
    std::shared_lock lock(mutex_files);
    std::string filename = storage_location + raw_directory + std::to_string(id);
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    auto deleter = [](FILE* file){ fclose(file); };
    const std::unique_ptr<FILE, decltype(deleter)> file_guard(fp);
    int32_t size = object.size();
    fwrite(&id, sizeof(id), 1, fp);
    fwrite(&size, sizeof(size), 1, fp);
    for (int i = 0; i < size; ++i) {
        const auto &[key, value] = object[i];
        int32_t length = key.size();
        fwrite(&length, sizeof(length), 1, fp);
        fwrite(key.data(), sizeof(char), length, fp);
        std::visit([fp](auto &&value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int64_t>) {
                const int8_t type = integer_index::number;
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&value, sizeof(value), 1, fp);
            }
            else if constexpr (std::is_same_v<T, double>) {
                const int8_t type = double_index::number;
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&value, sizeof(value), 1, fp);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                const int8_t type = string_index::number;
                const int32_t length = value.size();
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&length, sizeof(length), 1, fp);
                fwrite(value.data(), sizeof(char), length, fp);
            }
            else {
                static_assert(false, "Non-exhaustive visitor!");
            }
        }, value);
    }
}
std::vector<std::pair<int64_t, int64_t>> query() {
    std::vector<std::pair<int64_t, int64_t>> ret;
    for (const auto &[id, _] : data) {
        ret.emplace_back(id, 0);
    }
    return ret;
}
std::vector<std::pair<int64_t, int64_t>> query(const std::string& key, const std::string& range) {
    if (!indices.count(key)) {
        return {};
    }
    std::shared_lock lock(mutex_data);
    return indices[key]->query(range);
}
std::vector<std::vector<std::pair<const std::string, var>>> select(const std::vector<std::pair<int64_t, int64_t>>& results, 
    const std::vector<std::string> &keys, const std::vector<std::pair<std::string, std::vector<std::string>>>& constraints,
    const std::string &left, const std::string &right) {
    std::shared_lock lock(mutex_data);
    renderer transformer(constraints, left, right);
    std::vector<std::vector<std::pair<const std::string, var>>> ret;
    auto flag = (keys.empty() || std::find(keys.cbegin(), keys.cend(), key_correlation) != keys.end());
    for (auto [id, correlation] : results) {
        std::vector<std::pair<const std::string, var>> object;
        if (constraints.size()) {
            if (keys.size()) {
                for (const auto &key : keys) {
                    auto iter = data[id].find(key);
                    if (iter != data[id].end()) {
                        object.emplace_back(key, transformer(*iter));
                    }
                }
            }
            else {
                for (auto iter = data[id].begin(); iter != data[id].end(); ++iter) {
                    object.emplace_back(iter->first, transformer(*iter));
                }
            }
        }
        else {
            if (keys.size()) {
                for (const auto &key : keys) {
                    auto iter = data[id].find(key);
                    if (iter != data[id].end()) {
                        object.push_back(*iter);
                    }
                }
            }
            else {
                for (auto iter = data[id].begin(); iter != data[id].end(); ++iter) {
                    object.push_back(*iter);
                }
            }
        }
        if (correlation && flag) {
            object.emplace_back(key_correlation, correlation);
        }
        if (object.size()) {
            ret.push_back(std::move(object));
        }
    }
    return ret;
}
void remove(const std::vector<std::pair<int64_t, int64_t>>& result) {
    std::shared_lock lock(mutex_files);
    for (auto [id, _] : result) {
        std::filesystem::remove(storage_location + raw_directory + std::format("{}", id));
    }
}
void clear() {
    std::filesystem::remove_all(storage_location + raw_directory);
    std::filesystem::create_directory(storage_location + raw_directory);
    std::filesystem::remove_all(storage_location + backup_directory);
    std::filesystem::create_directory(storage_location + backup_directory);
}