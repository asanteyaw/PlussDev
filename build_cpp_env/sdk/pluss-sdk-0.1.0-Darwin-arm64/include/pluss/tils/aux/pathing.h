#pragma once

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::aux {

enum class RootResolve {
  WorkingDirectory,
  CallerFile,
};

struct PathPolicy {
  bool canonicalize_root{true};
  bool auto_create_dirs{false};
  bool force_replace_dirs{false};
  bool strip_leading_separators{true};
};

struct PathOptions {
  std::optional<bool> create_dirs{};
  std::optional<bool> force_replace_dirs{};
};

namespace detail {

inline std::filesystem::path detect_default_root_() {
#ifdef PLUSSDEV_SOURCE_DIR
  return std::filesystem::path(PLUSSDEV_SOURCE_DIR);
#else
  if (const char* env_root = std::getenv("PLUSS_ROOT_PATH")) return std::filesystem::path(env_root);
  if (const char* env_root = std::getenv("PLUSS_PROJECT_ROOT")) return std::filesystem::path(env_root);

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

inline std::filesystem::path resolve_root_input_(std::filesystem::path root,
                                                 RootResolve resolve_mode,
                                                 const std::source_location& where,
                                                 bool canonicalize_root) {
  if (root.empty()) root = ".";

  std::filesystem::path base = std::filesystem::current_path();
  if (resolve_mode == RootResolve::CallerFile) {
    std::filesystem::path caller_file(where.file_name());
    if (!caller_file.empty()) {
      if (caller_file.is_relative()) caller_file = std::filesystem::current_path() / caller_file;
      if (caller_file.has_parent_path()) base = caller_file.parent_path();
    }
  }

  std::filesystem::path out = root.is_absolute() ? root : (base / root);

  std::error_code ec;
  out = std::filesystem::absolute(out, ec);
  if (ec) out = (base / root);
  out = out.lexically_normal();

  if (canonicalize_root) {
    std::error_code canon_ec;
    const auto canon = std::filesystem::weakly_canonical(out, canon_ec);
    if (!canon_ec) out = canon;
  }

  return out;
}

inline std::filesystem::path sanitize_subpath_(const std::filesystem::path& subpath,
                                               bool strip_leading_separators) {
  (void)strip_leading_separators; // retained for source compatibility; separators are always stripped.
  if (subpath.empty()) return {};

  auto normalized = subpath.lexically_normal();
  if (normalized.is_absolute() || normalized.has_root_name() || normalized.has_root_directory()) {
    normalized = normalized.relative_path();
  }

  std::string s = normalized.generic_string();
  while (!s.empty() && (s.front() == '/' || s.front() == '\\')) {
    s.erase(s.begin());
  }
  return std::filesystem::path(s);
}

inline bool is_rooted_path_(const std::filesystem::path& p) {
  return p.is_absolute() || p.has_root_name() || p.has_root_directory();
}

inline void ensure_directory_(const std::filesystem::path& p, bool force_replace_dirs) {
  if (force_replace_dirs && std::filesystem::exists(p)) {
    std::filesystem::remove_all(p);
  }
  std::filesystem::create_directories(p);
}

inline bool is_path_within_root_(const std::filesystem::path& root, const std::filesystem::path& candidate) {
  std::error_code root_ec;
  std::error_code cand_ec;
  const auto abs_root = std::filesystem::absolute(root, root_ec).lexically_normal();
  const auto abs_candidate = std::filesystem::absolute(candidate, cand_ec).lexically_normal();
  if (root_ec || cand_ec) return false;

  auto r = abs_root.begin();
  auto c = abs_candidate.begin();
  for (; r != abs_root.end() && c != abs_candidate.end(); ++r, ++c) {
    if (*r != *c) return false;
  }
  return r == abs_root.end();
}

inline void apply_file_create_policy_(const std::filesystem::path& resolved_file,
                                      const PathOptions& options,
                                      const PathPolicy& policy) {
  const bool create_dirs = options.create_dirs.value_or(policy.auto_create_dirs);
  if (!create_dirs) return;

  const auto parent = resolved_file.parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
}

inline void apply_dir_create_policy_(const std::filesystem::path& resolved_dir,
                                     const PathOptions& options,
                                     const PathPolicy& policy) {
  const bool create_dirs = options.create_dirs.value_or(policy.auto_create_dirs);
  if (!create_dirs) return;

  const bool force_replace_dirs = options.force_replace_dirs.value_or(policy.force_replace_dirs);
  ensure_directory_(resolved_dir, force_replace_dirs);
}

inline std::filesystem::path resolve_root_relative_file_(const std::filesystem::path& root,
                                                         const std::filesystem::path& requested,
                                                         const PathPolicy& policy,
                                                         std::string_view api_name) {
  if (is_rooted_path_(requested)) {
    PLUSS_WARN(std::string(api_name) + ": rooted input '" + requested.string() +
               "' is treated as a subpath under configured root");
  }

  const auto rel = sanitize_subpath_(requested, policy.strip_leading_separators);
  PLUSS_CHECK(
      !rel.empty(),
      std::string(api_name) + ": path is empty after normalization; requested='" + requested.string() + "'");

  const auto resolved = (root / rel).lexically_normal();
  PLUSS_CHECK(
      is_path_within_root_(root, resolved),
      std::string(api_name) + ": path escapes configured root; root='" + root.string() +
          "' requested='" + requested.string() + "' resolved='" + resolved.string() + "'");
  return resolved;
}

inline std::filesystem::path resolve_root_relative_dir_(const std::filesystem::path& root,
                                                        const std::filesystem::path& requested,
                                                        const PathPolicy& policy,
                                                        std::string_view api_name) {
  if (is_rooted_path_(requested)) {
    PLUSS_WARN(std::string(api_name) + ": rooted input '" + requested.string() +
               "' is treated as a subpath under configured root");
  }

  const auto rel = sanitize_subpath_(requested, policy.strip_leading_separators);
  const auto resolved = rel.empty() ? root : (root / rel).lexically_normal();
  PLUSS_CHECK(
      is_path_within_root_(root, resolved),
      std::string(api_name) + ": path escapes configured root; root='" + root.string() +
          "' requested='" + requested.string() + "' resolved='" + resolved.string() + "'");
  return resolved;
}

inline std::filesystem::path resolve_under_parent_file_(const std::filesystem::path& parent,
                                                        const std::filesystem::path& requested,
                                                        const PathPolicy& policy,
                                                        std::string_view api_name) {
  if (is_rooted_path_(requested)) {
    PLUSS_WARN(std::string(api_name) + ": rooted input '" + requested.string() +
               "' is treated as a subpath under the current parent");
  }

  const auto rel = sanitize_subpath_(requested, policy.strip_leading_separators);
  PLUSS_CHECK(
      !rel.empty(),
      std::string(api_name) + ": path is empty after normalization; requested='" + requested.string() + "'");

  const auto resolved = (parent / rel).lexically_normal();
  PLUSS_CHECK(
      is_path_within_root_(parent, resolved),
      std::string(api_name) + ": path escapes parent root; parent='" + parent.string() +
          "' requested='" + requested.string() + "' resolved='" + resolved.string() + "'");
  return resolved;
}

inline std::filesystem::path resolve_absolute_file_(const std::filesystem::path& requested,
                                                    std::string_view api_name) {
  PLUSS_CHECK(!requested.empty(), std::string(api_name) + ": path must be non-empty");
  PLUSS_CHECK(
      is_rooted_path_(requested),
      std::string(api_name) + ": input must be absolute/rooted; use file_path(...) for root-relative paths");

  std::error_code abs_ec;
  auto resolved = std::filesystem::absolute(requested, abs_ec);
  if (abs_ec) {
    PLUSS_WARN(std::string(api_name) + ": std::filesystem::absolute failed for '" + requested.string() +
               "'; using lexical normalization fallback");
    resolved = requested;
  }
  resolved = resolved.lexically_normal();

  PLUSS_CHECK(
      is_rooted_path_(resolved),
      std::string(api_name) + ": resolved path is not absolute/rooted; input='" + requested.string() +
          "' resolved='" + resolved.string() + "'");
  return resolved;
}

inline bool is_http_url_(std::string_view value) {
  return value.rfind("http://", 0) == 0 || value.rfind("https://", 0) == 0;
}

inline std::string trim_ascii_(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b])) != 0) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) --e;
  return std::string(s.substr(b, e - b));
}

