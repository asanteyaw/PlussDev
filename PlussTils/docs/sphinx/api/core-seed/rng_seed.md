# `pluss::tils::rng_seed`

[Back to the Deterministic Seeds API Reference](../core/seeds.md)

```cpp
void pluss::tils::rng_seed(std::uint64_t seed)

std::uint64_t pluss::tils::rng_seed()
```

`rng_seed(seed)` establishes the process-wide base for later implicit seed
allocation. It also resets the stream position to its beginning. Repeating the
same reset therefore makes subsequent calls to `next_rng_seed()` replay the
same ordered sequence. Use it at a program, test, training-run, or experiment
boundary where all later default-seeded utilities should be reproducible.

The zero-argument overload reports the configured base. Reading it does not
advance or reset the stream, and it does not reveal the next derived value.
This distinction matters when recording run metadata: the base identifies the
sequence, while the stream position depends on how many implicit allocations
the program has already made.

The base is an unsigned 64-bit value, including zero. A zero base remains a
valid sequence base. It is different from passing zero to `seed_or_next()`,
where zero is deliberately the request for an implicit derived seed.

Reset and allocation are thread-safe and form one coherent state transition.
Code should still seed before launching parallel work when exact assignment of
derived seeds to individual tasks matters. The operation controls only Pluss
facilities that consult this seed family; it does not reseed unrelated C++
engines or third-party generators.

## Example 1: Set and inspect the base

```cpp
#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  pluss::tils::rng_seed(12345);
  std::cout << "base: " << pluss::tils::rng_seed() << '\n';

  (void)pluss::tils::next_rng_seed();
  (void)pluss::tils::next_rng_seed();
  std::cout << "base after draws: " << pluss::tils::rng_seed() << '\n';
}
```

Output:

```text
base: 12345
base after draws: 12345
```

## Example 2: Reset and replay

```cpp
#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  pluss::tils::rng_seed(12345);
  const auto first = pluss::tils::next_rng_seed();
  const auto second = pluss::tils::next_rng_seed();

  pluss::tils::rng_seed(12345);
  std::cout << std::boolalpha;
  std::cout << "reset replays first: "
            << (pluss::tils::next_rng_seed() == first) << '\n';
  std::cout << "reset replays second: "
            << (pluss::tils::next_rng_seed() == second) << '\n';
}
```

Output:

```text
reset replays first: true
reset replays second: true
```

## Example 3: Zero is a valid base

```cpp
#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  pluss::tils::rng_seed(0);
  const auto first = pluss::tils::next_rng_seed();

  pluss::tils::rng_seed(0);
  std::cout << "zero base: " << pluss::tils::rng_seed() << '\n';
  std::cout << std::boolalpha
            << "zero base replays: "
            << (pluss::tils::next_rng_seed() == first) << '\n';
}
```

Output:

```text
zero base: 0
zero base replays: true
```
