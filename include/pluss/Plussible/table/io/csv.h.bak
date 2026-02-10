#pragma once
#include <memory>
#include <string>
#include "datalog/table/dataframe/dataframe.h"
#include "datalog/table/io/raw_table.h"

namespace pluss::table::io {

// Read CSV into a RawTable (strings only). Use `convert_raw_to_dataframe`
// from the coordinator to turn that raw data into a `DataFrame`.
RawTable load_csv_raw(const std::string& filename);

// Backwards-compatible loader: reads CSV and converts to DataFrame using coordinator.
std::shared_ptr<DataFrame> load_csv(const std::string& filename);

}  // namespace pluss::table::io