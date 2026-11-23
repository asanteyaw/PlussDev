#pragma once

#include <pluss/codem/codem.h>
#include <pluss/version.h>

#include <string>
#include <vector>

namespace pluss::tils::info {

// What this program is running on and what it was built with: the facts to record beside a result.
struct BuildInfo {
  std::string version;                 // "0.0.6"
  std::string channel;                 // "steady", "transient" or "development"
  std::string build_type;              // "Release", "RelWithDebInfo", ...
  std::string compiler;                // "AppleClang 17.0.0.17000013"
  std::string system;                  // "macOS", "Linux" or "Windows"
  std::string architecture;            // "arm64" or "x86_64"
  std::vector<std::string> platforms;  // compute platforms usable here, "cpu" first
};

namespace detail {

[[nodiscard]] constexpr const char* system_name_() noexcept {
#if defined(__APPLE__)
  return "macOS";
#elif defined(__linux__)
  return "Linux";
#elif defined(_WIN32)
  return "Windows";
#else
  return "unknown";
#endif
}

[[nodiscard]] constexpr const char* architecture_name_() noexcept {
#if defined(__aarch64__) || defined(_M_ARM64)
  return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
  return "x86_64";
#else
  return "unknown";
#endif
}

}  // namespace detail

// The build this call is made from, with the compute platforms that are usable on this machine.
[[nodiscard]] inline BuildInfo build() {
  const Version v = version();
  BuildInfo out;
  out.version = std::string(v.text);
  out.channel = std::string(v.channel);
  out.build_type = std::string(v.build_type);
  out.compiler = std::string(v.compiler);
  out.system = detail::system_name_();
  out.architecture = detail::architecture_name_();
  out.platforms.emplace_back("cpu");
  if (pluss::codem::mps_backend_available()) out.platforms.emplace_back("mps");
  if (pluss::codem::cuda_backend_available()) out.platforms.emplace_back("cuda");
  return out;
}

// The same facts as readable lines, for a log or a bug report.
[[nodiscard]] inline std::string describe(const BuildInfo& info) {
  std::string platforms;
  for (const auto& name : info.platforms) {
    platforms += platforms.empty() ? name : ", " + name;
  }
  return "PlussII " + info.version + " (" + info.channel + ", " + info.build_type + ")\n" +
         "compiler:  " + info.compiler + "\n" + "system:    " + info.system + " " + info.architecture + "\n" +
         "platforms: " + platforms + "\n";
}

[[nodiscard]] inline std::string describe() { return describe(build()); }

}  // namespace pluss::tils::info
