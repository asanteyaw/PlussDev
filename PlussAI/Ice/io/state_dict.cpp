#include "pluss/ai/ice/io.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace pluss::ai::ice::io {

namespace {

constexpr std::string_view kMagicLine = "pluss_ice_model";
constexpr std::int64_t kCurrentSchema = 1;

// FNV-1a 64-bit: small, dependency-free, and already proven correct in
// NodeB's own artifact formats (PlussAI/NodeB/io/io_detail.cpp). Ice can't
// depend on NodeB (NodeB depends on ice, never the reverse), so this is a
// deliberate small duplication of an already-trivial primitive rather than
// a shared dependency in either direction — see ICE_BLUEPRINT.md §9.2.
[[nodiscard]] std::uint64_t fnv1a64(std::string_view data) noexcept {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffsetBasis;
  for (const unsigned char byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  return hash;
}

[[nodiscard]] std::string to_hex(std::uint64_t value) {
  std::ostringstream oss;
  oss << std::hex << std::setw(16) << std::setfill('0') << value;
  return oss.str();
}

[[nodiscard]] std::uint64_t parse_hex_u64(std::string_view value) {
  return static_cast<std::uint64_t>(std::stoull(std::string(value), nullptr, 16));
}

[[nodiscard]] std::string join_doubles(const std::vector<double>& values) {
  std::ostringstream oss{};
  oss << std::setprecision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

[[nodiscard]] std::vector<double> parse_doubles(std::string_view line) {
  std::vector<double> out{};
  std::istringstream iss{std::string(line)};
  double value = 0.0;
  while (iss >> value) out.push_back(value);
  return out;
}

} // namespace

void save_parameters(std::string_view filepath, const std::vector<Series*>& parameters) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(), "pluss::ai::ice::io::save_parameters: output path cannot be empty.");
  PLUSS_CHECK(!parameters.empty(), "pluss::ai::ice::io::save_parameters: no parameters to save.");

  std::ostringstream body{};
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    const auto* param = parameters[i];
    const auto shape_view = param->sizes();
    const std::vector<double> shape_as_double(shape_view.begin(), shape_view.end());
    // .host<double>() already migrates off-device and converts dtype
    // internally (see Series::item<T>()/host<T>()) — no explicit
    // .to(pCPU, ...) needed regardless of which device `param` lives on.
    const auto flat = param->reshape({param->numel()}).host<double>();
    body << "param_" << i << "_shape=" << join_doubles(shape_as_double) << "\n";
    body << "param_" << i << "=" << join_doubles(flat) << "\n";
  }
  const auto body_text = body.str();

  std::ofstream out{std::string(filepath)};
  PLUSS_CHECK(out.good(),
             "pluss::ai::ice::io::save_parameters: failed to open '" + std::string(filepath) + "'.");
  out << kMagicLine << "\n";
  out << "schema=" << kCurrentSchema << "\n";
  out << "param_count=" << parameters.size() << "\n";
  out << "checksum=" << to_hex(fnv1a64(body_text)) << "\n";
  out << body_text;
  PLUSS_CHECK(out.good(),
             "pluss::ai::ice::io::save_parameters: failed while writing '" + std::string(filepath) + "'.");
}

