#include "CodeM/core/StackTrace.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__APPLE__) || defined(__unix__)
#include <dlfcn.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <unistd.h>
#endif

namespace pluss::codem {

thread_local std::vector<Frame> StackTrace::frames_{};

namespace {

struct NativeFrame {
  void* addr{nullptr};
  void* image_base{nullptr};
  std::string image_path;
  std::string symbol;
  std::uint64_t symbol_offset{0};
};

static std::string trim_path_(const std::string& p) {
  if (const char* root = std::getenv("PLUSS_SOURCE_ROOT")) {
    const std::string r(root);
    if (!r.empty() && p.rfind(r, 0) == 0) {
      std::string out = p.substr(r.size());
      if (!out.empty() && (out[0] == '/' || out[0] == '\\')) out.erase(out.begin());
      return out;
    }
  }
  if (p.size() > 120) {
    const auto pos = p.find_last_of("/\\");
    if (pos != std::string::npos) return p.substr(pos + 1);
  }
  return p;
}

static std::string ptr_hex_(const void* p) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase
      << static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(p));
  return oss.str();
}

static std::string shell_escape_(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 2);
  out.push_back('\'');
  for (char c : s) {
    if (c == '\'') out += "'\\''";
    else out.push_back(c);
  }
  out.push_back('\'');
  return out;
}

static std::string run_command_capture_(const std::string& cmd) {
  std::array<char, 512> buf{};
  std::string out;
  FILE* pipe = ::popen(cmd.c_str(), "r");
  if (!pipe) return out;
  while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
    out.append(buf.data());
  }
  (void)::pclose(pipe);
  while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
  return out;
}

static std::string demangle_symbol_(const char* s) {
#if defined(__APPLE__) || defined(__unix__)
  if (!s || *s == '\0') return {};
  int status = 0;
  char* demangled = abi::__cxa_demangle(s, nullptr, nullptr, &status);
  if (status == 0 && demangled) {
    std::string out(demangled);
    std::free(demangled);
    return out;
  }
  if (demangled) std::free(demangled);
  return std::string(s);
#else
  return s ? std::string(s) : std::string{};
#endif
}

static std::string image_basename_(std::string_view p) {
  const auto pos = p.find_last_of("/\\");
  if (pos == std::string_view::npos) return std::string(p);
  return std::string(p.substr(pos + 1));
}

static bool parse_trailing_line_number_(std::string_view s, std::uint64_t& out_line) {
  if (s.empty()) return false;
  std::size_t i = s.size();
  while (i > 0) {
    while (i > 0 && !std::isdigit(static_cast<unsigned char>(s[i - 1]))) {
      --i;
    }
    if (i == 0) return false;

    std::size_t end = i;
    while (i > 0 && std::isdigit(static_cast<unsigned char>(s[i - 1]))) {
      --i;
    }
    if (i > 0 && s[i - 1] == ':') {
      std::uint64_t v = 0;
      for (std::size_t k = i; k < end; ++k) {
        v = v * 10 + static_cast<std::uint64_t>(s[k] - '0');
      }
      out_line = v;
      return true;
    }
  }
  return false;
}

static bool has_meaningful_source_line_(std::string_view s) {
  std::uint64_t line = 0;
  if (!parse_trailing_line_number_(s, line)) return false;
  return line > 0;
}

static bool text_has_pluss_hint_(std::string_view text) {
  return text.find("pluss::") != std::string_view::npos ||
         text.find("PlussDev") != std::string_view::npos ||
         text.find("PlussQit") != std::string_view::npos ||
         text.find("PlussAI") != std::string_view::npos ||
         text.find("PlussMa") != std::string_view::npos ||
         text.find("PlussMic") != std::string_view::npos ||
         text.find("libpluss") != std::string_view::npos;
}

static bool is_pluss_frame_(const NativeFrame& f) {
  return text_has_pluss_hint_(f.symbol) || text_has_pluss_hint_(f.image_path);
}

static bool is_internal_helper_frame_(const NativeFrame& f) {
  return f.symbol.find("pluss::codem::StackTrace::format_") != std::string::npos ||
         f.symbol.find("pluss::codem::throw_error") != std::string::npos ||
         f.symbol.find("pluss::throw_error") != std::string::npos ||
         f.symbol.find("capture_native_frames_") != std::string::npos;
}

