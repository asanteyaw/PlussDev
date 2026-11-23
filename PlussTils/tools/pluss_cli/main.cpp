// The `pluss` command: run a single C++ file against this PlussII, start a project, check the setup.
//
//   pluss run hello.cpp [more.cpp ...] [-- program arguments]
//   pluss new myproject
//   pluss doctor
//
// It lives in the bin folder of the extracted download and finds everything else from there, so it
// works wherever the download was unpacked. It needs only CMake and a C++20 compiler, which the
// download needs anyway; it adds no library of its own.

#include <pluss/version.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
extern char** environ;
#endif

namespace fs = std::filesystem;

namespace {

// ---- running other programs ---------------------------------------------------------------------

struct Outcome {
  int code{-1};
  std::string output;  // what the program printed, when it was captured
};

#if defined(_WIN32)

[[nodiscard]] std::wstring wide(const std::string& text) {
  if (text.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring out(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), size);
  return out;
}

// One argument quoted so that CommandLineToArgvW gives it back unchanged.
[[nodiscard]] std::wstring quoted(const std::wstring& arg) {
  if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) return arg;
  std::wstring out = L"\"";
  for (std::size_t i = 0; i < arg.size(); ++i) {
    std::size_t slashes = 0;
    while (i < arg.size() && arg[i] == L'\\') {
      ++slashes;
      ++i;
    }
    if (i == arg.size()) {
      out.append(slashes * 2, L'\\');
      break;
    }
    if (arg[i] == L'"') {
      out.append(slashes * 2 + 1, L'\\');
      out += L'"';
    } else {
      out.append(slashes, L'\\');
      out += arg[i];
    }
  }
  out += L'"';
  return out;
}

Outcome run_program(const std::vector<std::string>& args, bool capture) {
  Outcome outcome;
  std::wstring command;
  for (const auto& arg : args) command += (command.empty() ? L"" : L" ") + quoted(wide(arg));

  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};
  HANDLE log = INVALID_HANDLE_VALUE;
  fs::path log_path;
  if (capture) {
    log_path = fs::temp_directory_path() / ("pluss_cli_" + std::to_string(GetCurrentProcessId()) + ".log");
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    log = CreateFileW(log_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &inherit, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    startup.dwFlags |= STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = log;
    startup.hStdError = log;
  }
  if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, capture, 0, nullptr, nullptr, &startup, &process)) {
    outcome.output = "could not start '" + args.front() + "'";
    if (log != INVALID_HANDLE_VALUE) CloseHandle(log);
    return outcome;
  }
  WaitForSingleObject(process.hProcess, INFINITE);
  DWORD code = 1;
  GetExitCodeProcess(process.hProcess, &code);
  CloseHandle(process.hProcess);
  CloseHandle(process.hThread);
  if (log != INVALID_HANDLE_VALUE) {
    CloseHandle(log);
    std::ifstream in(log_path, std::ios::binary);
    outcome.output.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    std::error_code ignored;
    fs::remove(log_path, ignored);
  }
  outcome.code = static_cast<int>(code);
  return outcome;
}

#else

