#include <fstream>

#include "check.h"
#include "file.h"

std::vector<char> read_binary(std::string_view path) {
    std::ifstream file{path.data(), std::ios::ate | std::ios::binary};
    CHECK(file.is_open(), "");
    long const file_size = file.tellg();
    std::vector<char> data((size_t) file_size);
    file.seekg(0);
    file.read(data.data(), file_size);
    return data;
}