static std::vector<NativeFrame> capture_native_frames_(std::size_t max_frames) {
  std::vector<NativeFrame> out;
#if defined(__APPLE__) || defined(__unix__)
  constexpr int kCap = 512;
  void* addrs[kCap];
  const int want = static_cast<int>(std::min<std::size_t>(max_frames, static_cast<std::size_t>(kCap)));
  const int n = ::backtrace(addrs, want);
  if (n <= 0) return out;

  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    NativeFrame frame;
    frame.addr = addrs[i];
#if defined(__APPLE__) || defined(__unix__)
    Dl_info info{};
    if (::dladdr(addrs[i], &info) != 0) {
      if (info.dli_fname) frame.image_path = info.dli_fname;
      if (info.dli_sname) frame.symbol = demangle_symbol_(info.dli_sname);
      frame.image_base = info.dli_fbase;
      if (info.dli_saddr) {
        const auto pc = reinterpret_cast<std::uintptr_t>(addrs[i]);
        const auto sa = reinterpret_cast<std::uintptr_t>(info.dli_saddr);
        if (pc >= sa) frame.symbol_offset = static_cast<std::uint64_t>(pc - sa);
      }
    }
#endif
    out.push_back(std::move(frame));
  }
#else
  (void)max_frames;
#endif
  return out;
}

static std::string resolve_source_line_(const NativeFrame& f) {
  if (f.image_path.empty() || f.addr == nullptr) return {};
#if defined(__APPLE__)
  std::string cmd = "/usr/bin/atos -o " + shell_escape_(f.image_path);
  if (f.image_base != nullptr) cmd += " -l " + ptr_hex_(f.image_base);
  cmd += " " + ptr_hex_(f.addr) + " 2>/dev/null";
  std::string out = run_command_capture_(cmd);
  if (out.empty() || out == "???" || out.find("0x") == 0) return {};
  if (!has_meaningful_source_line_(out)) return {};
  return out;
#elif defined(__unix__)
  std::string cmd = "addr2line -f -C -e " + shell_escape_(f.image_path) + " " + ptr_hex_(f.addr) + " 2>/dev/null";
  std::string out = run_command_capture_(cmd);
  if (out.empty()) return {};
  const auto pos = out.find('\n');
  if (pos == std::string::npos) return {};
  const std::string fn = out.substr(0, pos);
  const std::string src = out.substr(pos + 1);
  if (src == "??:0" || src == "??:?") return {};
  if (!has_meaningful_source_line_(src)) return {};
  return fn + " (" + src + ")";
#else
  return {};
#endif
}

static std::vector<std::string> resolve_source_lines_batch_(const std::vector<NativeFrame>& frames) {
  std::vector<std::string> resolved(frames.size());
  if (frames.empty()) return resolved;

#if defined(__APPLE__) || defined(__unix__)
  std::unordered_map<std::string, std::vector<std::size_t>> groups;
  groups.reserve(frames.size());
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const auto& f = frames[i];
    if (f.image_path.empty() || f.addr == nullptr) continue;
    std::string key;
    key.reserve(f.image_path.size() + 32);
    key += f.image_path;
    key.push_back('\x1f');
    key += ptr_hex_(f.image_base);
    groups[std::move(key)].push_back(i);
  }

  for (const auto& [_, idxs] : groups) {
    if (idxs.empty()) continue;
    const auto& f0 = frames[idxs.front()];
#if defined(__APPLE__)
    std::string cmd = "/usr/bin/atos -o " + shell_escape_(f0.image_path);
    if (f0.image_base != nullptr) cmd += " -l " + ptr_hex_(f0.image_base);
    for (const auto idx : idxs) cmd += " " + ptr_hex_(frames[idx].addr);
    cmd += " 2>/dev/null";
    const std::string out = run_command_capture_(cmd);
    if (!out.empty()) {
      std::istringstream iss(out);
      std::string line;
      std::size_t j = 0;
      while (j < idxs.size() && std::getline(iss, line)) {
        if (!line.empty() &&
            line != "???" &&
            line.rfind("0x", 0) != 0 &&
            has_meaningful_source_line_(line)) {
          resolved[idxs[j]] = std::move(line);
        }
        ++j;
      }
    }
#elif defined(__unix__)
    std::string cmd = "addr2line -f -C -e " + shell_escape_(f0.image_path);
    for (const auto idx : idxs) cmd += " " + ptr_hex_(frames[idx].addr);
    cmd += " 2>/dev/null";
    const std::string out = run_command_capture_(cmd);
    if (!out.empty()) {
      std::istringstream iss(out);
      std::string fn;
      std::string src;
      std::size_t j = 0;
      while (j < idxs.size() && std::getline(iss, fn) && std::getline(iss, src)) {
        if (src != "??:0" && src != "??:?" && has_meaningful_source_line_(src)) {
          resolved[idxs[j]] = fn + " (" + src + ")";
        }
        ++j;
      }
    }
#endif
  }
