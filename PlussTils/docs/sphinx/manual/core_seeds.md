# Deterministic Seeds: A Guide

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to Core Utilities](core.md) · [Seed API Reference](../api/core/seeds.md)

Many utilities need a random seed, and most callers do not want to pick one.
A factory that builds a sampler, a split, or a model component can take an
explicit seed, but when the caller leaves it out, the utility still has to
start somewhere. If it starts from a fixed constant, every run of every program
gets the same stream, which hides mistakes. If it starts from the clock, no
run can be repeated. The seed family sits between those extremes. It gives a
process one reproducible base, and it hands out a deterministic sequence of
derived seeds from that base, so that a whole program can be replayed by
resetting one value.

Read this guide when you need one of three things: a run you can reproduce
exactly, a set of independent random streams that do not collide, or an
explicit rule for when a seed is "not given". The examples below build those
three things up in order. Every program is complete, and every printed line is
copied from the program's real output.

## The mental model

The family holds two numbers. The first is the **base**, a 64-bit value that
identifies the sequence. The second is the **stream position**, a counter of
how many derived seeds have been handed out since the last reset.

Each call to `next_rng_seed()` increments the position and returns a value
mixed from the base and the new position. Two consequences follow, and the rest
of the guide depends on both:

- Reading the base does not move the stream. `rng_seed()` with no argument
  reports the base and nothing else. It does not tell you the next derived
  value, and it does not tell you how far the stream has moved.
- Resetting with `rng_seed(base)` sets the base and returns the position to
  zero. From that point, the same calls produce the same values in the same
  order.

The family does not sample random numbers for you. It allocates seeds. Each
derived seed is meant to start an independent generator, which then produces
its own sequence without returning to the shared state for every element.

## Example 1: The base is set, read, and separate from the stream

The first program sets a base, reads it back, and draws two derived seeds.
Read it for two points. Reading the base changes nothing. Drawing advances the
stream but leaves the base where it was.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  // Setting the base also resets the stream position to zero.
  pluss::tils::rng_seed(12345);

  // Reading the base reports it; it neither advances nor resets the stream.
  std::cout << "base: " << pluss::tils::rng_seed() << '\n';
  std::cout << "base again: " << pluss::tils::rng_seed() << '\n';

  // Two derived seeds advance the stream. The base is unchanged afterwards.
  const auto first = pluss::tils::next_rng_seed();
  const auto second = pluss::tils::next_rng_seed();
  std::cout << "derived distinct: " << std::boolalpha << (first != second) << '\n';
  std::cout << "base after draws: " << pluss::tils::rng_seed() << '\n';
}
```

Output

```text
base: 12345
base again: 12345
derived distinct: true
base after draws: 12345
```

Both reads of the base return `12345`, and the two derived seeds are distinct.
After the draws the base is still `12345`. A program that records the base as
run metadata gets a stable identifier, and the position is a separate fact that
the base does not reveal.

## Example 2: A reset replays the same ordered sequence

The second program reproduces one run inside another. It resets the base,
draws three seeds, resets again to the same base, and draws three more. The
three positions of the second run match the first run exactly. The last check
shows that a different base gives a different sequence at the first position.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  // A first run of the program: reset, then draw three values.
  pluss::tils::rng_seed(12345);
  const auto run_one_a = pluss::tils::next_rng_seed();
  const auto run_one_b = pluss::tils::next_rng_seed();
  const auto run_one_c = pluss::tils::next_rng_seed();

  // A second run of the same program: reset to the same base, draw again.
  pluss::tils::rng_seed(12345);
  const auto run_two_a = pluss::tils::next_rng_seed();
  const auto run_two_b = pluss::tils::next_rng_seed();
  const auto run_two_c = pluss::tils::next_rng_seed();

  std::cout << "position one replays: " << std::boolalpha << (run_one_a == run_two_a) << '\n';
  std::cout << "position two replays: " << (run_one_b == run_two_b) << '\n';
  std::cout << "position three replays: " << (run_one_c == run_two_c) << '\n';

  // Resetting to a different base produces a different sequence.
  pluss::tils::rng_seed(54321);
  std::cout << "different base differs at position one: "
            << (pluss::tils::next_rng_seed() != run_one_a) << '\n';
}
```

Output

```text
position one replays: true
position two replays: true
position three replays: true
different base differs at position one: true
```

This is the property that makes a program reproducible. The whole sequence of
derived seeds is determined by the base, so reproducing a run means restoring
the base at its start and then making the same calls in the same order. The
final line confirms that the base is the identity of the sequence. Change it,
and the first value changes too.

## Example 3: Zero is a valid base

