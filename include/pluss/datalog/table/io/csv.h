#pragma once
#include <memory>
#include <string>
#include "table/dataframe.h"

namespace pluss::table::io {

std::shared_ptr<DataFrame> load_csv(const std::string& filename);

}  // namespace pluss::table::io