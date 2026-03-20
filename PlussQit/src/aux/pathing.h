#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pluss::aux {

namespace detail {

inline std::string normalize_key_(std::string_view key) {
  std::string out(key);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

inline std::string upper_key_(std::string_view key) {
  std::string out(key);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  return out;
}

inline std::optional<std::filesystem::path> getenv_path_(const std::string& key) {
  if (const char* v = std::getenv(key.c_str())) return std::filesystem::path(v);
  return std::nullopt;
}

inline std::optional<std::filesystem::path> env_override_for_key_(std::string_view key) {
  const std::string normalized = normalize_key_(key);
  const std::string generic = "PLUSS_PATH_" + upper_key_(normalized);
  if (auto p = getenv_path_(generic)) return p;

  if (normalized == "project") return getenv_path_("PLUSS_PROJECT_ROOT");
  if (normalized == "data") return getenv_path_("PLUSS_DATA_DIR");
  if (normalized == "output") return getenv_path_("PLUSS_OUTPUT_DIR");
  if (normalized == "model") return getenv_path_("PLUSS_MODEL_DIR");
  if (normalized == "cache") return getenv_path_("PLUSS_CACHE_DIR");
  return std::nullopt;
}

inline std::filesystem::path detect_project_root_() {
#ifdef PLUSSDEV_SOURCE_DIR
  return std::filesystem::path(PLUSSDEV_SOURCE_DIR);
#else
  if (auto p = env_override_for_key_("project")) return *p;

  auto probe = std::filesystem::current_path();
  for (int i = 0; i < 12; ++i) {
    if (std::filesystem::exists(probe / ".git") ||
        std::filesystem::exists(probe / "CMakeLists.txt") ||
        std::filesystem::exists(probe / "PlussQit")) {
      return probe;
    }
    if (!probe.has_parent_path()) break;
    probe = probe.parent_path();
  }
  return std::filesystem::current_path();
#endif
}

inline std::filesystem::path default_root_for_key_(
    std::string_view key,
    const std::filesystem::path& project_root) {
  if (auto p = env_override_for_key_(key)) return *p;

  const std::string normalized = normalize_key_(key);
  const auto pick_existing = [&](std::initializer_list<std::filesystem::path> candidates) {
    for (const auto& c : candidates) {
      if (std::filesystem::exists(c)) return c;
    }
    return *candidates.begin();
  };

  if (normalized == "qit") return pick_existing({project_root / "PlussQit", project_root / "qit"});
  if (normalized == "mic") return pick_existing({project_root / "PlussMic", project_root / "mic"});
  if (normalized == "ma") return pick_existing({project_root / "PlussMa", project_root / "ma"});
  if (normalized == "ai") return pick_existing({project_root / "PlussAI", project_root / "ai"});
  if (normalized == "data") return pick_existing({project_root / "data", project_root / "PlussMic" / "data"});
  if (normalized == "output") return pick_existing({project_root / "outputs", project_root / "examples" / "outputs"});
  if (normalized == "model") return pick_existing({project_root / "models", project_root / "trained_models"});
  if (normalized == "cache") return pick_existing({project_root / ".cache", project_root / "cache"});
  if (normalized == "temp") return std::filesystem::temp_directory_path();
  return project_root / normalized;
}

struct PathState {
  std::mutex mu{};
  std::filesystem::path project_root{detect_project_root_()};
  std::unordered_map<std::string, std::filesystem::path> roots{};
  bool auto_create_dirs{false};
};

inline PathState& path_state_() {
  static PathState state{};
  return state;
}

inline std::filesystem::path maybe_create_(std::filesystem::path p, bool create_dirs) {
  if (create_dirs) std::filesystem::create_directories(p);
  return p;
}

} // namespace detail

inline std::filesystem::path project_root() {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  return state.project_root;
}

inline void set_project_root(std::filesystem::path root) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.project_root = std::move(root);
}

inline void reset_project_root() {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.project_root = detail::detect_project_root_();
}

inline bool auto_create_path_dirs() {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  return state.auto_create_dirs;
}

inline void set_auto_create_path_dirs(bool enabled = true) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.auto_create_dirs = enabled;
}

inline void set_path_root(std::string_view key, std::filesystem::path root) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.roots[detail::normalize_key_(key)] = std::move(root);
}

inline void clear_path_root(std::string_view key) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.roots.erase(detail::normalize_key_(key));
}

inline bool has_path_root(std::string_view key) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  return state.roots.find(detail::normalize_key_(key)) != state.roots.end();
}

inline std::filesystem::path path_root(std::string_view key, bool create_dirs = false) {
  const std::string normalized = detail::normalize_key_(key);
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  const auto it = state.roots.find(normalized);
  const auto root = (it != state.roots.end())
      ? it->second
      : detail::default_root_for_key_(normalized, state.project_root);
  return detail::maybe_create_(root, create_dirs || state.auto_create_dirs);
}

inline std::filesystem::path path_join(std::filesystem::path base, std::initializer_list<std::string_view> parts) {
  for (auto part : parts) {
    if (!part.empty()) base /= std::string(part);
  }
  return base;
}

inline std::filesystem::path path_file(
    std::string_view key,
    std::string_view name,
    bool ensure_parent = false) {
  auto out = path_root(key, false) / std::string(name);
  if (ensure_parent) std::filesystem::create_directories(out.parent_path());
  return out;
}

inline std::filesystem::path ensure_dir(std::filesystem::path p) {
  std::filesystem::create_directories(p);
  return p;
}

// Convenience scopes.
inline std::filesystem::path qit_root() { return path_root("qit"); }
inline std::filesystem::path mic_root() { return path_root("mic"); }
inline std::filesystem::path ma_root() { return path_root("ma"); }
inline std::filesystem::path ai_root() { return path_root("ai"); }

// Generic defaults.
inline std::filesystem::path data_file(std::string_view name) { return path_file("data", name); }
inline std::filesystem::path output_dir(std::string_view name = {}) {
  auto out = path_root("output");
  if (!name.empty()) out /= std::string(name);
  return out;
}
inline std::filesystem::path model_dir(std::string_view name = {}) {
  auto out = path_root("model");
  if (!name.empty()) out /= std::string(name);
  return out;
}

// Backward-compatible overload used by existing demos.
inline std::filesystem::path output_dir(std::string_view library, std::string_view demo_name) {
  auto out = path_root("output");
  if (!library.empty()) out /= std::string(library);
  if (!demo_name.empty()) out /= std::string(demo_name);
  return out;
}

} // namespace pluss::aux

namespace pluss::loader {
using aux::ai_root;
using aux::auto_create_path_dirs;
using aux::clear_path_root;
using aux::data_file;
using aux::ensure_dir;
using aux::has_path_root;
using aux::ma_root;
using aux::mic_root;
using aux::model_dir;
using aux::output_dir;
using aux::path_file;
using aux::path_join;
using aux::path_root;
using aux::project_root;
using aux::qit_root;
using aux::reset_project_root;
using aux::set_auto_create_path_dirs;
using aux::set_path_root;
using aux::set_project_root;
} // namespace pluss::loader
