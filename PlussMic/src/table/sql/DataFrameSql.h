#pragma once

#include <string_view>

#include "table/DataFrame.h"

namespace pluss::table {

class DataFrameSqlAccessor {
public:
  DataFrameSqlAccessor(DataFrame frame, DataFrame::SqlOptions options = {});

  DataFrame execute(std::string_view statement) const;

private:
  DataFrame frame_{};
  DataFrame::SqlOptions options_{};
};

} // namespace pluss::table
