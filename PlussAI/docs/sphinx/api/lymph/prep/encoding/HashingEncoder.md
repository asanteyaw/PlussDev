# `lymph::prep::HashingEncoder`

`HashingEncoder` is the standalone, (almost) stateless categorical encoder of
the `prep` family: it maps each category into one of `n_components` fixed buckets
with a multiplicative hash and emits an `n_components`-wide indicator. The output
width is independent of how many categories appear, and **no vocabulary is
stored**. It is the object twin of
[`prep::stage::hashing_encoder`](../stage/hashing_encoder.md).

## Scope And Alias

The type is `lymph::prep::HashingEncoder`. It takes an
[`options::HashingEncoder`](HashingEncoderOptions.md) (family alias
`prep::HashingEncoderOptions`) at construction, default-constructed if omitted.
It shares its whole method surface with
[`prep::OneHotEncoder`](OneHotEncoder.md) — `features` / `learn` / `map` /
`learn_map` / `learned` / `summary`. It has **no** single-column `Slate`
overload; use the `DataFrame` path.

## Construction

`explicit HashingEncoder(HashingEncoderOptions options = {});` — cheap, no data
touched.

## Transform

There is nothing to learn beyond the frame shape — the bucket is a pure function
of the value and the `seed`. Replay emits `n_components` columns per pinned
column, named `<name>_0 .. <name>_{n-1}`, with a `1` in the hashed bucket.
Because it is stateless:

- an **unseen category** simply hashes somewhere — no error, no policy needed;
- two distinct categories can land in the **same bucket** (a collision) — the
  price of a fixed width. Widening `n_components` lowers the collision rate.

Reach for it when cardinality is very large or unbounded, or in a streaming
setting where a vocabulary cannot be fixed up front. A missing value produces an
all-zero row. Text (note-backed) columns are accepted on the `DataFrame` path.

## `summary()`

Returns a `TransformerSummary`; the `state` records only the frame shape and the
options, since there is no learned vocabulary.

## Methods

