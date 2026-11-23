#include <pluss/tils/settings.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "core settings test failed: " << message << '\n';
    ++failures;
  }
}

template <class F>
bool throws(F&& body) {
  try {
    body();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

std::string write_file(const std::string& name, const std::string& text) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::ofstream out(path, std::ios::binary);
  out << text;
  return path.string();
}

void set_env(const char* name, const char* value) {
#if defined(_WIN32)
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif
}

void clear_env(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

pluss::tils::Settings defaults() {
  pluss::tils::Settings s;
  s.set("port", 8080).set("debug", false).set("name", "run").set("rate", 0.5).set("region", "us-east-1");
  return s;
}

}  // namespace

int main() {
  using pluss::tils::Layer;
  using pluss::tils::Settings;

  // Defaults, typed reads and origins.
  auto s = defaults();
  require(s.get<int>("port") == 8080, "an integer default reads back");
  require(s.get<bool>("debug") == false, "a bool default reads back");
  require(s.get<std::string>("name") == "run", "a string default reads back");
  require(s.get<double>("rate") == 0.5, "a floating-point default reads back exactly");
  require(s.origin("port").layer == Layer::Default, "a default says it is a default");
  require(s.has("port") && !s.has("nope"), "has() tells declared keys from others");
  require(s.keys().size() == 5, "keys() lists every declared key");

  // A file overrides the defaults; a nested object is read as dotted keys.
  const auto path = write_file("pluss_settings_a.json",
                               R"({"port": 9000, "name": "from \"file\"", "debug": true, "db": {"host": "h1", "pool": 4}})");
  Settings f = defaults();
  f.set("db.host", "local").set("db.pool", 1);
  f.file(path);
  require(f.get<int>("port") == 9000, "a file overrides a default");
  require(f.get<std::string>("name") == "from \"file\"", "a file string with escapes is read");
  require(f.get<bool>("debug") == true, "a file boolean is read");
  require(f.get<std::string>("db.host") == "h1" && f.get<int>("db.pool") == 4, "nested objects become dotted keys");
  require(f.origin("port").layer == Layer::File && f.origin("port").source == path, "a file value names its file");
  require(f.get<std::string>("region") == "us-east-1", "a key the file does not name keeps its default");

  // The environment overrides the file.
  set_env("RUN_PORT", "9100");
  set_env("RUN_DEBUG", "off");
  f.env("RUN");
  require(f.get<int>("port") == 9100, "the environment overrides the file");
  require(f.get<bool>("debug") == false, "the environment word 'off' reads as false");
  require(f.origin("port").layer == Layer::Env && f.origin("port").source == "RUN_PORT", "an environment value names its variable");
  require(f.get<std::string>("db.host") == "h1", "a key the environment does not name keeps the file's value");
  set_env("RUN_DB_HOST", "from-env");
  f.env("RUN");
  require(f.get<std::string>("db.host") == "from-env", "a dotted key reads PREFIX_A_B");

  // The command line overrides everything.
  const char* argv[] = {"prog", "--port=9200", "--name", "cli", "--debug", "input.csv", "--no-debug", "--rate", "0.25", "--", "--raw"};
  Settings a = defaults();
  a.args(11, argv);
  require(a.get<int>("port") == 9200, "--key=value sets a value");
  require(a.get<std::string>("name") == "cli", "--key value sets a value");
  require(a.get<bool>("debug") == false, "--no-flag after --flag gives false");
  require(a.get<double>("rate") == 0.25, "a number argument is read");
  require(a.positional().size() == 2 && a.positional()[0] == "input.csv" && a.positional()[1] == "--raw",
          "words that are not options are positional, and '--' ends the options");
  require(a.origin("port").layer == Layer::Args && a.origin("port").source == "--port=9200", "an argument value names the argument");

  const char* flag_argv[] = {"prog", "--debug", "data.csv"};
  Settings g = defaults();
  g.args(3, flag_argv);
  require(g.get<bool>("debug") == true, "a bare flag is true");
  require(g.positional().size() == 1 && g.positional()[0] == "data.csv", "a flag does not swallow the next word");

  // All four layers: arguments win.
  Settings all = defaults();
  all.file(write_file("pluss_settings_b.json", R"({"port": 1})"));
  set_env("ALL_PORT", "2");
  all.env("ALL");
  const char* port_argv[] = {"prog", "--port=3"};
  all.args(2, port_argv);
  require(all.get<int>("port") == 3, "arguments beat the environment, the file and the default");
  clear_env("ALL_PORT");

  // Strict mode: an unknown key from a file or an argument is an error.
  Settings strict = defaults();
  require(throws([&] { strict.file(write_file("pluss_settings_c.json", R"({"prot": 1})")); }), "an unknown file key is an error");
  const char* typo_argv[] = {"prog", "--prot=1"};
  require(throws([&] { strict.args(2, typo_argv); }), "an unknown argument is an error");
  require(throws([&] { (void)strict.get<int>("prot"); }), "reading an unknown key is an error");
  Settings lenient = defaults();
  lenient.strict(false).file(write_file("pluss_settings_c.json", R"({"prot": 1})"));
  require(lenient.get<int>("prot") == 1, "lenient mode accepts a key the defaults did not declare");

  // Typed reads fail loudly, naming the key and where the value came from.
  Settings bad = defaults();
  bad.file(write_file("pluss_settings_d.json", R"({"port": "many", "debug": "maybe", "rate": "fast"})"));
  require(throws([&] { (void)bad.get<int>("port"); }), "a non-number is not an integer");
  require(throws([&] { (void)bad.get<bool>("debug"); }), "an unknown word is not a bool");
  require(throws([&] { (void)bad.get<double>("rate"); }), "a word is not a number");
  try {
    (void)bad.get<int>("port");
  } catch (const std::exception& error) {
    const std::string text = error.what();
    require(text.find("port") != std::string::npos && text.find("many") != std::string::npos &&
                text.find("pluss_settings_d.json") != std::string::npos,
            "the error names the key, the value and the file");
  }

  // Bad files.
  require(throws([&] { defaults().file(write_file("pluss_settings_e.json", R"({"port": [1, 2]})")); }), "an array is an error");
  require(throws([&] { defaults().file(write_file("pluss_settings_f.json", R"({"port": null})")); }), "null is an error");
  require(throws([&] { defaults().file(write_file("pluss_settings_g.json", R"({"port": 1)")); }), "an unclosed object is an error");
  require(throws([&] { defaults().file("no/such/file.json"); }), "a missing file is an error");

  clear_env("RUN_PORT");
  clear_env("RUN_DEBUG");
  clear_env("RUN_DB_HOST");
  if (failures == 0) std::cout << "core settings tests passed\n";
  return failures == 0 ? 0 : 1;
}