#endif

  // Best-effort fallback for unresolved frames (bounded to avoid
  // spawning one subprocess per frame when tooling/symbols are missing).
  constexpr std::size_t kFallbackLimit = 8;
  std::size_t fallback_used = 0;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (!resolved[i].empty()) continue;
    if (fallback_used >= kFallbackLimit) break;
    resolved[i] = resolve_source_line_(frames[i]);
    if (!resolved[i].empty()) ++fallback_used;
  }
  return resolved;
}

static std::string format_native_frame_(const NativeFrame& f, const std::string& resolved_src = {}) {
  if (!resolved_src.empty()) return resolved_src;

  std::ostringstream oss;
  if (!f.symbol.empty()) {
    oss << f.symbol;
    if (f.symbol_offset > 0) oss << " + " << f.symbol_offset;
  } else {
    oss << "<unknown>";
  }
  if (!f.image_path.empty()) {
    oss << " [" << image_basename_(f.image_path) << "]";
  }
  return oss.str();
}

} // namespace

void StackTrace::push(const std::source_location& loc) {
  Frame f{
    std::string(loc.function_name()),
    trim_path_(std::string(loc.file_name())),
    loc.line()
  };
  if (!frames_.empty()) {
    const auto& last = frames_.back();
    if (last.function == f.function && last.file == f.file && last.line == f.line) return;
  }
  frames_.push_back(std::move(f));
}

void StackTrace::pop() {
  if (!frames_.empty()) frames_.pop_back();
}

std::string StackTrace::format(std::size_t max_frames) {
  if (frames_.empty()) return {};
  std::ostringstream oss;
  const std::size_t n = frames_.size();
  const std::size_t start = (n > max_frames ? n - max_frames : 0);
  for (std::size_t i = n; i-- > start; ) {
    const auto& f = frames_[i];
    oss << "  at " << f.function << " (" << f.file << ":" << f.line << ")\n";
  }
  return oss.str();
}

std::string StackTrace::format_native(std::size_t max_frames) {
  const auto frames = capture_native_frames_(max_frames);
  if (frames.empty()) return {};
  const auto resolved = resolve_source_lines_batch_(frames);

  std::ostringstream oss;
  std::size_t i = 0;
  for (std::size_t idx = 0; idx < frames.size(); ++idx) {
    const auto& f = frames[idx];
    if (is_internal_helper_frame_(f)) continue;
    oss << "  " << i++ << "  " << format_native_frame_(f, resolved[idx]) << "\n";
  }
  return oss.str();
}

std::string StackTrace::format_user_to_trigger(std::size_t max_frames, std::size_t max_user_frames) {
  const auto frames = capture_native_frames_(max_frames);
  if (frames.empty()) return {};
  const auto resolved = resolve_source_lines_batch_(frames);

  // Skip local framing helpers near the top of the capture.
  std::size_t start = 0;
  while (start < frames.size()) {
    if (is_internal_helper_frame_(frames[start])) {
      ++start;
      continue;
    }
    break;
  }
  if (start >= frames.size()) return {};

  std::size_t trigger_index = start;
  while (trigger_index < frames.size() && !is_pluss_frame_(frames[trigger_index])) {
    ++trigger_index;
  }
  if (trigger_index >= frames.size()) trigger_index = start;

  std::vector<std::size_t> users;
  users.reserve(max_user_frames);
  for (std::size_t i = trigger_index + 1; i < frames.size() && users.size() < max_user_frames; ++i) {
    if (is_pluss_frame_(frames[i])) continue;
    if (!resolved[i].empty()) users.push_back(i);
  }
  if (users.empty()) {
    for (std::size_t i = start; i < frames.size() && users.size() < std::min<std::size_t>(max_user_frames, 4); ++i) {
      if (!resolved[i].empty()) users.push_back(i);
    }
  }

  std::ostringstream oss;
  std::size_t ui = 0;
  for (const auto idx : users) {
    oss << "  file: " << ui++ << "  "
        << format_native_frame_(frames[idx], resolved[idx]) << "\n";
  }
  if (trigger_index < frames.size()) {
    oss << "  pluss-trigger: "
        << format_native_frame_(frames[trigger_index], resolved[trigger_index])
        << "\n";
  }
  return oss.str();
}

} // namespace pluss::codem
