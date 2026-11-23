# Settings from a file, the environment and the command line

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md) · [Reading the environment](tils-env.md)

A program that is run many times needs its parameters from outside: a tolerance, an iteration limit, an output name. `pluss::tils::Settings` gathers them from four places in a fixed order of priority and reads them back as typed values, so the same program runs unchanged on a laptop, in a batch job and in a test.

## How it works

A setting has a name and a value, and the value can come from four layers. When more than one layer sets the same name, the layer later in the list wins.

| Layer | Set with | Written as |
|---|---|---|
| Default | `set(key, value)` | the value in code |
| File | `file(path)` | a JSON object: `{"max_iter": 500, "db": {"host": "h"}}`; a nested object becomes dotted names such as `db.host` |
| Environment | `env(prefix)` | `PREFIX_KEY`: with prefix `RUN`, the key `max_iter` reads `RUN_MAX_ITER`; letters are upper-cased and anything that is not a letter or digit becomes `_` |
| Arguments | `args(argc, argv)` | `--key=value`, `--key value`, `--flag` for true, `--no-flag` for false |

The defaults declare the keys, so set every default before applying another layer. By default `Settings` is strict: a file or an argument that names a key that was not declared is an error that lists the keys that exist and suggests the closest. `strict(false)` accepts new keys instead. Arguments that are not options are kept in order and returned by `positional()`; `--` ends the options.

A flag does not take the next word as its value when the setting's default is `true` or `false`, so `--verbose data.csv` leaves `data.csv` positional.

Values are read with `get<T>(key)` for `bool`, any integer type, any floating-point type and `std::string`. A value that is not the type asked for is an error that names the key, the value and where it came from. A boolean may be written `true`, `false`, `yes`, `no`, `on`, `off`, `1` or `0`.

`origin(key)` says which layer a value came from and, for a file, an environment variable or an argument, the file, variable or argument itself.

## Signatures

```cpp
pluss::tils::Settings& pluss::tils::Settings::set(std::string_view key, std::string_view value)
pluss::tils::Settings& pluss::tils::Settings::set(std::string_view key, bool value)
pluss::tils::Settings& pluss::tils::Settings::set(std::string_view key, T value)
pluss::tils::Settings& pluss::tils::Settings::file(const std::filesystem::path& path)
pluss::tils::Settings& pluss::tils::Settings::env(std::string_view prefix)
pluss::tils::Settings& pluss::tils::Settings::args(int argc, const char* const* argv)
pluss::tils::Settings& pluss::tils::Settings::strict(bool on = true)
bool pluss::tils::Settings::has(std::string_view key) const
T pluss::tils::Settings::get<T>(std::string_view key) const
pluss::tils::Origin pluss::tils::Settings::origin(std::string_view key) const
std::vector<std::string> pluss::tils::Settings::keys() const
const std::vector<std::string>& pluss::tils::Settings::positional() const
```

`set` with a number takes any arithmetic type. The header is `<pluss/tils/settings.h>`. An `Origin` has a `layer` (`Layer::Default`, `File`, `Env` or `Args`) and a `source` text.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Four layers

The program writes a settings file, sets one environment variable, and applies the layers in order. The command line is built in code so the example is the same everywhere.

```cpp
// Four layers of settings: defaults, a file, the environment, the command line.
#include <pluss/tils/settings.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace tils = pluss::tils;

const char* layer_name(tils::Layer layer) {
  switch (layer) {
    case tils::Layer::Default: return "default";
    case tils::Layer::File: return "file";
    case tils::Layer::Env: return "environment";
    case tils::Layer::Args: return "arguments";
  }
  return "?";
}

void show(const tils::Settings& s, const char* key) {
  std::cout << key << " = " << s.get<std::string>(key) << "   (" << layer_name(s.origin(key).layer) << ")\n";
}

int main() {
  // A settings file, written here so the example is complete.
  const auto path = std::filesystem::temp_directory_path() / "run.json";
  std::ofstream(path) << R"({"max_iter": 500, "tolerance": 1e-8, "db": {"host": "files-host"}})";

  setenv("RUN_MAX_ITER", "2000", 1);

  tils::Settings s;
  s.set("max_iter", 100)        // defaults declare the keys
      .set("tolerance", 1e-6)
      .set("verbose", false)
      .set("name", "baseline")
      .set("db.host", "localhost");
  s.file(path).env("RUN");

  const char* argv[] = {"program", "--name=trial", "--verbose", "data.csv"};
  s.args(4, argv);

  for (const char* key : {"max_iter", "tolerance", "verbose", "name", "db.host"}) show(s, key);

  std::cout << "\nmax_iter as an int: " << s.get<int>("max_iter") + 1 << "\n";
  std::cout << "verbose as a bool: " << std::boolalpha << s.get<bool>("verbose") << "\n";
  std::cout << "positional: " << s.positional().front() << "\n";
}
```

Output

```text
max_iter = 2000   (environment)
tolerance = 1e-8   (file)
verbose = true   (arguments)
name = trial   (arguments)
db.host = files-host   (file)

max_iter as an int: 2001
verbose as a bool: true
positional: data.csv
```

Each line names the layer that won. `max_iter` has a default of 100, the file says 500, the environment says 2000, and the environment wins because it comes after the file. `tolerance` comes from the file because nothing later sets it. `verbose` and `name` come from the command line. `db.host` is read from the nested object in the file. `--verbose` did not take `data.csv`, which is why it is the positional word.

## Example 2: Mistakes

A misspelt argument and a value of the wrong type.

```cpp
// Mistakes are errors that say what is wrong.
#include <pluss/tils/settings.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace tils = pluss::tils;

int main() {
  tils::Settings s;
  s.set("max_iter", 100).set("tolerance", 1e-6);

  // A misspelt key in an argument.
  const char* typo[] = {"program", "--max_itr=5"};
  try {
    s.args(2, typo);
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\n\n";
  }

  // A value that is not the type that was asked for.
  const std::filesystem::path path = "bad.json";
  std::ofstream(path) << R"({"max_iter": "many"})";
  s.file(path);
  try {
    std::cout << s.get<int>("max_iter") << "\n";
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\n";
  }
}
```

Output

```text
Settings: unknown key 'max_itr' (declared keys: max_iter, tolerance) Did you mean 'max_iter'?

Settings: 'max_iter' is 'many' (from bad.json), which is not a whole number
```

The first error lists the declared keys and suggests the one that was meant. The second names the key, the value, the file it came from and what was expected.

## Pitfalls

- **Declare before you apply.** In strict mode a key that has no default is unknown to a file or an argument. Set all defaults first.
- **The environment layer only looks for declared keys.** It reads `PREFIX_KEY` for each key it knows, so there is no unknown-key error from the environment.
- **A file value is one value.** Arrays and `null` in a settings file are errors; a setting is one value.