inline std::string normalize_url_root_(std::string_view root, std::string_view api_name) {
  std::string out = trim_ascii_(root);
  PLUSS_CHECK(!out.empty(), std::string(api_name) + ": URL root is empty");
  PLUSS_CHECK(is_http_url_(out), std::string(api_name) + ": URL root must start with http:// or https://");
  PLUSS_CHECK(out.find('?') == std::string::npos && out.find('#') == std::string::npos,
              std::string(api_name) + ": URL root must not contain query or fragment");

  while (out.size() > 1 && out.back() == '/') out.pop_back();
  return out;
}

inline std::string sanitize_url_subpath_(std::string_view subpath, std::string_view api_name) {
  std::string cleaned = trim_ascii_(subpath);
  PLUSS_CHECK(!cleaned.empty(), std::string(api_name) + ": subpath must be non-empty");
  PLUSS_CHECK(!is_http_url_(cleaned), std::string(api_name) + ": subpath must be relative, not a full URL");

  while (!cleaned.empty() && (cleaned.front() == '/' || cleaned.front() == '\\')) {
    cleaned.erase(cleaned.begin());
  }

  const std::filesystem::path requested(cleaned);
  PLUSS_CHECK(!requested.is_absolute() && !requested.has_root_name() && !requested.has_root_directory(),
              std::string(api_name) + ": rooted subpath is not allowed");

  const auto normalized = requested.lexically_normal();
  PLUSS_CHECK(!normalized.empty(), std::string(api_name) + ": subpath is empty after normalization");
  for (const auto& part : normalized) {
    if (part == "..") {
      PLUSS_FAIL(std::string(api_name) + ": subpath escapes URL root");
    }
  }

  std::string rel = normalized.generic_string();
  PLUSS_CHECK(rel.find('?') == std::string::npos && rel.find('#') == std::string::npos,
              std::string(api_name) + ": subpath must not contain query or fragment");
  PLUSS_CHECK(!rel.empty() && rel != ".", std::string(api_name) + ": subpath is not a file target");
  return rel;
}

