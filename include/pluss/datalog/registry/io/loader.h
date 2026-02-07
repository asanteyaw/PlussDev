#pragma once

#include <string>
#include "item/archive.h"

namespace pluss::item::io {

Archive load_from_file(const std::string& format, const std::string& path);
Archive parse_from_string(const std::string& format, const std::string& raw);

void save_to_file(const Archive& archive, const std::string& format, const std::string& path);
std::string dump_to_string(const Archive& archive, const std::string& format);

}  // namespace pluss::entry::io