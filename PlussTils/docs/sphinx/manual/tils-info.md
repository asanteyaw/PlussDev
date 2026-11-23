# Version, build information and run seeds

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A result is only as useful as your ability to say what produced it. This chapter shows three small tools for that: the version of PlussII a program was built with, a description of the machine and build it is running on, and a way to give every run of a sweep its own seed that does not depend on which thread ran it.

## How it works

`pluss::version()` returns the version of this copy of PlussII. It is fixed when PlussII is built, so it describes the libraries your program was compiled against.

| Field | Meaning |
|---|---|
| `major`, `minor`, `patch` | The version number as three integers |
| `text` | The version as text, such as `0.0.6` |
| `channel` | `steady` for a tagged release, `transient` for the latest validated build, `development` for a build made from source |
| `build_type` | How the libraries were built, such as `Release` |
| `compiler` | The compiler that built them, with its version |

`pluss::tils::info::build()` adds what only the running machine knows: the operating system, the processor architecture, and the compute platforms that are usable here. `cpu` is always first; `mps` (the Apple GPU) and `cuda` follow when the machine has them. `describe()` writes the same facts as four readable lines, which is the form to put in a log or a bug report.

`pluss::tils::derive_seed(base, index)` is the seed of run `index` when the sweep starts from `base`. It uses no shared state: the same pair always gives the same seed, so the answer does not change with the number of threads or the order they take the runs in. The process-wide stream behind `pluss::tils::next_rng_seed()` is this function applied to the base seed and a running counter, so the two agree.

## Signatures

```cpp
pluss::Version pluss::version()
pluss::tils::info::BuildInfo pluss::tils::info::build()
std::string pluss::tils::info::describe(const pluss::tils::info::BuildInfo& info)
std::string pluss::tils::info::describe()
std::uint64_t pluss::tils::derive_seed(std::uint64_t base, std::uint64_t index)
```

`pluss::tils::info::build()` and `describe()` are in `<pluss/tils/info.h>`; `derive_seed` is in `<pluss/tils/kit/rng_seed.h>`.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run. The version, compiler and platform lines describe the machine the guide was written on; yours will differ.

## Example 1: What am I running?

The program prints the version fields, the operating system and the first platform, and then the four-line description.

```cpp
// Which PlussII is this, and what is it running on? Record it beside a result.
#include <pluss/pluss.h>
#include <pluss/tils/info.h>

#include <iostream>

int main() {
  const pluss::Version v = pluss::version();
  std::cout << "version " << v.major << "." << v.minor << "." << v.patch << " (" << v.text << ")\n";

  const auto info = pluss::tils::info::build();
  std::cout << "system " << info.system << " " << info.architecture << "\n";
  std::cout << "first platform " << info.platforms.front() << "\n\n";
  std::cout << pluss::tils::info::describe();
}
```

Output

```text
version 0.0.6 (0.0.6)
system macOS arm64
first platform cpu

PlussII 0.0.6 (transient, Release)
compiler:  AppleClang 21.0.0.21000401
system:    macOS arm64
platforms: cpu
```

The first line is the version split into its parts, the next two come from `build()`, and the last four lines are `describe()`. Write `describe()` beside a result and the result can be reproduced or compared later.

## Example 2: One seed per run

A sweep runs the same computation many times. Each run needs its own seed, and rerunning the sweep must give the same seeds even when the runs are shared among threads in a different order.

```cpp
// One seed for each run of a sweep, the same whichever thread runs it.
#include <pluss/tils/kit/rng_seed.h>

#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  using pluss::tils::derive_seed;
  const std::uint64_t base = 2024;

  std::cout << "run 0: " << derive_seed(base, 0) << "\n";
  std::cout << "run 1: " << derive_seed(base, 1) << "\n";
  std::cout << "run 1 again: " << derive_seed(base, 1) << "\n";

  // Four threads take the runs in a different order than a loop would. The seeds do not change.
  std::vector<std::uint64_t> in_order(8), by_threads(8);
  for (std::uint64_t i = 0; i < 8; ++i) in_order[i] = derive_seed(base, i);
  std::vector<std::thread> workers;
  for (std::uint64_t t = 0; t < 4; ++t) {
    workers.emplace_back([&, t] {
      for (std::uint64_t i = 8; i-- > 0;) {
        if (i % 4 == t) by_threads[i] = derive_seed(base, i);
      }
    });
  }
  for (auto& w : workers) w.join();
  std::cout << "same seeds on four threads: " << std::boolalpha << (in_order == by_threads) << "\n";

  // The process-wide stream is the same function with a running counter.
  pluss::tils::rng_seed(base);
  std::cout << "next_rng_seed() is run 1: " << (pluss::tils::next_rng_seed() == derive_seed(base, 1)) << "\n";
}
```

Output

```text
run 0: 11487996472437173461
run 1: 560689627191100215
run 1 again: 560689627191100215
same seeds on four threads: true
next_rng_seed() is run 1: true
```

Run 1 has the same seed every time it is asked for. The four threads took the eight runs in a different order from the loop, and every run still got the seed the loop gave it, so a sweep run on one thread and on many threads uses identical seeds. The last line shows that the shared stream is the same function: its first value is the seed of run 1.

## Decisions

- **Index the runs yourself.** Give run `i` the seed `derive_seed(base, i)`. Do not take seeds from `next_rng_seed()` inside parallel code: its values are the same set of seeds, but which run gets which depends on which thread asks first.
- **Record the base seed with the result.** The base seed and `describe()` are the two facts a rerun needs.
