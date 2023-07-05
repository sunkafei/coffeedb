#ifndef INTERFACE_GUARD
#define INTERFACE_GUARD
#include "utility.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
void init();
void evaluate(const json &command);
#endif
