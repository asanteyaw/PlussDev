#include "CodeM/core/StackTrace.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__APPLE__) || defined(__unix__)
#include <dlfcn.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <execinfo.h>
#include <cxxabi.h>
#include <unistd.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#include <mutex>
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

static std::string add_in_tag_to_source_suffix_(const std::string& line) {
  if (line.empty()) return line;
  const auto open = line.rfind(" (");
  if (open == std::string::npos || line.back() != ')') return line;

  const std::string_view suffix(line.data() + open + 2, line.size() - (open + 3));
  if (!has_meaningful_source_line_(suffix)) return line;
  if (suffix.rfind("in ", 0) == 0) return line;

  std::string out;
  out.reserve(line.size() + 3);
  out.append(line, 0, open + 2); // includes " ("
  out += "in ";
  out.append(line, open + 2, std::string::npos);
  return out;
}

static std::string strip_image_tag_from_resolved_(const std::string& line) {
  // macOS `atos` often returns:
  //   function (in binary_name) (path/to/file.cpp:line)
  // Keep function + source only:
  //   function (path/to/file.cpp:line)
  const auto in_pos = line.find(" (in ");
  if (in_pos == std::string::npos) return line;

  const auto in_close = line.find(')', in_pos + 5);
  if (in_close == std::string::npos) return line;

  std::string out = line;
  out.erase(in_pos, in_close - in_pos + 1);
  while (in_pos < out.size() && out[in_pos] == ' ') {
    out.erase(in_pos, 1);
  }
  if (in_pos > 0 && in_pos < out.size() && out[in_pos - 1] == ' ' && out[in_pos] == ' ') {
    out.erase(in_pos, 1);
  }
  return add_in_tag_to_source_suffix_(out);
}

static std::string run_command_capture_(const std::string& cmd) {
  std::array<char, 512> buf{};
  std::string out;
#if defined(_WIN32)
  FILE* pipe = ::_popen(cmd.c_str(), "r");
#else
  FILE* pipe = ::popen(cmd.c_str(), "r");
#endif
  if (!pipe) return out;
  while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
    out.append(buf.data());
  }
#if defined(_WIN32)
  (void)::_pclose(pipe);
#else
  (void)::pclose(pipe);
#endif
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
  out = strip_image_tag_from_resolved_(out);
  out = add_in_tag_to_source_suffix_(out);
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
        line = strip_image_tag_from_resolved_(line);
        line = add_in_tag_to_source_suffix_(line);
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

namespace {

struct TraceFrame {
  std::string function;
  std::string file;
  std::uint32_t line{0};
  std::uint32_t column{0};
  std::string image;
};

static bool parse_atos_line_(const std::string& raw, TraceFrame& out) {
  // "function (in image) (/full/path/file.cpp:123)"  or  "function (in image) + 45"
  std::string s = raw;
  const auto in_pos = s.find(" (in ");
  if (in_pos == std::string::npos) return false;
  out.function = s.substr(0, in_pos);
  const auto in_close = s.find(')', in_pos + 5);
  if (in_close == std::string::npos) return false;
  out.image = s.substr(in_pos + 5, in_close - (in_pos + 5));
  const auto open = s.find(" (", in_close);
  if (open == std::string::npos || s.back() != ')') return true;
  const std::string loc = s.substr(open + 2, s.size() - open - 3);
  const auto colon = loc.rfind(':');
  if (colon == std::string::npos) return true;
  std::uint32_t line = 0;
  for (std::size_t k = colon + 1; k < loc.size(); ++k) {
    if (!std::isdigit(static_cast<unsigned char>(loc[k]))) return true;
    line = line * 10 + static_cast<std::uint32_t>(loc[k] - '0');
  }
  out.file = loc.substr(0, colon);
  out.line = line;
  return true;
}

// File paths in a traceback: library sources are shown relative to the repository root (the prefix is
// removed here as well as by the build, so objects built either way read the same), never as the
// directories of the machine that built the library.
static std::string forward_slashes_(std::string s) {
  for (char& c : s) {
    if (c == '\\') c = '/';
  }
  return s;
}

static bool has_drive_letter_(const std::string& s) {
  return s.size() >= 2 && std::isalpha(static_cast<unsigned char>(s[0])) && s[1] == ':';
}

static std::string shorten_path_impl_(std::string_view file_in, std::string_view this_file) {
  const std::string file = forward_slashes_(std::string(file_in));
  const std::string here = forward_slashes_(std::string(this_file));
  static const std::string marker = "/PlussQit/src/CodeM/core/";
  std::string root;
  if (const auto at = here.find(marker); at != std::string::npos) root = here.substr(0, at + 1);
  if (!root.empty()) {
    // Drive letters and directory names are not case-sensitive on Windows.
    const bool windows_style = has_drive_letter_(root);
    bool matches = file.size() >= root.size();
    for (std::size_t i = 0; matches && i < root.size(); ++i) {
      const char a = file[i], b = root[i];
      matches = windows_style ? std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)) : a == b;
    }
    if (matches) return file.substr(root.size());
  }
  if (file.rfind("./", 0) == 0) return file.substr(2);
  return file;
}