inline std::string resolve_root_relative_url_(std::string_view root,
                                              std::string_view subpath,
                                              std::string_view api_name) {
  const auto normalized_root = normalize_url_root_(root, api_name);
  const auto normalized_subpath = sanitize_url_subpath_(subpath, api_name);
  return normalized_root + "/" + normalized_subpath;
}

struct PathState {
  std::mutex mu{};
  PathPolicy policy{};
  std::filesystem::path root_path{
      resolve_root_input_(detect_default_root_(), RootResolve::WorkingDirectory, std::source_location::current(), true)};
};

inline PathState& path_state_() {
  static PathState state{};
  return state;
}

} // namespace detail

class PathRef {
public:
  PathRef() = default;

  explicit PathRef(std::filesystem::path p)
      : path_(std::move(p)) {}

  [[nodiscard]] std::string string() const {
    return path_.string();
  }

  operator const std::filesystem::path&() const noexcept { return path_; }

private:
  std::filesystem::path path_{};
};

class UrlRef {
public:
  UrlRef() = default;

  explicit UrlRef(std::string url)
      : url_(std::move(url)) {}

  [[nodiscard]] const std::string& string() const noexcept {
    return url_;
  }

private:
  std::string url_{};
};

class DirRef {
public:
  DirRef() = default;

