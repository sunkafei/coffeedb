#ifndef INDEX_GUARD
#define INDEX_GUARD
#include <cstdint>
#include <vector>
class index {
public:
    virtual std::vector<int64_t> query(const std::string &range) {
        throw std::logic_error("Unimplemented method index::query");
    }
    virtual void build() {
        throw std::logic_error("Unimplemented method index::build");
    }
};
class integer_index : public index {
private:
    std::vector<std::pair<int64_t, int64_t>> data;
public:
    static constexpr int8_t number = 0;
    void add(int64_t id, int64_t value);
    void build() override;
    std::vector<int64_t> query(const std::string &range) override;
};
class double_index : public index {
private:
    std::vector<std::pair<double, int64_t>> data;
public:
    static constexpr int8_t number = 1;
    void add(int64_t id, double value);
    void build() override;
    std::vector<int64_t> query(const std::string &range) override;
};
class string_index : public index {
private:
    std::vector<std::pair<std::u32string, int64_t>> data;
public:
    static constexpr int8_t number = 2;
    void add(int64_t id, const std::string &value);
    void build() override;
    std::vector<int64_t> query(const std::string &range) override;
};
#endif