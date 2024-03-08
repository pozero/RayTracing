#pragma once

#include <vector>
#include <cstdint>
#include <string_view>

#define PATH_FROM_ROOT(REL_PATH) ROOT_PATH REL_PATH
#define PATH_FROM_BINARY(REL_PATH) BINARY_PATH REL_PATH

std::vector<char> read_binary(std::string_view path);
