#include "CodeM/core/StackTrace.h"
#include <sstream>
#include <cstdlib>

namespace pluss::codem {

thread_local std::vector<Frame> StackTrace::frames_{};

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

} // namespace pluss::codem
