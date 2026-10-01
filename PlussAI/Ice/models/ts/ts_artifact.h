#pragma once

// Shared save()/load() persistence helpers for the models_ts catalog
// (AutoregressiveForecaster, TemporalFusionTransformer) -- both need to
// persist the same data::CategoricalVariable vectors (static/encoder/decoder
// categoricals) alongside their own flat Options fields, on top of the
// generic key=value primitives in pluss/ai/ice/io/kv_artifact.h.

#include "pluss/ai/ice/data.h"
#include "pluss/ai/ice/io/kv_artifact.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::ai::ice::models::ts_artifact {

inline void write_categoricals(std::ostringstream& out,
                               std::string_view prefix,
                               const std::vector<data::CategoricalVariable>& values) {
  io::detail::write_num_line(out, std::string(prefix) + ".count",
                             static_cast<std::int64_t>(values.size()));
  for (std::size_t i = 0; i < values.size(); ++i) {
    const std::string entry = std::string(prefix) + "." + std::to_string(i);
    io::detail::write_num_line(out, entry + ".variable_index", values[i].variable_index);
    io::detail::write_num_line(out, entry + ".cardinality", values[i].cardinality);
  }
}

[[nodiscard]] inline std::vector<data::CategoricalVariable> read_categoricals(
    const io::detail::KvMap& kv, std::string_view prefix, std::string_view context) {
  const auto count = io::detail::require_i64(kv, std::string(prefix) + ".count", context);
  std::vector<data::CategoricalVariable> out{};
  out.reserve(static_cast<std::size_t>(count));
  for (std::int64_t i = 0; i < count; ++i) {
    const std::string entry = std::string(prefix) + "." + std::to_string(i);
    out.push_back(data::CategoricalVariable{
        .variable_index = io::detail::require_i64(kv, entry + ".variable_index", context),
        .cardinality = io::detail::require_i64(kv, entry + ".cardinality", context),
    });
  }
  return out;
}

} // namespace pluss::ai::ice::models::ts_artifact