A base of zero is a legitimate sequence, not a sentinel for "unseeded". The
third program sets the base to zero, checks that the base reads back as zero,
and draws a derived seed. The derived seed is not zero, and it replays after a
reset.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  // Zero is a legitimate base. It is not a request for an implicit seed.
  pluss::tils::rng_seed(0);
  std::cout << "base is zero: " << (pluss::tils::rng_seed() == 0) << '\n';

  // The base still produces a stream of derived seeds, and that stream replays.
  pluss::tils::rng_seed(0);
  const auto first = pluss::tils::next_rng_seed();
  pluss::tils::rng_seed(0);
  std::cout << "zero base replays: " << (pluss::tils::next_rng_seed() == first) << '\n';
  std::cout << "zero base first derived is nonzero: " << (first != 0) << '\n';
}
```

Output

```text
base is zero: true
zero base replays: true
zero base first derived is nonzero: true
```

Keep this distinction in mind, because the next example introduces a function
that does use zero as a request. Passing zero as a *base* gives you a real
sequence. Passing zero as a *requested seed* asks for a derived one. The two
meanings never mix, because the base is set with `rng_seed` and the request is
made with `seed_or_next`.

## Example 4: An explicit seed is kept, and zero asks for the next one

The fourth program separates the two uses of a seed argument. A nonzero value
is treated as the caller's own decision and is returned unchanged, and the
stream does not move. Zero means the caller has no seed to offer, so the
function allocates the next derived one, and that consumes a position.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  // A nonzero explicit seed is used as given. The stream does not move.
  pluss::tils::rng_seed(700);
  const auto expected_next = pluss::tils::next_rng_seed();

  pluss::tils::rng_seed(700);
  std::cout << "explicit seed kept: " << pluss::tils::seed_or_next(99) << '\n';
  std::cout << "stream untouched by explicit: "
            << (pluss::tils::next_rng_seed() == expected_next) << '\n';

  // Zero asks for the next derived seed, so it consumes one stream position.
  pluss::tils::rng_seed(800);
  const auto expected_zero = pluss::tils::next_rng_seed();
  pluss::tils::rng_seed(800);
  std::cout << "zero requests next: " << (pluss::tils::seed_or_next(0) == expected_zero) << '\n';
}
```

Output

```text
explicit seed kept: 99
stream untouched by explicit: true
zero requests next: true
```

This is the rule a factory should follow for its own optional seed. Let zero
mean "not given". If the caller passes a value, the factory uses it, and the
shared stream stays where it was, so the caller's choice does not shift the
seeds handed to everything else. If the caller passes zero, the factory
allocates from the stream, so reproducibility still follows from the base.

## Example 5: Independent engines from consecutive seeds

A typical use gives each generator its own derived seed. The fifth program
builds two engines from two consecutive allocations and then rebuilds them
after a reset. Each engine replays its own first output. Advancing one engine
by a thousand draws leaves the other exactly where it was, because each engine
owns its state.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <cstdint>
#include <iostream>
#include <random>

int main() {
  std::cout << std::boolalpha;

  // Build two engines from two consecutive derived seeds, and record their
  // first outputs.
  pluss::tils::rng_seed(12345);
  std::mt19937_64 left(pluss::tils::next_rng_seed());
  std::mt19937_64 right(pluss::tils::next_rng_seed());
  const std::uint64_t left_first = left();
  const std::uint64_t right_first = right();
  std::cout << "engines start differently: " << (left_first != right_first) << '\n';

  // Reset the stream and build the same pair again. Each engine replays
  // its own first output.
  pluss::tils::rng_seed(12345);
  std::mt19937_64 left_again(pluss::tils::next_rng_seed());
  std::mt19937_64 right_again(pluss::tils::next_rng_seed());
  std::cout << "left replays: " << (left_again() == left_first) << '\n';
  std::cout << "right replays: " << (right_again() == right_first) << '\n';

  // Advancing one engine does not move the other. Advance left_again by a
  // thousand draws, then compare the next outputs of the two right engines.
  for (int i = 0; i < 1000; ++i) (void)left_again();
  std::cout << "right unaffected by left: " << (right() == right_again()) << '\n';
}
```

Output

```text
engines start differently: true
left replays: true
right replays: true
right unaffected by left: true
```

The engines start differently because they were seeded with different derived
values. They do not share state, because the family hands out values and does
not hold generator state. The family never produces random numbers itself. It
only produces the seeds that start each generator.

## Example 6: Concurrent allocation and reproducible sets

Workers that allocate seeds in parallel receive distinct values, because
allocation is serialized. The thread that receives a particular value depends
on scheduling, and this is the one thing that varies from run to run. The sixth
program therefore checks two properties that do not depend on scheduling. All
allocated seeds are distinct, and a replay after a reset produces the same
sorted set.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

namespace {

// Allocate one derived seed per worker. Which worker gets which seed depends on
// scheduling, so the program compares the sorted set, not the assignment.
std::vector<std::uint64_t> allocate_concurrently(std::size_t count) {
  std::vector<std::uint64_t> seeds(count);
  std::vector<std::thread> workers;
  workers.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    workers.emplace_back([&seeds, i] { seeds[i] = pluss::tils::next_rng_seed(); });
  }
  for (auto& worker : workers) worker.join();
  std::sort(seeds.begin(), seeds.end());
  return seeds;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  constexpr std::size_t workers = 32;

  pluss::tils::rng_seed(900);
  const auto first_run = allocate_concurrently(workers);
  std::cout << "allocated: " << first_run.size() << '\n';
  std::cout << "all distinct: "
            << (std::adjacent_find(first_run.begin(), first_run.end()) == first_run.end()) << '\n';

  // Reset to the same base and run again. The set of derived seeds is the same,
  // even though the thread that received each seed may differ.
  pluss::tils::rng_seed(900);
  const auto second_run = allocate_concurrently(workers);
  std::cout << "same set on replay: " << (first_run == second_run) << '\n';
}
```