Outcome run_program(const std::vector<std::string>& args, bool capture) {
  Outcome outcome;
  std::vector<char*> argv;
  for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
  argv.push_back(nullptr);

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  fs::path log_path;
  if (capture) {
    log_path = fs::temp_directory_path() / ("pluss_cli_" + std::to_string(getpid()) + ".log");
    posix_spawn_file_actions_addopen(&actions, 1, log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    posix_spawn_file_actions_adddup2(&actions, 1, 2);
  }
  pid_t pid = 0;
  const int started = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  if (started != 0) {
    outcome.output = "could not start '" + args.front() + "'";
    return outcome;
  }
  int status = 0;
  waitpid(pid, &status, 0);
  outcome.code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
  if (capture) {
    std::ifstream in(log_path, std::ios::binary);
    outcome.output.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    std::error_code ignored;
    fs::remove(log_path, ignored);
  }
  return outcome;
}

#endif

// ---- where things are ----------------------------------------------------------------------------

[[nodiscard]] fs::path self_path() {
#if defined(_WIN32)
  std::wstring buffer(32768, L'\0');
  const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  buffer.resize(size);
  return fs::path(buffer);
#elif defined(__APPLE__)
  std::uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::string buffer(size, '\0');
  if (_NSGetExecutablePath(buffer.data(), &size) != 0) return {};
  return fs::weakly_canonical(fs::path(buffer.c_str()));
#else
  std::error_code ec;
  return fs::read_symlink("/proc/self/exe", ec);
#endif
}

// The extracted download this program came from: <prefix>/bin/pluss.
[[nodiscard]] fs::path sdk_prefix() {
  const fs::path self = self_path();
  if (self.empty()) return {};
  const fs::path prefix = self.parent_path().parent_path();
  return fs::exists(prefix / "include" / "pluss" / "pluss.h") ? prefix : fs::path{};
}

[[nodiscard]] fs::path cache_root() {
#if defined(_WIN32)
  if (const char* base = std::getenv("LOCALAPPDATA")) return fs::path(base) / "pluss" / "cache";
#elif defined(__APPLE__)
  if (const char* home = std::getenv("HOME")) return fs::path(home) / "Library" / "Caches" / "pluss";
#else
  if (const char* base = std::getenv("XDG_CACHE_HOME"); base != nullptr && *base != '\0') return fs::path(base) / "pluss";
  if (const char* home = std::getenv("HOME")) return fs::path(home) / ".cache" / "pluss";
#endif
  return fs::temp_directory_path() / "pluss";
}

[[nodiscard]] std::string slashes(const fs::path& path) {
  std::string text = path.generic_string();
  return text;
}

[[nodiscard]] std::string hash_hex(std::string_view text) {
  std::uint64_t h = 0xcbf29ce484222325ULL;
  for (const char ch : text) {
    h ^= static_cast<unsigned char>(ch);
    h *= 0x100000001b3ULL;
  }
  constexpr std::string_view digits = "0123456789abcdef";
  std::string out;
  for (int shift = 60; shift >= 0; shift -= 4) out += digits[(h >> shift) & 0xF];
  return out;
}

[[nodiscard]] std::string read_text(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Write a file only if its text changed, so an unchanged project is not reconfigured.
bool write_if_changed(const fs::path& path, const std::string& text) {
  if (fs::exists(path) && read_text(path) == text) return false;
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
  return true;
}

[[nodiscard]] std::vector<std::string> extra_cmake_args() {
  std::vector<std::string> out;
  if (const char* raw = std::getenv("PLUSS_CMAKE_ARGS")) {
    std::istringstream in(raw);
    std::string word;
    while (in >> word) out.push_back(word);
  }
  return out;
}

// What CMake needs to find this download and, on Windows, to use the toolset the download is built with.
[[nodiscard]] std::vector<std::string> configure_args(const fs::path& source, const fs::path& build, const fs::path& prefix, const std::string& type) {
  std::vector<std::string> args{"cmake", "-S", slashes(source), "-B", slashes(build), "-DCMAKE_PREFIX_PATH=" + slashes(prefix),
                                "-DCMAKE_BUILD_TYPE=" + type};
#if defined(_WIN32)
  args.insert(args.end(), {"-G", "Visual Studio 17 2022", "-T", "ClangCL"});
#endif
  for (auto& extra : extra_cmake_args()) args.push_back(std::move(extra));
  return args;
}

// ---- commands -----------------------------------------------------------------------------------

void usage() {
  std::cout << "pluss " << PLUSS_VERSION_STRING << " (" << PLUSS_RELEASE_CHANNEL << ")\n\n"
            << "  pluss run file.cpp [more.cpp ...] [-- arguments]   build against this PlussII and run\n"
            << "      --release | --debug     build type (default: RelWithDebInfo)\n"
            << "      --verbose               show the build output\n"
            << "      --clean                 discard the cached build first\n"
            << "  pluss new name                                     start a project in a new folder\n"
            << "  pluss doctor                                       check that this machine can build\n"
            << "  pluss version                                      print the version\n\n"
            << "Set PLUSS_CMAKE_ARGS to add arguments to the CMake configure step.\n";
}

bool is_source(std::string_view arg) {
  const auto dot = arg.rfind('.');
  if (dot == std::string_view::npos) return false;
  std::string ext(arg.substr(dot));
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext == ".cpp" || ext == ".cc" || ext == ".cxx";
}

[[nodiscard]] std::optional<fs::path> built_program(const fs::path& build, const std::string& type) {
  for (const fs::path& candidate : {build / "program", build / (type) / "program", build / "program.exe", build / type / "program.exe"}) {
    if (fs::is_regular_file(candidate)) return candidate;
  }
  return std::nullopt;
}

int command_run(const std::vector<std::string>& args) {
  const fs::path prefix = sdk_prefix();
  if (prefix.empty()) {
    std::cerr << "pluss: this program must stay in the bin folder of the extracted PlussII download.\n";
    return 2;
  }
  std::vector<fs::path> sources;
  std::vector<std::string> program_args;
  std::string type = "RelWithDebInfo";
  bool verbose = false, clean = false;
  bool options = true;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& arg = args[i];
    if (options && arg == "--") {
      options = false;
      for (std::size_t j = i + 1; j < args.size(); ++j) program_args.push_back(args[j]);
      break;
    }
    if (options && arg == "--release") type = "Release";
    else if (options && arg == "--debug") type = "Debug";
    else if (options && arg == "--verbose") verbose = true;
    else if (options && arg == "--clean") clean = true;
    else if (options && arg.rfind("--", 0) == 0) {
      std::cerr << "pluss run: unknown option '" << arg << "' (see: pluss)\n";
      return 2;
    } else if (is_source(arg) && program_args.empty()) {
      sources.push_back(fs::absolute(arg));
    } else {
      // The first word that is not a source file starts the program's own arguments.
      for (std::size_t j = i; j < args.size(); ++j) program_args.push_back(args[j]);
      break;
    }
  }
  if (sources.empty()) {
    std::cerr << "pluss run: give a .cpp file to build, for example: pluss run hello.cpp\n";
    return 2;
  }
  for (const auto& source : sources) {
    if (!fs::is_regular_file(source)) {
      std::cerr << "pluss run: '" << source.string() << "' is not a file\n";
      return 2;
    }
  }

  // One build folder per program, so editing it rebuilds incrementally.
  std::string identity = slashes(prefix) + "|" + PLUSS_VERSION_STRING + "|" + type;
  for (const auto& source : sources) identity += "|" + slashes(source);
  const fs::path work = cache_root() / "run" / hash_hex(identity);
  const fs::path build = work / "build";
  if (clean) {
    std::error_code ignored;
    fs::remove_all(work, ignored);
  }

  std::string project =
      "cmake_minimum_required(VERSION 3.28)\n"
      "project(pluss_run LANGUAGES CXX)\n"
      "set(CMAKE_CXX_STANDARD 20)\n"
      "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
      "set(CMAKE_CXX_EXTENSIONS OFF)\n"
      "find_package(Pluss CONFIG REQUIRED)\n"
      "add_executable(program";
  for (const auto& source : sources) project += "\n    \"" + slashes(source) + "\"";
  project += ")\ntarget_link_libraries(program PRIVATE Pluss::PlussII)\n";
  write_if_changed(work / "CMakeLists.txt", project);

  const Outcome configured = run_program(configure_args(work, build, prefix, type), !verbose);
  if (configured.code != 0) {
    std::cerr << configured.output << "\npluss: CMake could not set up the build. Run 'pluss doctor' to see what is missing.\n";
    return 1;
  }
  const Outcome built = run_program({"cmake", "--build", slashes(build), "--config", type, "--parallel"}, !verbose);
  if (built.code != 0) {
    std::cerr << built.output << "\npluss: the build failed.\n";
    return 1;
  }
  const auto program = built_program(build, type);
  if (!program) {
    std::cerr << "pluss: the build finished but there is no program to run.\n";
    return 1;
  }
  std::vector<std::string> command{program->string()};
  for (auto& arg : program_args) command.push_back(std::move(arg));
  return run_program(command, /*capture=*/false).code;
}

[[nodiscard]] bool valid_name(std::string_view name) {
  if (name.empty() || name == "." || name == "..") return false;
  return std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isalnum(c) != 0 || c == '_' || c == '-'; });
}