static std::string display_path_(const std::string& file) { return shorten_path_impl_(file, __FILE__); }

static bool parse_addr2line_impl_(std::string_view function_line, std::string_view location_line, Frame& out) {
  const auto colon = location_line.rfind(':');
  if (location_line.empty() || location_line.rfind("??", 0) == 0 || colon == std::string_view::npos) return false;
  // "file:42 (discriminator 3)": the line number is the digits after the last colon that is followed by a digit.
  std::size_t at = colon;
  while (at != std::string_view::npos && !(at + 1 < location_line.size() && std::isdigit(static_cast<unsigned char>(location_line[at + 1])))) {
    at = at == 0 ? std::string_view::npos : location_line.rfind(':', at - 1);
  }
  if (at == std::string_view::npos || at == 0) return false;
  out.function = std::string(function_line);
  out.file = std::string(location_line.substr(0, at));
  out.line = static_cast<std::uint32_t>(std::strtoul(std::string(location_line.substr(at + 1)).c_str(), nullptr, 10));
  return out.line != 0;  // addr2line prints line 0 when it knows the file but not the line
}

// Where a library source file may be found on a user's machine. A shipped library has no source tree at
// the build machine's path, so the repository-relative path is also looked up under $PLUSS_SOURCE_ROOT
// and under share/pluss/src next to the folder that holds the library, as an installed Python
// package finds its .py files. Without any of them a frame simply has no source line.
static std::vector<std::string> source_candidates_(const std::string& file) {
  std::vector<std::string> out{file};
  std::string rel = file;
  if (rel.rfind("./", 0) == 0) rel = rel.substr(2);
  if (!rel.empty() && rel[0] != '/') {
    if (const char* root = std::getenv("PLUSS_SOURCE_ROOT")) {
      if (*root) out.push_back(std::string(root) + "/" + rel);
    }
#if defined(__APPLE__) || defined(__unix__)
    Dl_info info{};
    if (::dladdr(reinterpret_cast<const void*>(&StackTrace::format_traceback), &info) != 0 && info.dli_fname) {
      std::string dir = info.dli_fname;
      const auto slash = dir.find_last_of('/');
      if (slash != std::string::npos) {
        dir.resize(slash);
        out.push_back(dir + "/../share/pluss/src/" + rel);
        out.push_back(dir + "/share/pluss/src/" + rel);
      }
    }
#endif
  }
  return out;
}

// Columns come from the debug information, which atos does not print; llvm-symbolizer does.
// Without it a traceback simply has no caret markers.
static std::string find_symbolizer_() {
#if defined(_WIN32)
  return {};  // caret markers need a Unix shell to find the symbolizer; Windows tracebacks have none
#else
  if (const char* env = std::getenv("PLUSS_SYMBOLIZER")) {
    if (*env) return env;
  }
  static const char* candidates[] = {"llvm-symbolizer", "/opt/homebrew/opt/llvm/bin/llvm-symbolizer",
                                     "/usr/local/opt/llvm/bin/llvm-symbolizer", "/usr/bin/llvm-symbolizer"};
  for (const char* c : candidates) {
    const std::string probe = std::string("command -v ") + shell_escape_(c) + " >/dev/null 2>&1";
    if (std::system(probe.c_str()) == 0) return c;
  }
  return {};
#endif
}

