#pragma once

#include "loader/CsvLoader.h"

namespace pluss::table::detail {

// prefer_numeric_over_bool: when true, tries Int64 before Bool for ambiguous
// digit-only literals ("1"/"0" and similar), rather than pandas-style Bool-first
// priority. Callers that infer a whole column at once (the default) want Bool-first
// so a genuine 1/0-encoded boolean column still resolves to Bool. Callers that infer
// a single cell in isolation, with no sibling rows to disambiguate, want the opposite
// so a lone "1" reads as an integer rather than a boolean.
pluss::table::Thread infer_thread_from_raw_csv_column(std::string_view col_name,
                                                       std::size_t source_col_index,
                                                       const std::vector<std::string>& raw_values,
                                                       const pluss::loader::ReadCsvOptions& options,
                                                       bool prefer_numeric_over_bool = false);
pluss::table::Thread infer_thread_from_raw_csv_column(const pluss::loader::CsvRawColumn& column,
                                                       const pluss::loader::ReadCsvOptions& options,
                                                       bool prefer_numeric_over_bool = false);

pluss::loader::CsvBatch csv_batch_from_raw_batch(const pluss::loader::CsvRawBatch& raw,
                                                 const pluss::loader::ReadCsvOptions& options,
                                                 double* infer_columns_ms = nullptr);

} // namespace pluss::table::detail
