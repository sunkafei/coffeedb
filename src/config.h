#ifndef CONFIG_GUARD
#define CONFIG_GUARD
#include <string>
#include <variant>
#include <cstdint>
using var = std::variant<int64_t, double, std::u32string>;
inline std::string storage_location = "./";
inline std::string backup_directory = "backup/";
inline std::string raw_directory = "raw/";
enum class type : int8_t {integer, real, string};
#endif