#if defined(__APPLE__)
static std::uintptr_t image_slide_(const void* header) {
  const std::uint32_t n = _dyld_image_count();
  for (std::uint32_t i = 0; i < n; ++i) {
    if (reinterpret_cast<const void*>(_dyld_get_image_header(i)) == header) {
      return static_cast<std::uintptr_t>(_dyld_get_image_vmaddr_slide(i));
    }
  }
  return 0;
}
#endif

// Underline the call expression that starts at `column` (1-based) in `line`, as Python does:
// from the start of the postfix chain (a.b->c::d) to the closing parenthesis of its call.
// Returns false when the marker would cover the whole statement, where Python prints none.
static bool caret_span_(const std::string& line, std::uint32_t column, std::size_t& begin, std::size_t& end) {
  if (column == 0 || column > line.size()) return false;
  std::size_t b = column - 1;
  auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
  while (b > 0) {
    const char p = line[b - 1];
    if (word(p) || p == '.' || p == ':') { --b; continue; }
    if (p == '>' && b > 1 && line[b - 2] == '-') { b -= 2; continue; }
    break;
  }
  std::size_t e = column - 1;
  while (e < line.size() && (word(line[e]) || line[e] == ':' || line[e] == '.')) ++e;
  while (e < line.size() && line[e] == ' ') ++e;
  if (e < line.size() && line[e] == '(') {
    int depth = 0;
    bool instr = false;
    for (; e < line.size(); ++e) {
      const char c = line[e];
      if (instr) { if (c == '\\') ++e; else if (c == '"') instr = false; continue; }
      if (c == '"') instr = true;
      else if (c == '(') ++depth;
      else if (c == ')' && --depth == 0) { ++e; break; }
    }
  }
  if (e <= b) return false;
  const auto first = line.find_first_not_of(" \t");
  const auto last = line.find_last_not_of(" \t\r;");
  if (b <= first && e > last) return false;
  begin = b;
  end = e;
  return true;
}

static std::string source_raw_(const std::string& file, std::uint32_t line, std::unordered_map<std::string, std::vector<std::string>>& cache) {
  if (file.empty() || line == 0) return {};
  auto it = cache.find(file);
  if (it == cache.end()) {
    std::vector<std::string> lines;
    for (const auto& candidate : source_candidates_(file)) {
      std::ifstream in(candidate);
      if (!in) continue;
      std::string l;
      while (std::getline(in, l)) lines.push_back(std::move(l));
      break;
    }
    it = cache.emplace(file, std::move(lines)).first;
  }
  if (line > it->second.size()) return {};
  std::string text = it->second[line - 1];
  while (!text.empty() && (text.back() == '\r')) text.pop_back();
  return text;
}

static std::string source_text_(const std::string& file, std::uint32_t line, std::unordered_map<std::string, std::vector<std::string>>& cache) {
  const std::string text = source_raw_(file, line, cache);
  const auto b = text.find_first_not_of(" \t");
  if (b == std::string::npos) return {};
  const auto e = text.find_last_not_of(" \t\r");
  return text.substr(b, e - b + 1);
}

} // namespace

