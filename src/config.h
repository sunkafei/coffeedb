#ifndef CONFIG_GUARD
#define CONFIG_GUARD
#include <string>
#include <variant>
#include <cstdint>
#include <filesystem>
using var = std::variant<bool, int64_t, double, std::string>;
inline constexpr std::string key_correlation("$correlation");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
constexpr std::string seperator("\\");
#else
constexpr std::string seperator("/");
#endif
inline std::string storage_location = std::filesystem::current_path().string() + seperator;
inline std::string backup_directory = "backup" + seperator;
inline std::string raw_directory = "raw" + seperator;
inline int port = 14920;
#endif