#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace pluss::table::io {

struct RawTable {
    std::vector<std::string> columns;
    // column name -> column values (all read as strings)
    std::unordered_map<std::string, std::vector<std::string>> data;
};

} // namespace pluss::table::io
