#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "datalog/table/dataframe/dataframe.h"
#include "datalog/table/io/raw_table.h"

namespace pluss::table::io {

// Read Excel (.xlsx) into a RawTable (strings only). 
// Uses libzip to extract the .xlsx archive and parses the XML content.
// sheet_name: If empty, reads the first sheet. Otherwise, reads the specified sheet by name.
// Use `convert_raw_to_dataframe` from the coordinator to turn that raw data into a `DataFrame`.
RawTable load_excel_raw(const std::string& filename, const std::string& sheet_name = "");

// High-level loader: reads Excel and converts to DataFrame using coordinator.
// sheet_name: If empty, reads the first sheet. Otherwise, reads the specified sheet by name.
std::shared_ptr<DataFrame> load_excel(const std::string& filename, const std::string& sheet_name = "");

// List all sheet names in an Excel workbook
std::vector<std::string> list_excel_sheets(const std::string& filename);

// Load all sheets from an Excel workbook into a map of sheet_name -> DataFrame
std::unordered_map<std::string, std::shared_ptr<DataFrame>> load_excel_all_sheets(const std::string& filename);

}  // namespace pluss::table::io