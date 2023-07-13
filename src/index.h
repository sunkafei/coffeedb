#ifndef INDEX_GUARD
#define INDEX_GUARD
#include <cstdint>
#include <vector>
#include <variant>
#include <memory>
class index {
public:
    index(){}
    index(const index&) = delete;
    index(index&&) = delete;
    index &operator= (const index&) = delete;
    index &operator= (index&&) = delete;
    virtual std::vector<std::pair<int64_t, int64_t>> query(const std::string &range) {
        throw std::logic_error("Unimplemented method index::query");
    }
    virtual void build() {
        throw std::logic_error("Unimplemented method index::build");
    }
    virtual ~index() {}
};
class integer_index : public index {
private:
    std::vector<std::pair<int64_t, int64_t>> data;
public:
    using value_type = int64_t;
    static constexpr int8_t number = 0;
    void add(int64_t id, int64_t value);
    void build() override;
    std::vector<std::pair<int64_t, int64_t>> query(const std::string &range) override;
};
class double_index : public index {
private:
    std::vector<std::pair<double, int64_t>> data;
public:
    using value_type = double;
    static constexpr int8_t number = 1;
    void add(int64_t id, double value);
    void build() override;
    std::vector<std::pair<int64_t, int64_t>> query(const std::string &range) override;
};
class string_index : public index {
private:
    uint64_t bits, mask;
    uint64_t size;
    std::vector<int64_t> ids;
    std::vector<std::string_view> data;
    std::variant<uint32_t*, uint64_t*> sa;
    inline std::string_view locate(auto position) noexcept {
        auto index1 = position & mask;
        auto index2 = position >> bits;
        return data[index1].substr(index2);
    }
public:
    using value_type = std::string;
    static constexpr int8_t number = 2;
    void add(int64_t id, std::string_view value);
    void build() override;
    std::vector<std::pair<int64_t, int64_t>> query(const std::string &range) override;
    ~string_index() override {
        std::visit([](auto *sa) {
            delete[] sa;
        }, sa);
    }
};
#endif