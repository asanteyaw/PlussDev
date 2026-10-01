#include "pluss/ai/ice/io/kv_artifact.h"

#include <CodeM/core/Errors.h>

#include <iomanip>
#include <sstream>

namespace pluss::ai::ice::io::detail {

void write_kv_line(std::ostringstream& out, std::string_view key, std::string_view value) {
  out << key << '=';
  out << std::quoted(std::string(value)) << '\n';
}

void write_bool_line(std::ostringstream& out, std::string_view key, bool value) {
  out << key << '=' << (value ? 1 : 0) << '\n';
}

KvMap parse_kv_body(std::string_view body, std::string_view context) {
  KvMap out{};
  std::istringstream stream{std::string(body)};
  std::string line{};
  while (std::getline(stream, line)) {
    if (line.empty()) continue;
    const auto eq = line.find('=');
    PLUSS_CHECK(eq != std::string::npos,
                std::string(context) + ": malformed metadata line '" + line + "'.");
    std::string key = line.substr(0, eq);
    std::string value_text = line.substr(eq + 1);
    std::string value{};
    std::istringstream value_stream(value_text);
    if (!value_text.empty() && value_text.front() == '"') {
      value_stream >> std::quoted(value);
    } else {
      value = value_text;
    }
    out.emplace(std::move(key), std::move(value));
  }
  return out;
}

std::string require_field(const KvMap& kv, std::string_view key, std::string_view context) {
  const auto it = kv.find(std::string(key));
  PLUSS_CHECK(it != kv.end(),
              std::string(context) + ": missing field '" + std::string(key) + "'.");
  return it->second;
}

std::int64_t require_i64(const KvMap& kv, std::string_view key, std::string_view context) {
  return std::stoll(require_field(kv, key, context));
}

std::uint64_t require_u64(const KvMap& kv, std::string_view key, std::string_view context) {
  return static_cast<std::uint64_t>(std::stoull(require_field(kv, key, context)));
}

double require_f64(const KvMap& kv, std::string_view key, std::string_view context) {
  return std::stod(require_field(kv, key, context));
}

bool require_bool(const KvMap& kv, std::string_view key, std::string_view context) {
  const auto value = require_field(kv, key, context);
  return value == "1" || value == "true";
}

std::vector<std::string> collect_indexed(
    const KvMap& kv, std::string_view prefix, std::int64_t count, std::string_view context) {
  std::vector<std::string> out(static_cast<std::size_t>(count < 0 ? 0 : count));
  for (std::int64_t i = 0; i < count; ++i) {
    out[static_cast<std::size_t>(i)] =
        require_field(kv, std::string(prefix) + "." + std::to_string(i), context);
  }
  return out;
}

} // namespace pluss::ai::ice::io::detail