void load_parameters(std::string_view filepath, const std::vector<Series*>& parameters) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(), "pluss::ai::ice::io::load_parameters: input path cannot be empty.");
  std::ifstream in{std::string(filepath)};
  PLUSS_CHECK(in.good(),
             "pluss::ai::ice::io::load_parameters: failed to open '" + std::string(filepath) + "'.");

  std::string line{};
  PLUSS_CHECK(std::getline(in, line) && line == kMagicLine,
             "pluss::ai::ice::io::load_parameters: invalid model artifact header.");

  std::int64_t schema = -1;
  std::int64_t param_count = -1;
  std::uint64_t expected_checksum = 0;
  bool have_checksum = false;
  while (std::getline(in, line)) {
    if (line.rfind("schema=", 0) == 0) {
      schema = std::stoll(line.substr(7));
      continue;
    }
    if (line.rfind("param_count=", 0) == 0) {
      param_count = std::stoll(line.substr(12));
      continue;
    }
    if (line.rfind("checksum=", 0) == 0) {
      expected_checksum = parse_hex_u64(std::string_view(line).substr(9));
      have_checksum = true;
      break; // checksum is always the last header line; the body follows immediately.
    }
    PLUSS_FAIL("pluss::ai::ice::io::load_parameters: unrecognized header line '" + line + "'.");
  }
  PLUSS_CHECK(schema == kCurrentSchema,
             "pluss::ai::ice::io::load_parameters: unsupported schema " + std::to_string(schema) + ".");
  PLUSS_CHECK(param_count >= 0 && have_checksum,
             "pluss::ai::ice::io::load_parameters: model artifact header is incomplete.");
  PLUSS_CHECK(
      static_cast<std::int64_t>(parameters.size()) == param_count,
      "pluss::ai::ice::io::load_parameters: expected " + std::to_string(parameters.size()) +
          " parameters (does the reconstructed model match the saved architecture?) but the artifact "
          "has " + std::to_string(param_count) + ".");

  std::ostringstream body_buffer{};
  body_buffer << in.rdbuf();
  const auto body_text = body_buffer.str();
  PLUSS_CHECK(fnv1a64(body_text) == expected_checksum,
             "pluss::ai::ice::io::load_parameters: checksum mismatch, artifact is truncated or corrupted.");

  std::vector<std::string> shape_lines(static_cast<std::size_t>(param_count));
  std::vector<std::string> value_lines(static_cast<std::size_t>(param_count));
  std::istringstream body_stream(body_text);
  while (std::getline(body_stream, line)) {
    const auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = line.substr(0, eq);
    const std::string_view value(line.data() + eq + 1, line.size() - eq - 1);
    if (key.rfind("param_", 0) != 0) continue;
    const std::string_view rest = std::string_view(key).substr(6);
    std::size_t digits = 0;
    while (digits < rest.size() && rest[digits] >= '0' && rest[digits] <= '9') ++digits;
    if (digits == 0) continue;
    const auto idx = static_cast<std::size_t>(std::stoll(std::string(rest.substr(0, digits))));
    if (idx >= shape_lines.size()) continue;
    if (rest.substr(digits) == "_shape") {
      shape_lines[idx] = std::string(value);
    } else if (rest.substr(digits).empty()) {
      value_lines[idx] = std::string(value);
    }
  }

  for (std::int64_t i = 0; i < param_count; ++i) {
    const auto idx = static_cast<std::size_t>(i);
    const auto shape_doubles = parse_doubles(shape_lines[idx]);
    const std::vector<std::int64_t> shape(shape_doubles.begin(), shape_doubles.end());
    auto values = parse_doubles(value_lines[idx]);
    auto* param = parameters[idx];
    PLUSS_CHECK(
        static_cast<std::int64_t>(values.size()) == param->numel(),
        "pluss::ai::ice::io::load_parameters: parameter " + std::to_string(i) +
            " size mismatch (does the reconstructed model match the saved architecture?).");
    // Reconstructed on whichever device/dtype `param` already has — not
    // hardcoded to CPU/Float64 — since `make_series` infers dtype from the
    // `double` vector regardless of `cfg` (see the historical dtype bug
    // this exact pattern fixed in NodeB's EmbeddingTable), the explicit
    // `.to(cfg.platform, cfg.dtype)` is load-bearing, not decorative.
    const auto cfg = Config{}.with_dtype(param->scalar_type()).with_platform(param->platform());
    *param = pluss::make_series(values, shape, cfg).to(cfg.platform, cfg.dtype);
  }
}

} // namespace pluss::ai::ice::io