  explicit DirRef(std::filesystem::path p)
      : path_(std::move(p)) {}

  explicit DirRef(std::filesystem::path p, PathPolicy policy)
      : path_(std::move(p)), policy_(policy), has_policy_(true) {}

  [[nodiscard]] std::string string() const {
    std::string out = path_.string();
    if (!out.empty() && out.back() != '/' && out.back() != '\\') {
      out.push_back(std::filesystem::path::preferred_separator);
    }
    return out;
  }

  [[nodiscard]] PathRef file(std::string_view subpath,
                             PathOptions options = {}) const {
    const std::filesystem::path requested(subpath);
    PLUSS_CHECK(!requested.empty(), "pluss::loader::DirRef::file: path must be non-empty");

    PathPolicy policy = policy_;
    if (!has_policy_) {
      auto& state = detail::path_state_();
      std::scoped_lock lock(state.mu);
      policy = state.policy;
    }

    const auto resolved = detail::resolve_under_parent_file_(path_, requested, policy, "pluss::loader::DirRef::file");
    detail::apply_file_create_policy_(resolved, options, policy);
    return PathRef(resolved);
  }

  operator const std::filesystem::path&() const noexcept { return path_; }

private:
  std::filesystem::path path_{};
  PathPolicy policy_{};
  bool has_policy_{false};
};

class UrlContext {
public:
  UrlContext() = default;

  explicit UrlContext(std::string root_url) {
    set_root(std::move(root_url));
  }

  [[nodiscard]] static UrlContext from_root(std::string root_url) {
    return UrlContext(std::move(root_url));
  }

  void set_root(std::string root_url) {
    root_url_ = detail::normalize_url_root_(root_url, "pluss::loader::UrlContext::set_root");
  }

  [[nodiscard]] const std::string& root() const noexcept {
    return root_url_;
  }

  [[nodiscard]] UrlRef file_path(std::string_view subpath) const {
    return UrlRef(detail::resolve_root_relative_url_(
        root_url_, subpath, "pluss::loader::UrlContext::file_path"));
  }

  [[nodiscard]] std::vector<UrlRef> file_paths(std::initializer_list<std::string_view> subpaths) const {
    std::vector<UrlRef> out{};
    out.reserve(subpaths.size());
    for (const auto& subpath : subpaths) {
      out.emplace_back(file_path(subpath));
    }
    return out;
  }

  [[nodiscard]] std::unordered_map<std::string, UrlRef> file_path_map(
      std::initializer_list<std::pair<std::string_view, std::string_view>> entries) const {
    std::unordered_map<std::string, UrlRef> out{};
    out.reserve(entries.size());
    for (const auto& [name, subpath] : entries) {
      PLUSS_CHECK(!name.empty(), "pluss::loader::UrlContext::file_path_map: key must not be empty");
      const auto key = std::string(name);
      const auto [it, inserted] = out.emplace(key, file_path(subpath));
      PLUSS_CHECK(inserted, "pluss::loader::UrlContext::file_path_map: duplicate key: " + key);
      (void)it;
    }
    return out;
  }

private:
  std::string root_url_{};
};

class PathContext {
public:
  PathContext() = default;

  explicit PathContext(std::filesystem::path root,
                       RootResolve resolve_mode = RootResolve::WorkingDirectory,
                       const std::source_location& where = std::source_location::current()) {
    set_root_path(std::move(root), resolve_mode, where);
  }

  [[nodiscard]] static PathContext from_root(std::filesystem::path root,
                                             RootResolve resolve_mode = RootResolve::WorkingDirectory,
                                             const std::source_location& where = std::source_location::current()) {
    return PathContext(std::move(root), resolve_mode, where);
  }

  [[nodiscard]] std::filesystem::path root_path() const {
    return root_path_;
  }

