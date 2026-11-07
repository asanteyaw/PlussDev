# `pluss::tils::seed_or_next`

[Back to the Deterministic Seeds API Reference](../core/seeds.md)

```cpp
std::uint64_t pluss::tils::seed_or_next(std::uint64_t seed)
```

`seed_or_next()` implements the seed convention used by convenience APIs that
accept an optional seed without introducing an optional wrapper: a nonzero
argument is an explicit seed and is returned unchanged; zero asks for the next
derived seed from the shared stream.

The two branches differ in shared-state behavior. An explicit seed does not
read, reset, or advance the stream. The zero branch is exactly one call to
`next_rng_seed()` and consumes one position. This lets an operation offer both
local reproducibility and coordinated default seeding without duplicating the
policy at every call site.

Because zero is the sentinel, this helper cannot express “use literal seed
zero.” Use a nonzero explicit seed, or construct and seed the target engine
directly when literal zero has domain significance. Setting the global base to
zero remains valid: `seed_or_next(0)` then allocates from the sequence whose
base is zero.

## Example 1: Preserve an explicit seed

```cpp
#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  std::cout << "explicit seed: " << pluss::tils::seed_or_next(99) << '\n';
}
```

Output:

```text
explicit seed: 99
```

## Example 2: An explicit seed does not consume the stream

```cpp
#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  pluss::tils::rng_seed(700);
  const auto expected = pluss::tils::next_rng_seed();

  pluss::tils::rng_seed(700);
  (void)pluss::tils::seed_or_next(99);
  std::cout << std::boolalpha
            << "explicit preserves stream: "
            << (pluss::tils::next_rng_seed() == expected) << '\n';
}
```

Output:

```text
explicit preserves stream: true
```

## Example 3: Zero consumes the next position

```cpp
#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  pluss::tils::rng_seed(800);
  const auto expected = pluss::tils::next_rng_seed();

  pluss::tils::rng_seed(800);
  std::cout << std::boolalpha
            << "zero requests next: "
            << (pluss::tils::seed_or_next(0) == expected) << '\n';
}
```

Output:

```text
zero requests next: true
```