namespace {

#if defined(_WIN32) && defined(_MSC_VER)
// Windows: the DbgHelp library reads the program's PDB files, so no external tool is needed. DbgHelp is
// not thread-safe, hence the lock.
static void win_resolve_(const std::vector<NativeFrame>& frames, std::vector<TraceFrame>& out) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  const HANDLE process = ::GetCurrentProcess();
  static const bool initialised = [&] {
    ::SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    return ::SymInitialize(process, nullptr, TRUE) != FALSE;
  }();
  if (!initialised) return;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const DWORD64 address = reinterpret_cast<DWORD64>(frames[i].addr) - (i == 0 ? 0 : 1);
    alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256];
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    std::memset(symbol, 0, sizeof(buffer));
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 255;
    DWORD64 displacement = 0;
    if (::SymFromAddr(process, address, &displacement, symbol)) out[i].function = symbol->Name;
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    DWORD line_displacement = 0;
    if (::SymGetLineFromAddr64(process, address, &line_displacement, &line) && line.FileName) {
      out[i].file = line.FileName;
      out[i].line = line.LineNumber;
    }
  }
}
#endif

// ---- Readable frames -------------------------------------------------------------------------------------------
// A frame inside a template-heavy library call can have a function name of several hundred characters. What a
// reader needs is the function and where it is, so nested template arguments and long parameter lists are folded
// to "<...>" and "(...)", and the libc++ inline namespace is dropped. The cut is visible, never silent.
static std::string shorten_function_name_(std::string name) {
  if (name.rfind("_Z", 0) == 0) name = demangle_symbol_(name.c_str());  // a lambda or an anonymous function arrives mangled
  // libc++ puts everything under an inline namespace std::__1; the reader writes std::.
  for (std::size_t at; (at = name.find("std::__1::")) != std::string::npos;) name.replace(at, 10, "std::");
  constexpr std::size_t kMaxNameLength = 100;
  if (name.size() <= kMaxNameLength) return name;
  std::string out;
  out.reserve(name.size());
  int angle = 0;
  int paren = 0;
  std::size_t paren_start = 0;
  for (std::size_t i = 0; i < name.size(); ++i) {
    const char c = name[i];
    const bool after_operator = i >= 8 && name.compare(i - 8, 8, "operator") == 0;
    if (c == '<' && !after_operator && paren == 0) {
      if (angle++ == 0) out += "<...>";
      continue;
    }
    if (c == '>' && angle > 0 && paren == 0) {
      --angle;
      continue;
    }
    if (angle > 0) continue;
    if (c == '(') {
      if (paren++ == 0) paren_start = out.size();
      out += c;
      continue;
    }
    if (c == ')' && paren > 0) {
      if (--paren == 0 && out.size() - paren_start > 60) {
        out.resize(paren_start);
        out += "(...";
      }
      out += c;
      continue;
    }
    out += c;
  }
  return out;
}

// Frames that are only machinery of the standard library (a std::visit dispatcher, an invoker): they are folded into
// one note so the frames of the program and of the library being called stand out.
static bool is_standard_library_noise_(const std::string& name) {
  return name.rfind("std::", 0) == 0 || name.find("decltype(auto) std::") != std::string::npos ||
         name.find("__variant_detail") != std::string::npos || name.find("__invoke") != std::string::npos;
}

// Fill `out` (same length as `frames`) with function, file and line for each frame.
static void resolve_frames_(const std::vector<NativeFrame>& frames, std::vector<TraceFrame>& out) {
#if defined(__APPLE__)
  std::unordered_map<std::string, std::vector<std::size_t>> groups;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (frames[i].image_path.empty()) continue;
    groups[frames[i].image_path + '\x1f' + ptr_hex_(frames[i].image_base)].push_back(i);
  }
  for (const auto& [_, idxs] : groups) {
    const auto& f0 = frames[idxs.front()];
    std::string cmd = "/usr/bin/atos -fullPath -o " + shell_escape_(f0.image_path);
    if (f0.image_base != nullptr) cmd += " -l " + ptr_hex_(f0.image_base);
    for (const auto idx : idxs) {
      const auto pc = reinterpret_cast<std::uintptr_t>(frames[idx].addr);
      cmd += " " + ptr_hex_(reinterpret_cast<const void*>(pc - (idx == 0 ? 0 : 1)));
    }
    cmd += " 2>/dev/null";
    std::istringstream iss(run_command_capture_(cmd));
    std::string line;
    for (const auto idx : idxs) {
      if (!std::getline(iss, line)) break;
      parse_atos_line_(line, out[idx]);
    }
  }
