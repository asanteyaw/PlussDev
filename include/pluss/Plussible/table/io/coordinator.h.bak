#pragma once
#include <memory>
#include "datalog/table/io/raw_table.h"
#include "datalog/table/dataframe/dataframe.h"

namespace pluss::table::io {

// Convert a RawTable (strings-only) into a DataFrame.
// Current implementation does basic numeric inference and converts numeric
// columns to torch::Series. Later this coordinator will map textual columns
// to `pluss::Note` and temporal columns to `pluss::Timeline`.
std::shared_ptr<pluss::table::DataFrame> convert_raw_to_dataframe(const RawTable& raw);

} // namespace pluss::table::io
