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
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
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
            << "  pluss kernel install [options]                     add the PlussII C++ kernel to Jupyter and VS Code\n"
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

// ---- run: one shared project per download, so every program reuses one precompiled header -------

[[nodiscard]] std::optional<fs::path> built_program(const fs::path& dir, const std::string& stem) {
  for (const fs::path& candidate : {dir / stem, dir / (stem + ".exe")}) {
    if (fs::is_regular_file(candidate)) return candidate;
  }
  return std::nullopt;
}

// Only one `pluss run` at a time may change the shared project; the others wait their turn.
class WorkLock {
 public:
  explicit WorkLock(const fs::path& work) : path_(work / "lock") {
    std::error_code ec;
    fs::create_directories(work, ec);
    for (int attempt = 0; attempt < 1200; ++attempt) {  // up to a minute, then assume a crashed run left it
      if (fs::create_directory(path_, ec)) {
        held_ = true;
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    held_ = true;
  }
  ~WorkLock() {
    if (held_) {
      std::error_code ec;
      fs::remove(path_, ec);
    }
  }
  WorkLock(const WorkLock&) = delete;
  WorkLock& operator=(const WorkLock&) = delete;

 private:
  fs::path path_;
  bool held_{false};
};

struct Program {
  std::string id;  // a hash of its source files
  std::string stem;
  std::vector<fs::path> sources;
};

[[nodiscard]] bool is_header(const fs::path& path) {
  const std::string ext = path.extension().string();
  return ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx" || ext == ".inc";
}

// True when the built program is newer than its sources and the headers beside them, so nothing
// needs to be looked at by the build tool at all.
[[nodiscard]] bool up_to_date(const fs::path& binary, const Program& program) {
  std::error_code ec;
  const auto built = fs::last_write_time(binary, ec);
  if (ec) return false;
  for (const auto& source : program.sources) {
    const auto when = fs::last_write_time(source, ec);
    if (ec || when > built) return false;
    for (const auto& entry : fs::directory_iterator(source.parent_path(), ec)) {
      if (is_header(entry.path()) && fs::last_write_time(entry.path(), ec) > built) return false;
    }
  }
  return true;
}

// A hash of what the program is made from: its sources and the headers beside them. File times are
// not enough on their own: a file written within a second of the last build can look unchanged to the
// build tool, and a program that is edited and run in a loop must always run its latest text.
[[nodiscard]] std::string content_hash(const Program& program) {
  std::uint64_t h = 0xcbf29ce484222325ULL;
  const auto feed = [&h](std::string_view text) {
    for (const char ch : text) {
      h ^= static_cast<unsigned char>(ch);
      h *= 0x100000001b3ULL;
    }
  };
  std::vector<fs::path> files(program.sources);
  std::error_code ec;
  for (const auto& source : program.sources) {
    for (const auto& entry : fs::directory_iterator(source.parent_path(), ec)) {
      if (is_header(entry.path()) && files.size() < 256) files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  files.erase(std::unique(files.begin(), files.end()), files.end());
  for (const auto& file : files) {
    feed(slashes(file));
    feed(read_text(file));
  }
  constexpr std::string_view digits = "0123456789abcdef";
  std::string out;
  for (int shift = 60; shift >= 0; shift -= 4) out += digits[(h >> shift) & 0xF];
  return out;
}

// What each program was last built from, by content hash.
[[nodiscard]] std::map<std::string, std::string> read_built(const fs::path& work) {
  std::map<std::string, std::string> out;
  std::ifstream in(work / "built.txt");
  std::string id, hash;
  while (in >> id >> hash) out[id] = hash;
  return out;
}

void write_built(const fs::path& work, const std::map<std::string, std::string>& built) {
  std::string text;
  for (const auto& [id, hash] : built) text += id + " " + hash + "\n";
  write_if_changed(work / "built.txt", text);
}

// A program whose text changed must be compiled again even if the build tool cannot tell: take its
// object files away so there is nothing to mistake for current.
void forget_objects(const fs::path& build, const std::string& id) {
  std::error_code ec;
  const fs::path dir = build / "CMakeFiles" / ("p_" + id + ".dir");
  if (!fs::is_directory(dir, ec)) return;
  for (const auto& entry : fs::recursive_directory_iterator(dir, ec)) {
    const std::string ext = entry.path().extension().string();
    if (entry.is_regular_file(ec) && (ext == ".o" || ext == ".obj")) fs::remove(entry.path(), ec);
  }
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

  // One project per download and build type. Every program is a target of it, and they all reuse one
  // precompiled header of <pluss/pluss.h>, so only the first program pays for parsing the headers.
  const fs::path work = cache_root() / "run" / hash_hex(slashes(prefix) + "|" + PLUSS_VERSION_STRING + "|" + type);
  const fs::path build = work / "build";
  const WorkLock lock(work);
  if (clean) {
    std::error_code ignored;
    fs::remove_all(work / "build", ignored);
    fs::remove(work / "programs.txt", ignored);
  }

  Program program;
  std::string joined;
  for (const auto& source : sources) joined += slashes(source) + "|";
  program.id = hash_hex(joined);
  program.stem = sources.front().stem().string();
  program.sources = sources;

  // The programs run so far, so none of them is rebuilt when another is added; ones whose files are gone are dropped.
  std::vector<Program> known;
  {
    std::ifstream registry(work / "programs.txt");
    std::string line;
    while (std::getline(registry, line)) {
      const auto tab = line.find('\t');
      const auto tab2 = line.find('\t', tab == std::string::npos ? 0 : tab + 1);
      if (tab == std::string::npos || tab2 == std::string::npos) continue;
      Program other;
      other.id = line.substr(0, tab);
      other.stem = line.substr(tab + 1, tab2 - tab - 1);
      std::istringstream files(line.substr(tab2 + 1));
      std::string file;
      bool present = true;
      while (std::getline(files, file, ';')) {
        other.sources.emplace_back(file);
        present = present && fs::is_regular_file(file);
      }
      if (present && other.id != program.id) known.push_back(std::move(other));
    }
  }
  known.push_back(program);
  std::sort(known.begin(), known.end(), [](const Program& x, const Program& y) { return x.id < y.id; });
  std::string registry_text, project =
      "cmake_minimum_required(VERSION 3.28)\n"
      "project(pluss_run LANGUAGES CXX)\n"
      "set(CMAKE_CXX_STANDARD 20)\n"
      "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
      "set(CMAKE_CXX_EXTENSIONS OFF)\n"
      "find_package(Pluss CONFIG REQUIRED)\n"
      "file(WRITE ${CMAKE_BINARY_DIR}/anchor.cpp \"\")\n"
      "add_library(pluss_pch STATIC ${CMAKE_BINARY_DIR}/anchor.cpp)\n"
      "target_link_libraries(pluss_pch PUBLIC Pluss::PlussII)\n"
      "target_precompile_headers(pluss_pch PUBLIC <pluss/pluss.h>)\n";
  for (const auto& item : known) {
    std::string files;
    for (const auto& source : item.sources) files += (files.empty() ? "" : ";") + slashes(source);
    registry_text += item.id + "\t" + item.stem + "\t" + files + "\n";
    project += "add_executable(p_" + item.id;
    for (const auto& source : item.sources) project += "\n    \"" + slashes(source) + "\"";
    project += ")\ntarget_link_libraries(p_" + item.id + " PRIVATE pluss_pch Pluss::PlussII)\n"
               "target_precompile_headers(p_" + item.id + " REUSE_FROM pluss_pch)\n"
               "set_target_properties(p_" + item.id + " PROPERTIES OUTPUT_NAME \"" + item.stem + "\" RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/bin/" + item.id + "\"";
    for (const char* config : {"DEBUG", "RELEASE", "RELWITHDEBINFO", "MINSIZEREL"}) {
      project += std::string(" RUNTIME_OUTPUT_DIRECTORY_") + config + " \"${CMAKE_BINARY_DIR}/bin/" + item.id + "\"";
    }
    project += ")\n";
  }
  const bool changed = write_if_changed(work / "CMakeLists.txt", project);
  write_if_changed(work / "programs.txt", registry_text);

  const fs::path bin = build / "bin" / program.id;
  auto built_hashes = read_built(work);
  const std::string hash = content_hash(program);
  const bool same_text = built_hashes.count(program.id) != 0 && built_hashes[program.id] == hash;
  // Nothing to build when the program was made from exactly this text and nothing is newer.
  if (!changed && !clean && same_text) {
    if (const auto existing = built_program(bin, program.stem); existing && up_to_date(*existing, program)) {
      std::vector<std::string> command{existing->string()};
      for (auto& arg : program_args) command.push_back(std::move(arg));
      return run_program(command, /*capture=*/false).code;
    }
  }
  if (!same_text) forget_objects(build, program.id);

  if (changed || !fs::exists(build / "CMakeCache.txt")) {
    const Outcome configured = run_program(configure_args(work, build, prefix, type), !verbose);
    if (configured.code != 0) {
      std::cerr << configured.output << "\npluss: CMake could not set up the build. Run 'pluss doctor' to see what is missing.\n";
      return 1;
    }
  }
  const Outcome built = run_program({"cmake", "--build", slashes(build), "--config", type, "--target", "p_" + program.id, "--parallel"}, !verbose);
  if (built.code != 0) {
    std::cerr << built.output << "\npluss: the build failed.\n";
    return 1;
  }
  const auto binary = built_program(bin, program.stem);
  if (!binary) {
    std::cerr << "pluss: the build finished but there is no program to run.\n";
    return 1;
  }
  built_hashes[program.id] = hash;
  write_built(work, built_hashes);
  std::vector<std::string> command{binary->string()};
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
                   "#include <pluss/pluss.h>\n"
                   "#include <pluss/tils/kit/datastore.h>\n\n"
                   "#include <iostream>\n\n"
                   "int main() {\n"
                   "  // A built-in dataset; pluss::loader::datastores() lists the others.\n"
                   "  auto sales = pluss::loader::datastore(\"sales\").frame();\n"
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
  auto program = built_program(build, "program");
  if (!program) program = built_program(build / "RelWithDebInfo", "program");  // a multi-configuration generator
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

// ---- kernel: the Jupyter kernel that comes with the download -------------------------------------

int command_kernel(const std::vector<std::string>& args) {
  if (args.empty() || args[0] != "install") {
    std::cerr << "usage: pluss kernel install [--clang-repl path] [--sysroot path]\n";
    return 2;
  }
  const fs::path prefix = sdk_prefix();
  const fs::path script = prefix / "share" / "pluss" / "notebook" / "install_kernel.py";
  if (prefix.empty() || !fs::exists(script)) {
    std::cerr << "pluss: the notebook kernel files are not in this download (share/pluss/notebook/install_kernel.py).\n";
    return 1;
  }
#if defined(_WIN32)
  const char* default_python = "python";
#else
  const char* default_python = "python3";
#endif
  const char* chosen = std::getenv("PLUSS_PYTHON");
  std::vector<std::string> command{chosen != nullptr && *chosen != '\0' ? chosen : default_python, script.string(), "--prefix", prefix.string()};
  command.insert(command.end(), args.begin() + 1, args.end());
  const auto outcome = run_program(command, false);
  if (outcome.code == 127 || outcome.code < 0) {
    std::cerr << "pluss: Python was not found. Install Python 3 (or set PLUSS_PYTHON to it) and run this again.\n";
    return 1;
  }
  return outcome.code;
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
  if (command == "kernel") return command_kernel(args);
  if (command == "doctor") return command_doctor();
  if (command == "version" || command == "--version") {
    std::cout << PLUSS_VERSION_STRING << " " << PLUSS_RELEASE_CHANNEL << "\n";
    return 0;
  }
  std::cerr << "pluss: unknown command '" << command << "'\n\n";
  usage();
  return 2;
}
