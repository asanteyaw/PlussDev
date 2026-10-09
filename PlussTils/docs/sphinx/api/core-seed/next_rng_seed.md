# `pluss::tils::next_rng_seed`

[Back to the Deterministic Seeds API Reference](../core/seeds.md)

```cpp
std::uint64_t pluss::tils::next_rng_seed()
```

`next_rng_seed()` allocates one deterministic 64-bit seed from the current
process-wide stream. Each call advances the stream by one position and mixes
the base with that position, so neighboring allocations are not merely small
integer increments. The result is suitable for constructing independent
random engines or supplying explicit seeds to separate operations.

The operation does not sample a distribution and does not retain an engine.
Its cost is constant and independent of the amount of data a later operation
will generate. Shared-state access is serialized only while a stream position
is reserved; the integer mixing happens after that short critical section.

Concurrent callers receive distinct positions. Their result collection is
safe, but scheduler order decides which caller receives which seed. If task A
must always receive the first derived seed and task B the second, allocate both
on the coordinating thread and pass them into the tasks explicitly.

After the 64-bit stream position wraps, the sequence repeats. Reaching that
boundary requires exhausting the complete unsigned 64-bit position space and
is not a practical application limit, but it defines the arithmetic contract.

## Example 1: Allocate successive seeds

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  pluss::tils::rng_seed(12345);
  std::cout << "first: " << pluss::tils::next_rng_seed() << '\n';
  std::cout << "second: " << pluss::tils::next_rng_seed() << '\n';
}
```

Output:

```text
first: 14663218685290508263
second: 5886303152054975053
```

## Example 2: Initialize independent engines

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>
#include <random>

int main() {
  pluss::tils::rng_seed(12345);
  std::mt19937_64 left(pluss::tils::next_rng_seed());
  std::mt19937_64 right(pluss::tils::next_rng_seed());

  std::cout << "left differs from right: " << std::boolalpha
            << (left() != right()) << '\n';
}
```

Output:

```text
left differs from right: true
```

The example tests the intended separation property rather than publishing the
standard engine's numeric output, which can distract from the seed contract.

## Example 3: Allocate from several threads

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <algorithm>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  constexpr std::size_t count = 32;
  pluss::tils::rng_seed(900);

  std::vector<std::uint64_t> seeds(count);
  std::vector<std::thread> workers;
  for (std::size_t i = 0; i < count; ++i) {
    workers.emplace_back([&, i] {
      seeds[i] = pluss::tils::next_rng_seed();
    });
  }
  for (auto& worker : workers) worker.join();

  std::sort(seeds.begin(), seeds.end());
  const bool unique =
      std::adjacent_find(seeds.begin(), seeds.end()) == seeds.end();
  std::cout << "concurrent count: " << seeds.size() << '\n';
  std::cout << "concurrent unique: " << std::boolalpha << unique << '\n';
}
```

Output:

```text
concurrent count: 32
concurrent unique: true
```