  void set_root_path(std::filesystem::path root,
                     RootResolve resolve_mode = RootResolve::WorkingDirectory,
                     const std::source_location& where = std::source_location::current()) {
    root_path_ = detail::resolve_root_input_(std::move(root), resolve_mode, where, policy_.canonicalize_root);
    PLUSS_CHECK(!root_path_.empty(), "pluss::loader::PathContext::set_root_path: resolved root path is empty");
  }

  void reset_root_path() {
    root_path_ = detail::resolve_root_input_(
        detail::detect_default_root_(),
        RootResolve::WorkingDirectory,
        std::source_location::current(),
        policy_.canonicalize_root);
  }

  [[nodiscard]] PathPolicy path_policy() const {
    return policy_;
  }

  void set_path_policy(PathPolicy policy) {
    policy_ = policy;
    root_path_ = detail::resolve_root_input_(
        root_path_,
        RootResolve::WorkingDirectory,
        std::source_location::current(),
        policy_.canonicalize_root);
  }

  [[nodiscard]] PathRef file_path(std::filesystem::path subpath, PathOptions options = {}) const {
    const auto resolved = detail::resolve_root_relative_file_(
        root_path_, subpath, policy_, "pluss::loader::PathContext::file_path");
    detail::apply_file_create_policy_(resolved, options, policy_);
    return PathRef(resolved);
  }

  [[nodiscard]] std::vector<PathRef> file_paths(
      std::initializer_list<std::filesystem::path> subpaths,
      PathOptions options = {}) const {
    std::vector<PathRef> out{};
    out.reserve(subpaths.size());
    for (const auto& subpath : subpaths) {
      out.emplace_back(file_path(subpath, options));
    }
    return out;
  }

  [[nodiscard]] std::unordered_map<std::string, PathRef> file_path_map(
      std::initializer_list<std::pair<std::string_view, std::filesystem::path>> entries,
      PathOptions options = {}) const {
    std::unordered_map<std::string, PathRef> out{};
    out.reserve(entries.size());
    for (const auto& [name, subpath] : entries) {
      PLUSS_CHECK(!name.empty(), "pluss::loader::PathContext::file_path_map: key must not be empty");
      const auto key = std::string(name);
      const auto [it, inserted] = out.emplace(key, file_path(subpath, options));
      PLUSS_CHECK(inserted, "pluss::loader::PathContext::file_path_map: duplicate key: " + key);
      (void)it;
    }
    return out;
  }

  [[nodiscard]] DirRef file_dir(std::filesystem::path subpath, PathOptions options = {}) const {
    const auto resolved = detail::resolve_root_relative_dir_(
        root_path_, subpath, policy_, "pluss::loader::PathContext::file_dir");
    detail::apply_dir_create_policy_(resolved, options, policy_);
    return DirRef(resolved, policy_);
  }

  [[nodiscard]] PathRef absolute_path(std::filesystem::path path, PathOptions options = {}) const {
    const auto resolved = detail::resolve_absolute_file_(path, "pluss::loader::PathContext::absolute_path");
    detail::apply_file_create_policy_(resolved, options, policy_);
    return PathRef(resolved);
  }

private:
  PathPolicy policy_{};
  std::filesystem::path root_path_{
      detail::resolve_root_input_(detail::detect_default_root_(), RootResolve::WorkingDirectory,
                                  std::source_location::current(), true)};
};

inline std::filesystem::path root_path() {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  return state.root_path;
}

inline void set_root_path(std::filesystem::path root,
                          RootResolve resolve_mode = RootResolve::WorkingDirectory,
                          const std::source_location& where = std::source_location::current()) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.root_path = detail::resolve_root_input_(std::move(root), resolve_mode, where, state.policy.canonicalize_root);
  PLUSS_CHECK(!state.root_path.empty(), "pluss::loader::set_root_path: resolved root path is empty");
}

