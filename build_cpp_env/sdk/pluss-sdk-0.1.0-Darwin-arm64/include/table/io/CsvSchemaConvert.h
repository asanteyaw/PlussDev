#pragma once

#include "loader/CsvLoader.h"

namespace pluss::table::detail {

pluss::table::Thread infer_thread_from_raw_csv_column(std::string_view col_name,
                                                       std::size_t source_col_index,
                                                       const std::vector<std::string>& raw_values,
                                                       const pluss::loader::ReadCsvOptions& options);
pluss::table::Thread infer_thread_from_raw_csv_column(const pluss::loader::CsvRawColumn& column,
                                                       const pluss::loader::ReadCsvOptions& options);

pluss::loader::CsvBatch csv_batch_from_raw_batch(const pluss::loader::CsvRawBatch& raw,
                                                 const pluss::loader::ReadCsvOptions& options,
                                                 double* infer_columns_ms = nullptr);

} // namespace pluss::table::detail