int command_new(const std::vector<std::string>& args) {
  const fs::path prefix = sdk_prefix();
  if (prefix.empty()) {
    std::cerr << "pluss: this program must stay in the bin folder of the extracted PlussII download.\n";
    return 2;
  }
  if (args.size() != 1 || !valid_name(args[0])) {
    std::cerr << "pluss new: give one project name made of letters, digits, '_' and '-', for example: pluss new myproject\n";
    return 2;
  }
  const std::string& name = args[0];
  const fs::path dir = fs::absolute(name);
  if (fs::exists(dir) && !fs::is_empty(dir)) {
    std::cerr << "pluss new: '" << dir.string() << "' already exists and is not empty\n";
    return 1;
  }
  std::string target = name;
  std::replace(target.begin(), target.end(), '-', '_');

  write_if_changed(dir / "CMakeLists.txt",
                   "cmake_minimum_required(VERSION 3.28)\n"
                   "project(" + target + " LANGUAGES CXX)\n\n"
                   "set(CMAKE_CXX_STANDARD 20)\n"
                   "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
                   "set(CMAKE_CXX_EXTENSIONS OFF)\n"
                   "if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)\n"
                   "  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING \"Build type\" FORCE)\n"
                   "endif()\n\n"
                   "find_package(Pluss CONFIG REQUIRED)\n\n"
                   "add_executable(" + target + " main.cpp)\n"
                   "target_link_libraries(" + target + " PRIVATE Pluss::PlussII)\n");
  write_if_changed(dir / "main.cpp",
                   "#include <pluss/pluss.h>\n\n"
                   "#include <iostream>\n\n"
                   "int main() {\n"
                   "  // A built-in example table; pluss::table::examples() lists the others.\n"
                   "  auto sales = pluss::table::example(\"sales\");\n"
                   "  std::cout << sales.head(6) << '\\n';\n"
                   "  std::cout << sales.describe() << '\\n';\n"
                   "}\n");
  write_if_changed(dir / "CMakePresets.json",
                   "{\n"
                   "  \"version\": 6,\n"
                   "  \"configurePresets\": [\n"
                   "    {\n"
                   "      \"name\": \"default\",\n"
                   "      \"binaryDir\": \"${sourceDir}/build\",\n"
                   "      \"cacheVariables\": {\n"
                   "        \"CMAKE_PREFIX_PATH\": \"" + slashes(prefix) + "\",\n"
                   "        \"CMAKE_BUILD_TYPE\": \"RelWithDebInfo\"\n"
                   "      }\n"
                   "    }\n"
                   "  ],\n"
                   "  \"buildPresets\": [ { \"name\": \"default\", \"configurePreset\": \"default\" } ]\n"
                   "}\n");
  write_if_changed(dir / ".gitignore", "build/\n");
  write_if_changed(dir / "README.md",
                   "# " + name + "\n\nA PlussII project.\n\n```sh\ncmake --preset default\ncmake --build --preset default\n./build/" + target + "\n```\n\n"
                   "The preset points CMake at the PlussII download this project was made with:\n`" + slashes(prefix) + "`.\n");
  std::cout << "created " << dir.string() << "\n\n  cd " << name << "\n  cmake --preset default\n  cmake --build --preset default\n  ./build/" << target << "\n";
  return 0;
}