The method surface is shared verbatim with
[`prep::OneHotEncoder`](OneHotEncoder.md) — see that page for the full
signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](OneHotEncoder.md#features) | pin the categorical columns (required) |
| [`learn(frame)`](OneHotEncoder.md#learn) | record the frame shape; returns `*this` |
| [`map(frame)`](OneHotEncoder.md#map) | emit the `n_components` bucket columns; `const` |
| [`learn_map(frame)`](OneHotEncoder.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](OneHotEncoder.md#learned) | has `learn` run yet? |
| [`summary()`](OneHotEncoder.md#summary) | shape/options as a `TransformerSummary` |

## Practical Examples

### Example 1

Bucket columns, an equal-category collision, and an unseen category at `map`
time.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const DataFrame train({{"tag", Slate(pluss::make_note(
      {"red", "green", "blue", "red", "cyan", "green", "black", "blue"}))}});
  const DataFrame later({{"tag", Slate(pluss::make_note({"blue", "magenta"}))}});

  lymph::prep::HashingEncoder encoder(
      lymph::prep::HashingEncoderOptions{.n_components = 4});
  std::cout << "learn_map(train) -- 4 bucket columns; equal categories may collide:\n"
            << encoder.features({"tag"}).learn_map(train) << "\n";
  std::cout << "map(later) -- unseen 'magenta' still encodes:\n"
            << encoder.map(later) << "\n";

  return 0;
}
```

Output

```text
learn_map(train) -- 4 bucket columns; equal categories may collide:
=================================
     | tag_0  tag_1  tag_2  tag_3
---------------------------------
0    | 0      1      0      0    
1    | 0      0      1      0    
2    | 0      0      0      1    
3    | 0      1      0      0    
4    | 0      0      0      1    
5    | 0      0      1      0    
6    | 1      0      0      0    
7    | 0      0      0      1    
---------------------------------
[8 cs x 4 feat]
map(later) -- unseen 'magenta' still encodes:
=================================
     | tag_0  tag_1  tag_2  tag_3
---------------------------------
0    | 0      0      0      1    
1    | 1      0      0      0    
---------------------------------
[2 cs x 4 feat]
```

Rows 2 and 4 (`blue` and `cyan`) both land in `tag_3` — a collision.

### Example 2

`seed` perturbs the assignment; the same categories move to different buckets.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const DataFrame train({{"tag", Slate(pluss::make_note(
      {"red", "green", "blue", "cyan", "black", "white"}))}});

  lymph::prep::HashingEncoder a(lymph::prep::HashingEncoderOptions{.n_components = 3, .seed = 0});
  lymph::prep::HashingEncoder b(lymph::prep::HashingEncoderOptions{.n_components = 3, .seed = 7});
  std::cout << "seed = 0:\n" << a.features({"tag"}).learn_map(train) << "\n";
  std::cout << "seed = 7 -- same rows, different buckets:\n"
            << b.features({"tag"}).learn_map(train) << "\n";

  return 0;
}
```

Output

```text
seed = 0:
==========================
     | tag_0  tag_1  tag_2
--------------------------
0    | 1      0      0    
1    | 0      1      0    
2    | 0      0      1    
3    | 0      0      1    
4    | 1      0      0    
5    | 0      0      1    
--------------------------
[6 cs x 3 feat]
seed = 7 -- same rows, different buckets:
==========================
     | tag_0  tag_1  tag_2
--------------------------
0    | 0      1      0    
1    | 0      0      1    
2    | 1      0      0    
3    | 1      0      0    
4    | 0      1      0    
5    | 1      0      0    
--------------------------
[6 cs x 3 feat]
```

### Example 3

`n_components` is the width / collision trade-off. Six distinct tags hashed into
2 buckets must collide heavily; into 8 buckets they spread out.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const DataFrame train({{"tag", Slate(pluss::make_note(
      {"red", "green", "blue", "cyan", "black", "white"}))}});

  lymph::prep::HashingEncoder narrow{lymph::prep::HashingEncoderOptions{.n_components = 2}};
  std::cout << "n_components = 2 -- heavy collisions, only 2 possible rows:\n"
            << narrow.features({"tag"}).learn_map(train) << "\n";

  lymph::prep::HashingEncoder wide{lymph::prep::HashingEncoderOptions{.n_components = 8}};
  std::cout << "n_components = 8 -- room to separate the six tags:\n"
            << wide.features({"tag"}).learn_map(train) << "\n";

  return 0;
}
```

Output

```text
n_components = 2 -- heavy collisions, only 2 possible rows:
===================
     | tag_0  tag_1
-------------------
0    | 1      0    
1    | 0      1    
2    | 0      1    
3    | 0      1    
4    | 1      0    
5    | 0      1    
-------------------
[6 cs x 2 feat]
n_components = 8 -- room to separate the six tags:
=============================================================
     | tag_0  tag_1  tag_2  tag_3  tag_4  tag_5  tag_6  tag_7
-------------------------------------------------------------
0    | 0      0      1      0      0      0      0      0    
1    | 0      0      0      0      1      0      0      0    
2    | 0      0      0      0      0      0      0      1    
3    | 0      0      0      0      0      0      0      1    
4    | 0      1      0      0      0      0      0      0    
5    | 0      0      0      0      0      0      1      0    
-------------------------------------------------------------
[6 cs x 8 feat]
```

## See Also

- [`prep::OneHotEncoder`](OneHotEncoder.md) — the shared method surface, in full.
- [`options::HashingEncoder`](HashingEncoderOptions.md) — the option fields.
- [`prep::stage::hashing_encoder`](../stage/hashing_encoder.md) — the factory
  twin.

## Changelog

- **0.1** — standalone `HashingEncoder` with the shared encoder surface and the
  `n_components` / `seed` controls.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

HashingEncoderOptions
```
<!-- apiref:toctree:end -->
