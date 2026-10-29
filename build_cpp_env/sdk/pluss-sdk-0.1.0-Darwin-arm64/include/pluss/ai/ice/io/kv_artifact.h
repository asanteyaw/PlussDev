#pragma once

// Shared text key=value artifact primitives -- the format every ice::models::
// catalog entry's save()/load() uses to persist its own flat Options fields
// (and, for TextJudge, a label vocabulary) alongside the actual tensor
// weights, which go through ice::io::save_parameters/load_parameters
// separately. Previously duplicated verbatim across
// Ice/models/nlp/text_judge.cpp and Ice/models/rl/action_value_agent.cpp;
// pulled out here so every current and future catalog entry shares one
// implementation instead of hand-rolling its own parser.
//
// Not part of the public ice::io surface (ice::io::save_parameters/
// load_parameters/save_checkpoint/load_checkpoint remain the only things a
// caller uses directly) -- this is the internal building block those and
// every models_* save()/load() are written in terms of.

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pluss::ai::ice::io::detail {

using KvMap = std::unordered_map<std::string, std::string>;

// Writes `key="value"` (quoted, so a value may itself contain '=' or
// whitespace) followed by a newline.
void write_kv_line(std::ostringstream& out, std::string_view key, std::string_view value);

// Writes `key=0` or `key=1`.
void write_bool_line(std::ostringstream& out, std::string_view key, bool value);

// Writes `key=<value>` via operator<<, unquoted -- for any type whose stream
// insertion round-trips through parse via std::sto*/std::stoull (integers,
// floating point).
template <class T>
void write_num_line(std::ostringstream& out, std::string_view key, T value) {
  out << key << '=' << value << '\n';
}

// Parses a body of `key=value`/`key="value"` lines (one per line, blank
// lines skipped) into a lookup map. `context` is prefixed onto the "malformed
// line" error message (e.g. "pluss::ai::ice::models::TextJudge::load").
[[nodiscard]] KvMap parse_kv_body(std::string_view body, std::string_view context);

// Each throws (`context`-prefixed) when `key` is absent, or when present but
// not parseable as the requested type.
[[nodiscard]] std::string require_field(const KvMap& kv, std::string_view key, std::string_view context);
[[nodiscard]] std::int64_t require_i64(const KvMap& kv, std::string_view key, std::string_view context);
[[nodiscard]] std::uint64_t require_u64(const KvMap& kv, std::string_view key, std::string_view context);
[[nodiscard]] double require_f64(const KvMap& kv, std::string_view key, std::string_view context);
[[nodiscard]] bool require_bool(const KvMap& kv, std::string_view key, std::string_view context);

// Reads `count` fields named `<prefix>.0`, `<prefix>.1`, ... -- the
// companion read for a caller that wrote an indexed list of plain strings
// (e.g. TextJudge's label vocabulary) via a loop of write_kv_line calls.
[[nodiscard]] std::vector<std::string> collect_indexed(
    const KvMap& kv, std::string_view prefix, std::int64_t count, std::string_view context);

} // namespace pluss::ai::ice::io::detail