// A small program that proves the whole chain: find the download, compile against it, link, run.
constexpr std::string_view kProbe =
    "#include <pluss/pluss.h>\n"
    "#include <pluss/tils/info.h>\n"
    "#include <iostream>\n"
    "int main() {\n"
    "  std::cout << pluss::tils::info::describe();\n"
    "  auto x = pluss::make_series({1.0, 2.0, 3.0});\n"
    "  std::cout << \"sum:       \" << x.sum().item<double>() << \"\\n\";\n"
    "}\n";

int command_doctor() {
  int problems = 0;
  const auto ok = [](const std::string& line) { std::cout << "  [ok]   " << line << "\n"; };
  const auto bad = [&](const std::string& line, const std::string& fix) {
    std::cout << "  [FAIL] " << line << "\n         " << fix << "\n";
    ++problems;
  };

  std::cout << "pluss " << PLUSS_VERSION_STRING << " (" << PLUSS_RELEASE_CHANNEL << ")\n\n";

  const fs::path prefix = sdk_prefix();
  if (prefix.empty()) {
    bad("the PlussII download", "this program must stay in the bin folder of the extracted download (it looks for include/pluss/pluss.h beside it)");
    return 1;
  }
  ok("PlussII download: " + prefix.string());
  if (fs::exists(prefix / "lib" / "cmake" / "Pluss" / "PlussConfig.cmake")) {
    ok("CMake package files found");
  } else {
    bad("CMake package files", "lib/cmake/Pluss/PlussConfig.cmake is missing from the download; extract the whole archive again");
  }

  const Outcome cmake = run_program({"cmake", "--version"}, true);
  if (cmake.code == 0) {
    std::string first = cmake.output.substr(0, cmake.output.find('\n'));
    int major = 0, minor = 0;
    const auto at = first.find_last_of(' ');
    if (at != std::string::npos && std::sscanf(first.c_str() + at + 1, "%d.%d", &major, &minor) == 2 && (major > 3 || (major == 3 && minor >= 28))) {
      ok(first);
    } else {
      bad(first, "PlussII needs CMake 3.28 or newer: install a newer CMake from cmake.org");
    }
  } else {
    bad("CMake", "'cmake' was not found on the PATH; install CMake 3.28 or newer from cmake.org");
    return 1;
  }

#if defined(__linux__)
  const Outcome zenity = run_program({"zenity", "--version"}, true);
  const Outcome kdialog = run_program({"kdialog", "--version"}, true);
  if (zenity.code == 0 || kdialog.code == 0) {
    ok("a file dialog for Save As (" + std::string(zenity.code == 0 ? "zenity" : "kdialog") + ")");
  } else {
    std::cout << "  [note] no zenity or kdialog: the PlussView Save As button will not be able to ask where to save\n";
  }
#endif

  // The real test: build and run a program against this download with this toolchain.
  const fs::path work = cache_root() / "doctor";
  std::error_code ignored;
  fs::remove_all(work, ignored);
  write_if_changed(work / "probe.cpp", std::string(kProbe));
  write_if_changed(work / "CMakeLists.txt",
                   "cmake_minimum_required(VERSION 3.28)\n"
                   "project(pluss_doctor LANGUAGES CXX)\n"
                   "set(CMAKE_CXX_STANDARD 20)\n"
                   "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
                   "set(CMAKE_CXX_EXTENSIONS OFF)\n"
                   "find_package(Pluss CONFIG REQUIRED)\n"
                   "add_executable(program probe.cpp)\n"
                   "target_link_libraries(program PRIVATE Pluss::PlussII)\n");
  const fs::path build = work / "build";
  const Outcome configured = run_program(configure_args(work, build, prefix, "RelWithDebInfo"), true);
  if (configured.code != 0) {
    bad("configuring a test project", "CMake could not find a working C++20 compiler or this download:\n" + configured.output);
    return 1;
  }
  ok("a C++ compiler was found and CMake can use this download");
  const Outcome built = run_program({"cmake", "--build", slashes(build), "--config", "RelWithDebInfo", "--parallel"}, true);
  if (built.code != 0) {
    bad("building a test program", "the compiler or linker failed:\n" + built.output);
    return 1;
  }
  ok("a test program compiled and linked against this download");
  const auto program = built_program(build, "RelWithDebInfo");
  if (!program) {
    bad("running the test program", "the build finished but produced no program");
    return 1;
  }
  const Outcome ran = run_program({program->string()}, true);
  if (ran.code != 0) {
    bad("running the test program", "it exited with code " + std::to_string(ran.code) + ":\n" + ran.output);
    return 1;
  }
  ok("the test program ran");
  std::cout << "\n" << ran.output;
  std::cout << (problems == 0 ? "\nEverything needed is in place.\n" : "\nSome checks failed; see above.\n");
  return problems == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") {
    usage();
    return args.empty() ? 2 : 0;
  }
  const std::string command = args[0];
  args.erase(args.begin());
  if (command == "run") return command_run(args);
  if (command == "new") return command_new(args);
  if (command == "doctor") return command_doctor();
  if (command == "version" || command == "--version") {
    std::cout << PLUSS_VERSION_STRING << " " << PLUSS_RELEASE_CHANNEL << "\n";
    return 0;
  }
  std::cerr << "pluss: unknown command '" << command << "'\n\n";
  usage();
  return 2;
}