inline void reset_root_path() {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.root_path = detail::resolve_root_input_(
      detail::detect_default_root_(),
      RootResolve::WorkingDirectory,
      std::source_location::current(),
      state.policy.canonicalize_root);
}

inline PathPolicy path_policy() {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  return state.policy;
}

inline void set_path_policy(PathPolicy policy) {
  auto& state = detail::path_state_();
  std::scoped_lock lock(state.mu);
  state.policy = policy;
  state.root_path = detail::resolve_root_input_(
      state.root_path,
      RootResolve::WorkingDirectory,
      std::source_location::current(),
      state.policy.canonicalize_root);
}

inline PathRef file_path(std::filesystem::path subpath, PathOptions options = {}) {
  std::filesystem::path root;
  PathPolicy policy;
  {
    auto& state = detail::path_state_();
    std::scoped_lock lock(state.mu);
    root = state.root_path;
    policy = state.policy;
  }

  const auto resolved = detail::resolve_root_relative_file_(root, subpath, policy, "pluss::loader::file_path");
  detail::apply_file_create_policy_(resolved, options, policy);
  return PathRef(resolved);
}

inline std::vector<PathRef> file_paths(std::initializer_list<std::filesystem::path> subpaths,
                                       PathOptions options = {}) {
  std::vector<PathRef> out{};
  out.reserve(subpaths.size());
  for (const auto& subpath : subpaths) {
    out.emplace_back(file_path(subpath, options));
  }
  return out;
}

inline std::unordered_map<std::string, PathRef> file_path_map(
    std::initializer_list<std::pair<std::string_view, std::filesystem::path>> entries,
    PathOptions options = {}) {
  std::unordered_map<std::string, PathRef> out{};
  out.reserve(entries.size());
  for (const auto& [name, subpath] : entries) {
    PLUSS_CHECK(!name.empty(), "pluss::loader::file_path_map: key must not be empty");
    const auto key = std::string(name);
    const auto [it, inserted] = out.emplace(key, file_path(subpath, options));
    PLUSS_CHECK(inserted, "pluss::loader::file_path_map: duplicate key: " + key);
    (void)it;
  }
  return out;
}

inline DirRef file_dir(std::filesystem::path subpath, PathOptions options = {}) {
  std::filesystem::path root;
  PathPolicy policy;
  {
    auto& state = detail::path_state_();
    std::scoped_lock lock(state.mu);
    root = state.root_path;
    policy = state.policy;
  }

  const auto resolved = detail::resolve_root_relative_dir_(root, subpath, policy, "pluss::loader::file_dir");
  detail::apply_dir_create_policy_(resolved, options, policy);
  return DirRef(resolved, policy);
}

inline PathRef absolute_path(std::filesystem::path path, PathOptions options = {}) {
  PathPolicy policy;
  {
    auto& state = detail::path_state_();
    std::scoped_lock lock(state.mu);
    policy = state.policy;
  }

  const auto resolved = detail::resolve_absolute_file_(path, "pluss::loader::absolute_path");
  detail::apply_file_create_policy_(resolved, options, policy);
  return PathRef(resolved);
}

inline std::filesystem::path join_path(std::filesystem::path base,
                                       std::initializer_list<std::string_view> parts) {
  for (auto part : parts) {
    if (!part.empty()) base /= std::string(part);
  }
  return base;
}

} // namespace pluss::aux

namespace pluss::loader {
using aux::DirRef;
using aux::PathContext;
using aux::PathOptions;
using aux::PathPolicy;
using aux::PathRef;
using aux::RootResolve;
using aux::UrlContext;
using aux::UrlRef;
using aux::absolute_path;
using aux::file_dir;
using aux::file_path;
using aux::file_path_map;
using aux::file_paths;
using aux::join_path;
using aux::path_policy;
using aux::reset_root_path;
using aux::root_path;
using aux::set_path_policy;
using aux::set_root_path;
} // namespace pluss::loader