#elif defined(__unix__)
  // addr2line wants addresses relative to the image for position-independent code and absolute ones for
  // a fixed-address executable; ask with the relative form first and the absolute form for what is left.
  std::unordered_map<std::string, std::vector<std::size_t>> groups;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (frames[i].image_path.empty()) continue;
    groups[frames[i].image_path + '\x1f' + ptr_hex_(frames[i].image_base)].push_back(i);
  }
  auto run = [&](const std::vector<std::size_t>& idxs, bool relative) {
    const auto& f0 = frames[idxs.front()];
    std::string cmd = "addr2line -f -C -e " + shell_escape_(f0.image_path);
    const auto base = reinterpret_cast<std::uintptr_t>(f0.image_base);
    for (const auto idx : idxs) {
      const auto pc = reinterpret_cast<std::uintptr_t>(frames[idx].addr) - (idx == 0 ? 0 : 1);
      cmd += " " + ptr_hex_(reinterpret_cast<const void*>(relative ? pc - base : pc));
    }
    cmd += " 2>/dev/null";
    std::istringstream iss(run_command_capture_(cmd));
    std::string fn, src;
    std::vector<std::size_t> unresolved;
    for (const auto idx : idxs) {
      if (!std::getline(iss, fn) || !std::getline(iss, src)) { unresolved.push_back(idx); continue; }
      Frame parsed;
      if (!parse_addr2line_impl_(fn, src, parsed)) { unresolved.push_back(idx); continue; }
      out[idx].function = parsed.function;
      out[idx].file = parsed.file;
      out[idx].line = parsed.line;
    }
    return unresolved;
  };
  for (const auto& [_, idxs] : groups) {
    const auto left = run(idxs, true);
    if (!left.empty() && left.size() == idxs.size()) run(left, false);
  }
#elif defined(_WIN32) && defined(_MSC_VER)
  win_resolve_(frames, out);
#else
  (void)frames; (void)out;
#endif
}

} // namespace

std::string StackTrace::shorten_path(std::string_view file, std::string_view this_file) {
  return shorten_path_impl_(file, this_file);
}

bool StackTrace::parse_addr2line(std::string_view function_line, std::string_view location_line, Frame& out) {
  return parse_addr2line_impl_(function_line, location_line, out);
}

std::size_t StackTrace::capture_addresses(void** out, std::size_t capacity) {
#if defined(__APPLE__) || defined(__unix__)
  const int n = ::backtrace(out, static_cast<int>(std::min<std::size_t>(capacity, 256)));
  return n > 0 ? static_cast<std::size_t>(n) : 0;
#elif defined(_WIN32) && defined(_MSC_VER)
  return static_cast<std::size_t>(::CaptureStackBackTrace(0, static_cast<DWORD>(std::min<std::size_t>(capacity, 256)), out, nullptr));
#else
  (void)out; (void)capacity;
  return 0;
#endif
}

std::string StackTrace::format_traceback(
    const void* const* addrs,
    std::size_t count,
    const char* raise_file,
    std::uint32_t raise_line,
    const char* raise_function,
    const char* raise_source) {
  std::vector<NativeFrame> frames;
#if defined(__APPLE__) || defined(__unix__)
  frames.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    NativeFrame f;
    f.addr = const_cast<void*>(addrs[i]);
    Dl_info info{};
    if (::dladdr(addrs[i], &info) != 0) {
      if (info.dli_fname) f.image_path = info.dli_fname;
      if (info.dli_sname) f.symbol = demangle_symbol_(info.dli_sname);
      f.image_base = info.dli_fbase;
    }
    frames.push_back(std::move(f));
  }
#else
  frames.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    NativeFrame f;
    f.addr = const_cast<void*>(addrs[i]);
    frames.push_back(std::move(f));
  }
