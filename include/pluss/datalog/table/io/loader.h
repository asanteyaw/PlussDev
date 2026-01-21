#pragma once
#include <memory>
#include <string>
#include "table/dataframe.h"

namespace pluss::table::io {

// Dispatcher
std::shared_ptr<DataFrame> load(const std::string& format, const std::string& filename);

}  // namespace pluss::table::io