Output

```text
allocated: 32
all distinct: true
same set on replay: true
```

If your results depend on which worker received which seed, allocate the seeds
on one thread before launching the workers, and pass each worker its own value.
The set is reproducible either way. The assignment is reproducible only when
you assign it explicitly.

## Example 7: The reset you forget

Most seed bugs in practice are not about the family's rules. They are about a
reset that is missing. The seventh program shows the failure. A second run in
the same process draws from a stream that has already moved, so its value
differs even though the code looks identical. Resetting before the second run
restores the match.

```cpp
#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;

  // Run one: reset, then draw.
  pluss::tils::rng_seed(12345);
  const auto run_one = pluss::tils::next_rng_seed();

  // Run two without a reset. The stream has already moved, so this run
  // draws a different value even though the code looks identical.
  const auto run_two_forgotten_reset = pluss::tils::next_rng_seed();
  std::cout << "forgotten reset matches run one: " << (run_two_forgotten_reset == run_one) << '\n';

  // Run two done correctly: reset first.
  pluss::tils::rng_seed(12345);
  const auto run_two = pluss::tils::next_rng_seed();
  std::cout << "reset before run two matches run one: " << (run_two == run_one) << '\n';
}
```

Output

```text
forgotten reset matches run one: false
reset before run two matches run one: true
```

The fix is always the same: call `rng_seed` once at the start of every unit of
work that must be reproducible. For a test, that is the start of the test. For
an experiment, it is the start of the run. For a process that performs several
independent experiments, it is the start of each one.

## Decisions

Use this table to choose among the three operations.

| You want to | Use | Why |
|---|---|---|
| Fix the base at the start of a reproducible unit of work | `rng_seed(base)` | Resets the position, so the work replays |
| Record which sequence a run used | `rng_seed()` | Reads the base without changing anything |
| Give a component its own seed, if the caller gave none | `seed_or_next(seed)` with a zero default | Keeps a caller's nonzero seed; allocates otherwise |
| Get a fresh derived seed you will hand to a generator | `next_rng_seed()` | Advances the stream by one |

Two decisions matter more than the rest. First, reset at the boundary of a
reproducible unit, not inside it. A reset in the middle of a run makes the
second half depend on where the reset landed. Second, when a factory takes an
optional seed, route it through `seed_or_next`, so that callers who care about
reproducibility pass their own seed, and callers who do not still get a
reproducible allocation from the base.

## Pitfalls

**Assuming a second run starts fresh.** Example 7 shows the failure. The
stream position is process-wide. Any allocation, from any part of the program,
moves it, so a run that does not reset first depends on everything that ran
before it.

**Reading the base to learn the next value.** Example 1 shows that the getter
does not reveal the next derived seed or the position. Record the base as
metadata, and do not treat it as a checkpoint of progress.

**Treating zero as "unseeded" for the base.** Example 3 shows that a zero base
is a real, replayable sequence. Use `seed_or_next` for "the caller gave no
seed", and use `rng_seed` for "this run's base is zero".

**Expecting a reproducible assignment from parallel workers.** Example 6 shows
that the set is reproducible and the thread assignment is not. Allocate on one
thread when assignment must be reproducible.

**Expecting the family to seed engines you did not create through it.** The
family controls only the facilities that consult it. An engine you construct
yourself with a literal seed is not affected by `rng_seed`.

## Where next

- [`rng_seed`](../api/core-seed/rng_seed.md) -- the setter and the base getter,
  with every overload.
- [`next_rng_seed`](../api/core-seed/next_rng_seed.md) -- the derived-seed
  allocator and its ordering.
- [`seed_or_next`](../api/core-seed/seed_or_next.md) -- the explicit-or-derived
  rule for factory arguments.
- [Core Utilities Manual](core.md) -- the other decision groups in the core
  shelf.
