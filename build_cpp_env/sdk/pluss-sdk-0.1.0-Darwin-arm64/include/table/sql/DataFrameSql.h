#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "table/DataFrame.h"

namespace pluss::table {

namespace detail {
std::string sql_primary_source_name(std::string_view statement,
                                    const DataFrame::SqlOptions& options = {});
}

class DataFrameSqlPrepared {
public:
  DataFrameSqlPrepared(DataFrame frame,
                       DataFrame::SqlOptions options,
                       std::string statement_template);

  DataFrame execute() const;
  DataFrame execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const;
  DataFrame execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params,
                    std::unordered_map<std::string, std::string> identifier_params) const;

private:
  DataFrame frame_{};
  DataFrame::SqlOptions options_{};
  std::string statement_template_{};
};

class DataFrameSqlAccessor {
public:
  DataFrameSqlAccessor(DataFrame frame, DataFrame::SqlOptions options = {});

  DataFrame execute(std::string_view statement) const;
  DataFrameSqlPrepared prepare(std::string_view statement) const;

private:
  DataFrame frame_{};
  DataFrame::SqlOptions options_{};
};

} // namespace pluss::table