#endif
  // Resolve every captured frame to function, file and line in batches (one tool call per image). A return
  // address points after the call, so every frame but the newest is looked up one byte earlier.
  std::vector<TraceFrame> all_resolved(frames.size());
  resolve_frames_(frames, all_resolved);

  auto name_of = [&](std::size_t i) -> const std::string& {
    return all_resolved[i].function.empty() ? frames[i].symbol : all_resolved[i].function;
  };
  // Drop the error machinery at the top, and everything below main().
  std::size_t first = 0;
  while (first < frames.size() &&
         (name_of(first).rfind("pluss::codem::", 0) == 0 ||
          name_of(first).rfind("pluss::Error", 0) == 0 ||
          name_of(first).rfind("pluss::warn", 0) == 0 ||
          name_of(first).rfind("pluss::throw_error", 0) == 0)) {
    ++first;
  }
  std::size_t last = frames.size();
  for (std::size_t i = first; i < frames.size(); ++i) {
    if (name_of(i) == "main" || name_of(i).rfind("main(", 0) == 0) { last = i + 1; break; }
  }
  first = std::min(first, frames.size());
  last = std::min(last, frames.size());
  std::vector<NativeFrame> kept(frames.begin() + static_cast<std::ptrdiff_t>(first), frames.begin() + static_cast<std::ptrdiff_t>(last));
  std::vector<TraceFrame> resolved(all_resolved.begin() + static_cast<std::ptrdiff_t>(first), all_resolved.begin() + static_cast<std::ptrdiff_t>(last));
  constexpr std::size_t kMaxFrames = 200;
  std::size_t omitted = 0;
  if (kept.size() > kMaxFrames) { omitted = kept.size() - kMaxFrames; kept.resize(kMaxFrames); resolved.resize(kMaxFrames); }  // newest kept

  // The newest frame is the raise site itself: exact without debug information.
  if (!kept.empty() || raise_file) {
    if (resolved.empty()) { resolved.emplace_back(); kept.emplace_back(); }
    if (raise_file && *raise_file) { resolved[0].file = raise_file; resolved[0].line = raise_line; }
    if (resolved[0].function.empty() && raise_function) resolved[0].function = raise_function;
  }

  std::unordered_map<std::string, std::vector<std::string>> sources;
  std::ostringstream oss;
  oss << "Traceback (most recent call last):\n";
  if (omitted > 0) oss << "  ... " << omitted << " earlier frame" << (omitted == 1 ? "" : "s") << " omitted\n";
  // Columns for the caret markers: one symbolizer call per image, only for frames whose source is readable.
  {
    const std::string symbolizer = find_symbolizer_();
    if (!symbolizer.empty()) {
      std::unordered_map<std::string, std::vector<std::size_t>> groups;
      for (std::size_t i = 0; i < kept.size(); ++i) {
        if (resolved[i].line == 0 || resolved[i].file.empty() || kept[i].image_path.empty()) continue;
        if (i == 0 && raise_source && *raise_source) continue;
        if (source_raw_(resolved[i].file, resolved[i].line, sources).empty()) continue;
        groups[kept[i].image_path + '\x1f' + ptr_hex_(kept[i].image_base)].push_back(i);
      }
      for (const auto& [_, idxs] : groups) {
        const auto& f0 = kept[idxs.front()];
        std::uintptr_t slide = 0;
#if defined(__APPLE__)
        slide = image_slide_(f0.image_base);
#else
        slide = reinterpret_cast<std::uintptr_t>(f0.image_base);
#endif
        std::string cmd = shell_escape_(symbolizer) + " --obj=" + shell_escape_(f0.image_path);
#if !defined(__APPLE__)
        cmd += " --relative-address";
#endif
        for (const auto idx : idxs) {
          const auto pc = reinterpret_cast<std::uintptr_t>(kept[idx].addr) - (idx == 0 ? 0 : 1) - slide;
          cmd += " " + ptr_hex_(reinterpret_cast<const void*>(pc));
        }
        cmd += " 2>/dev/null";
        std::istringstream iss(run_command_capture_(cmd));
        std::string fn, loc, blank;
        for (const auto idx : idxs) {
          if (!std::getline(iss, fn) || !std::getline(iss, loc)) break;
          std::getline(iss, blank);
          // file:line:column
          const auto c2 = loc.rfind(':');
          if (c2 == std::string::npos) continue;
          const auto c1 = loc.rfind(':', c2 - 1);
          if (c1 == std::string::npos) continue;
          const std::uint32_t line = static_cast<std::uint32_t>(std::strtoul(loc.c_str() + c1 + 1, nullptr, 10));
          const std::uint32_t col = static_cast<std::uint32_t>(std::strtoul(loc.c_str() + c2 + 1, nullptr, 10));
          if (line == resolved[idx].line) resolved[idx].column = col;
        }
      }
    }
  }

  bool missing_lines = false;
  // Frames oldest to newest. Like Python, a run of the same frame (recursion) shows the first three
  // and then "[Previous line repeated N more times]".
  constexpr std::size_t kRepeatCutoff = 3;
  std::string last_key;
  std::size_t repeats = 0;
  auto flush_repeats = [&] {
    if (repeats > kRepeatCutoff) {
      const std::size_t more = repeats - kRepeatCutoff;
      oss << "  [Previous line repeated " << more << " more time" << (more > 1 ? "s" : "") << "]\n";
    }
  };
  std::size_t noise_run = 0;
  auto flush_noise = [&] {
    if (noise_run > 0) {
      oss << "  [" << noise_run << " standard library frame" << (noise_run == 1 ? "" : "s") << " hidden]\n";
      noise_run = 0;
    }
  };
  for (std::size_t i = kept.size(); i-- > 0;) {
    const auto& r = resolved[i];
    std::string fn = r.function.empty() ? (kept[i].symbol.empty() ? std::string("<unknown>") : kept[i].symbol) : r.function;
    // The raise site names itself through source_location, which is readable even when the symbol is not.
    if (i == 0 && fn.rfind("_Z", 0) == 0 && raise_function && *raise_function) fn = raise_function;
    // The raise site is always shown; other frames that are standard-library machinery are folded.
    // A std wrapper around the program's own lambda (std::function, std::invoke) is the program's frame.
    const bool wraps_lambda = fn.find("$_") != std::string::npos || fn.find("lambda") != std::string::npos;
    if (i != 0 && !wraps_lambda && is_standard_library_noise_(shorten_function_name_(fn))) {
      ++noise_run;
      continue;
    }
    fn = shorten_function_name_(std::move(fn));
    const std::string key = display_path_(r.file) + ":" + std::to_string(r.line) + ":" + fn;
    // Hidden frames inside a run of the same frame (recursion through std::function, say) belong to the run.
    if (key == last_key) {
      noise_run = 0;
    } else {
      flush_repeats();
      flush_noise();
      last_key = key;
      repeats = 0;
    }
    std::string frame_text;
    if (!r.file.empty() && r.line > 0) {
      frame_text += "  File \"" + display_path_(r.file) + "\", line " + std::to_string(r.line) + ", in " + fn + "\n";
      std::string text = (i == 0 && raise_source && *raise_source) ? std::string(raise_source) : source_text_(r.file, r.line, sources);
      if (!text.empty()) frame_text += "    " + text + "\n";
      if (!(i == 0 && raise_source && *raise_source) && r.column > 0) {
        const std::string raw = source_raw_(r.file, r.line, sources);
        std::size_t cb = 0, ce = 0;
        if (caret_span_(raw, r.column, cb, ce)) {
          const auto lead = raw.find_first_not_of(" \t");
          frame_text += "    " + std::string(cb > lead ? cb - lead : 0, ' ') + std::string(ce - cb, '^') + "\n";
        }
      }
    } else {
      missing_lines = true;
      frame_text += "  File \"" + (kept[i].image_path.empty() ? std::string("<unknown>") : image_basename_(kept[i].image_path)) +
                    "\", in " + fn + "\n";
    }
    ++repeats;
    if (repeats <= kRepeatCutoff) oss << frame_text;
  }
  flush_repeats();
  flush_noise();
  if (missing_lines) oss << "  (frames without a line were built without debug information; compile with -g to see them)\n";
  return oss.str();
}

} // namespace pluss::codem
