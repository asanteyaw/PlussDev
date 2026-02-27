#pragma once
#include <cctype>
#include <unordered_map>
#include <string>
#include <initializer_list>
#include <utility>
#include <sstream>

#include "CodeM/core/Config.h"
#include "CodeM/core/Errors.h"

namespace pluss {

// Typed option tags to avoid boilerplate chains.
// Usage: pluss::zeros({128,64}, pluss::cfg::dtype(pluss::ScalarType::Float16), pluss::cfg::pinned(true));
namespace cfg {
  struct DType { pluss::codem::ScalarType v; };
  struct Pinned { bool v; };
  struct Platform { pluss::codem::Platform v; };
  struct TracksSlope { bool v; };
  struct RetainSensitivity { bool v; };

  inline DType dtype(pluss::codem::ScalarType v) { return DType{v}; }
  inline Pinned pinned(bool v) { return Pinned{v}; }
  inline Platform platform(pluss::codem::Platform v) { return Platform{v}; }
  inline TracksSlope tracks_slope(bool v = true) { return TracksSlope{v}; }
  inline RetainSensitivity retain_sensitivity(bool v = true) { return RetainSensitivity{v}; }
}

namespace detail {
  inline void apply_opt(pluss::codem::Config& c, cfg::DType o) { c = c.with_dtype(o.v); }
  inline void apply_opt(pluss::codem::Config& c, cfg::Pinned o) { c = c.with_pinned(o.v); }
  inline void apply_opt(pluss::codem::Config& c, cfg::Platform o) { c = c.with_platform(o.v); }

  template <class T>
  inline void apply_opt(pluss::codem::Config&, const T&) {
    pluss::codem::throw_error("Unknown Config option type");
  }

  template <class... Opts>
  inline pluss::codem::Config make_config(Opts... opts) {
    pluss::codem::Config c{};
    (apply_opt(c, opts), ...);
    return c;
  }

  inline pluss::codem::ScalarType parse_dtype(const std::string& s) {
    auto v = s;
    for (auto& ch : v) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (v=="float16" || v=="half" ) return pluss::codem::ScalarType::Float16;
    if (v=="bfloat16") return pluss::codem::ScalarType::BFloat16;
    if (v=="float32" || v=="float") return pluss::codem::ScalarType::Float32;
    if (v=="float64" || v=="double") return pluss::codem::ScalarType::Float64;
    if (v=="int64" || v=="long") return pluss::codem::ScalarType::Int64;
    if (v=="int32" || v=="int") return pluss::codem::ScalarType::Int32;
    if (v=="int16") return pluss::codem::ScalarType::Int16;
    if (v=="int8") return pluss::codem::ScalarType::Int8;
    if (v=="uint8" || v=="byte") return pluss::codem::ScalarType::UInt8;
    if (v=="bool") return pluss::codem::ScalarType::Bool;
    pluss::codem::throw_error("Invalid dtype string: " + s);
  }

  inline bool parse_bool(const std::string& s) {
    auto v=s; for (auto& ch: v) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (v=="1"||v=="true"||v=="yes") return true;
    if (v=="0"||v=="false"||v=="no") return false;
    pluss::codem::throw_error("Invalid bool string: " + s);
  }

  inline pluss::codem::Platform parse_platform(const std::string& s) {
    auto v = s;
    for (auto& ch : v) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (v == "cpu") return pluss::codem::Platform::CPU;
    if (v == "cuda") return pluss::codem::Platform::CUDA;
    if (v == "mps") return pluss::codem::Platform::MPS;
    pluss::codem::throw_error("Invalid platform string: " + s);
  }
}

// String-map based config builder (unordered, ergonomic; CPU-only subset).
inline pluss::codem::Config config(const std::unordered_map<std::string,std::string>& m) {
  pluss::codem::Config c{};
  for (auto& [k,v] : m) {
    std::string key=k; for (auto& ch:key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (key=="dtype") c = c.with_dtype(detail::parse_dtype(v));
    else if (key=="pinned" || key=="pinned_memory") c = c.with_pinned(detail::parse_bool(v));
    else if (key=="platform" || key=="device" || key=="backend") c = c.with_platform(detail::parse_platform(v));
    else pluss::codem::throw_error("Unknown Config key: " + k);
  }
  return c;
}

template <class T>
inline std::string to_string_value(const T& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <class T>
inline pluss::codem::Config config(std::initializer_list<std::pair<const char*, T>> list) {
  std::unordered_map<std::string,std::string> m;
  for (auto& kv : list) m.emplace(kv.first, to_string_value(kv.second));
  return config(m);
}

} // namespace pluss
