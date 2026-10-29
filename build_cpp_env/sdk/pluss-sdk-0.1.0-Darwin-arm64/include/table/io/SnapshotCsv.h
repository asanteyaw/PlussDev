#pragma once

// Small CSV "snapshot" files (manifests, schemas, link tables) are read with no
// type inference: every cell comes back as text, and the typed getters parse
// what they need. Shared by Nexus, Catalog persistence and Locus.

#include <cstdint>
#include <string>
#include <string_view>

#include "loader/Config.h"
#include "table/DataFrame.h"

namespace pluss::table::snapshot {

std::string trim_ascii(std::string_view text);
std::string lower_trimmed(std::string_view text);
pluss::loader::ReadCsvOptions snapshot_read_options();
// Read a small CSV with every column as text.
pluss::table::DataFrame load_snapshot_csv(std::string_view filepath);
std::string note_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row);
std::int64_t int_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row);
double double_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row);
bool bool_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row);

} // namespace pluss::table::snapshot
