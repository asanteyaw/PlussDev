# Keeping results between runs

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

Some steps are slow and give the same answer for the same inputs: fitting a model, preparing a large table, running a simulation. `pluss::tils::Cache` saves such a result in a folder and returns it the next time the same inputs come round, so the step runs once.

## How it works

An entry is found by a key and a list of parts. The parts are the inputs the result depends on, written as text: a setting, a file's modification time, a version of the data. Change any part, or the key, and the entry is a different one. The version of PlussII is part of every entry, so a new release never reads what an older one saved.

`get<T>(key, parts, make)` returns the saved value if there is one that reads back as `T`. Otherwise it calls `make()`, saves the result and returns it. Values are saved with `pluss::io`, so any type it supports works: `Series`, `Note`, `Timeline`, `TimeIndex` and the standard containers of them. A table is not supported yet; save its columns.

An entry is written to a temporary file and renamed, so a reader never sees half an entry, and two programs computing the same entry at once are safe: the last to finish wins, and both computed the same inputs. A file that cannot be read back as the requested type is not an error; it is replaced by a fresh result, so a cache cannot make a program fail.

| Member | What it does |
|---|---|
| `Cache(dir)` | uses the folder `dir`, created when the first entry is saved |
| `Cache::default_dir(app)` | the platform's per-user cache folder for an application |
| `get<T>(key, parts, make)` | the saved value, or `make()` saved for next time |
| `find<T>(key, parts)` | the saved value if there is one, with no computing |
| `put(key, parts, value)` | saves a value |
| `has(key, parts)`, `erase(key, parts)` | whether an entry exists, and removes one |
| `clear()` | removes every entry and returns how many |
| `stats()` | entries and bytes in the folder, and the hits and misses of this object |

## Signatures

```cpp
pluss::tils::Cache::Cache(std::filesystem::path dir)
std::filesystem::path pluss::tils::Cache::default_dir(std::string_view app = "pluss")
T pluss::tils::Cache::get<T>(std::string_view key, std::initializer_list<std::string> parts, Make&& make)
T pluss::tils::Cache::get<T>(std::string_view key, const std::vector<std::string>& parts, Make&& make)
std::optional<T> pluss::tils::Cache::find<T>(std::string_view key, const std::vector<std::string>& parts) const
void pluss::tils::Cache::put(std::string_view key, const std::vector<std::string>& parts, const T& value)
bool pluss::tils::Cache::has(std::string_view key, const std::vector<std::string>& parts) const
bool pluss::tils::Cache::erase(std::string_view key, const std::vector<std::string>& parts)
std::size_t pluss::tils::Cache::clear()
pluss::tils::Cache::Stats pluss::tils::Cache::stats() const
```

The header is `<pluss/tils/cache.h>`. `Stats` has `entries`, `bytes`, `hits` and `misses`.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Compute once

The program fits a result for two values of `alpha` and asks for the first one twice. A counter stands for the work, so the output shows how often it ran.

```cpp
// Run an expensive step once for a given set of inputs.
#include <pluss/pluss.h>
#include <pluss/tils/cache.h>

#include <filesystem>
#include <iostream>

int main() {
  const auto dir = std::filesystem::temp_directory_path() / "cache_guide";
  std::filesystem::remove_all(dir);
  pluss::tils::Cache cache(dir);

  int fits = 0;
  const auto fit = [&](const std::string& alpha) {
    return cache.get<pluss::Series>("scores", {"alpha=" + alpha, "rows=3"}, [&] {
      ++fits;  // stands for work that takes a while
      const double a = std::stod(alpha);
      return pluss::make_series({a, 2 * a, 3 * a});
    });
  };

  std::cout << "alpha 0.1: " << fit("0.1").host<double>()[2] << "\n";
  std::cout << "alpha 0.1 again: " << fit("0.1").host<double>()[2] << "\n";
  std::cout << "alpha 0.2: " << fit("0.2").host<double>()[2] << "\n";
  std::cout << "fits computed: " << fits << " of 3 calls\n";

  const auto stats = cache.stats();
  std::cout << "entries " << stats.entries << ", hits " << stats.hits << ", misses " << stats.misses << "\n";

  std::cout << "erased: " << std::boolalpha << cache.erase("scores", {"alpha=0.1", "rows=3"}) << "\n";
  std::cout << "cleared " << cache.clear() << " entry\n";
  std::filesystem::remove_all(dir);
}
```

Output

```text
alpha 0.1: 0.3
alpha 0.1 again: 0.3
alpha 0.2: 0.6
fits computed: 2 of 3 calls
entries 2, hits 1, misses 2
erased: true
cleared 1 entry
```

Three calls, two computations: the second call for alpha 0.1 was answered from the folder. The statistics agree, with one hit and two misses, and two entries on disk. `erase` removed one entry and `clear` removed the other.

## Pitfalls

The cache does not look at your data. It trusts the key and the parts. A part left out is an input the cache cannot see.

```cpp
// A part left out of the key: the cache cannot know the inputs changed.
#include <pluss/pluss.h>
#include <pluss/tils/cache.h>

#include <filesystem>
#include <iostream>

int main() {
  const auto dir = std::filesystem::temp_directory_path() / "cache_pitfall";
  std::filesystem::remove_all(dir);
  pluss::tils::Cache cache(dir);

  const auto total = [&](const std::vector<double>& values, const std::vector<std::string>& parts) {
    return cache.get<pluss::Series>("total", parts, [&] { return pluss::make_series({values[0] + values[1]}); }).host<double>()[0];
  };

  std::cout << "inputs 1, 2, key without the inputs:  " << total({1, 2}, {}) << "\n";
  std::cout << "inputs 10, 20, same key:              " << total({10, 20}, {}) << "   <- the old answer\n";
  std::cout << "inputs 10, 20, key names the inputs:  " << total({10, 20}, {"a=10", "b=20"}) << "\n";
  std::filesystem::remove_all(dir);
}
```

Output

```text
inputs 1, 2, key without the inputs:  3
inputs 10, 20, same key:              3   <- the old answer
inputs 10, 20, key names the inputs:  30
```

With no parts in the key, new inputs got the old answer. Putting the inputs in the key made a different entry and the right answer. Name everything the result depends on, and use a stamp (a file's modification time, a data version) for inputs too large to write out.
