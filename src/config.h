#ifndef CONFIG_GUARD
#define CONFIG_GUARD
#include <string>
#include <variant>
#include <cstdint>
#include <filesystem>
using var = std::variant<int64_t, double, std::string>;
inline constexpr std::string key_correlation("$correlation");
inline std::string storage_location = std::filesystem::current_path().string() + "/";
inline std::string backup_directory = "backup/";
inline std::string raw_directory = "raw/";
inline int port = 14920;
#endif