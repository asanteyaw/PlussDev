# `lymph::prep::OneHotEncoder`

`OneHotEncoder` is the standalone, fittable one-hot expansion object of the
`prep` family: construct it, [`learn`](#learn) the category set of one or more
columns on a frame, then [`map`](#map) that layout onto any later frame, where
each learned category becomes its own `0/1` indicator column. It is the object
twin of the [`prep::stage::one_hot_encoder`](../stage/one_hot_encoder.md)
factory — same arithmetic, same [`options::OneHotEncoder`](OneHotEncoderOptions.md),
but a value you hold and inspect rather than a step folded into a model.

This page is the reference for the method surface shared by every encoder in the
[`prep::encoding`](../encoding.md) subfamily.

## Scope And Alias

The type is `lymph::prep::OneHotEncoder`. It takes an
[`options::OneHotEncoder`](OneHotEncoderOptions.md) (family alias
`prep::OneHotEncoderOptions`) at construction, default-constructed if omitted.

```cpp
lymph::prep::OneHotEncoder a;                                    // defaults
lymph::prep::OneHotEncoder b{lymph::options::OneHotEncoder{.include_unknown_bucket = false}};
```

## Construction

`explicit OneHotEncoder(options::OneHotEncoder options = {});`

Cheap — no data touched. The three option fields (`max_categories`, `overflow`,
`include_unknown_bucket`) are covered on the
[`options::OneHotEncoder`](OneHotEncoderOptions.md) and
[`prep::stage::one_hot_encoder`](../stage/one_hot_encoder.md) pages.

## State

Before [`learn`](#learn) the object holds only its options.
[`learned()`](#learned) reads `false`. After `learn` it holds, per pinned
column, the ordered set of categories seen at fit time — reachable through
[`summary()`](#summary) as a `state` tensor vector, and through the public
`categories_` / `mapped_names_` attributes ([Attributes](#attributes)). Fitting
again replaces that state.

## Methods

### `features`

```cpp
OneHotEncoder& features(std::initializer_list<std::string_view> names);
```

A second overload takes any range of string views in place of the braced list.

Pin the categorical columns the encoder acts on, and fix their order. Returns
`*this`, so it chains into `learn`. **Required** on the `DataFrame` path — an
encoder with no pinned columns raises rather than guessing which columns are
categorical. (The single-column [`Slate` overloads](#single-column-overloads)
need no pin.)

### `learn`

```cpp
OneHotEncoder& learn(const pluss::table::DataFrame& frame,
                     const TrainOptions& options = {});
```

Record the category set of each pinned column from `frame`. Returns `*this` for
chaining, and flips [`learned()`](#learned) to `true`. Calling `learn` again
re-fits from scratch on the new frame — the object is a reusable template, not
single-shot.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // region is an integer-coded category (0, 1, 2). The standalone encoder holds
  // its fitted category layout; pin the column with features(), fit with
  // learn() (which returns *this so it chains), then map() any later frame.
  const DataFrame train({{"region", Slate(pluss::make_series({0., 1., 2., 0., 1., 2.}, {6}, f64))}});
  const DataFrame later({{"region", Slate(pluss::make_series({2., 0., 1.}, {3}, f64))}});

  lymph::prep::OneHotEncoder encoder;
  std::cout << "learned before = " << std::boolalpha << encoder.learned() << "\n";

  encoder.features({"region"}).learn(train);
  std::cout << "learned after  = " << encoder.learned() << "\n\n";

  std::cout << "map(train) -- one indicator column per category, plus a reserved\n"
               "region_unknown column (include_unknown_bucket defaults to true):\n"
            << encoder.map(train) << "\n";
  std::cout << "map(later) -- same fitted layout replayed on new rows:\n"
            << encoder.map(later) << "\n";

  lymph::prep::OneHotEncoder fresh;
  std::cout << "learn_map(train) -- fit and apply in one call:\n"
            << fresh.features({"region"}).learn_map(train);

  return 0;
}
```

Output

```text
learned before = false
learned after  = true

map(train) -- one indicator column per category, plus a reserved
region_unknown column (include_unknown_bucket defaults to true):
=============================================
     | region_0  region_1  region_2  region_3
---------------------------------------------
0    | 1         0         0         0       
1    | 0         1         0         0       
2    | 0         0         1         0       
3    | 1         0         0         0       
4    | 0         1         0         0       
5    | 0         0         1         0       
---------------------------------------------
[6 cs x 4 feat]
map(later) -- same fitted layout replayed on new rows:
=============================================
     | region_0  region_1  region_2  region_3
---------------------------------------------
0    | 0         0         1         0       
1    | 1         0         0         0       
2    | 0         1         0         0       
---------------------------------------------
[3 cs x 4 feat]
learn_map(train) -- fit and apply in one call:
=============================================
     | region_0  region_1  region_2  region_3
---------------------------------------------
0    | 1         0         0         0       
1    | 0         1         0         0       
2    | 0         0         1         0       
3    | 1         0         0         0       
4    | 0         1         0         0       
5    | 0         0         1         0       
---------------------------------------------
[6 cs x 4 feat]
```

`region_0..region_2` are the three learned categories; `region_3` is the
reserved unknown bucket. The returned frame is rebuilt with `<name>_<i>`
columns — see [Attributes](#attributes) for the `<name>_<category>` labels.

### `map`

```cpp
DataFrame map(const pluss::table::DataFrame& frame,
              const InferOptions& options = {}) const;
```

Apply the fitted category layout to `frame`, returning a transformed copy — the
routed column is replaced by its indicator columns, every other column passes
through untouched. `frame` may have a different row count than the training
frame. `map` is `const` and does not consume the object, so the same encoder
serves many frames.

A category not seen at `learn` time is governed by
`options.unknown_categorical` (an `InferOptions` field of type
`UnknownCategoryPolicy`), which **defaults to `Error`**:

| `unknown_categorical` | effect on an unknown row |
| --- | --- |
| `Error` (default) | `map` fails |
| `MissingBucket` | a `1` in the reserved `_unknown` column (needs `include_unknown_bucket`) |
| `Ignore` | the row is left all-zero |

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train({{"region", Slate(pluss::make_series({0., 1., 2., 0., 1., 2.}, {6}, f64))}});
  // Row 1 carries category 9, never seen at fit time.
  const DataFrame later({{"region", Slate(pluss::make_series({1., 9.}, {2}, f64))}});

  lymph::prep::OneHotEncoder encoder;
  encoder.features({"region"}).learn(train);

  try {
    const DataFrame out = encoder.map(later);
    std::cout << "default: " << out.ncases() << " rows\n";
  } catch (const std::exception& ex) {
    std::cout << "default (Error) threw: " << ex.what() << "\n\n";
  }

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket -- unknown row lands in region_unknown:\n"
            << encoder.map(later, to_bucket) << "\n";

  lymph::InferOptions ignore;
  ignore.unknown_categorical = lymph::UnknownCategoryPolicy::Ignore;
  std::cout << "Ignore -- unknown row is left all-zero:\n"
            << encoder.map(later, ignore);

  return 0;
}
```

Output

```text
default (Error) threw: prep::OneHotEncoder.map: OneHotEncoder saw unknown category during map().
Triggered at: /.../lymph/common.h:122 (T pluss::ai::lymph::resolve(StatusOr<T>, std::string_view) [T = pluss::codem::Series])

MissingBucket -- unknown row lands in region_unknown:
=============================================
     | region_0  region_1  region_2  region_3
---------------------------------------------
0    | 0         1         0         0       
1    | 0         0         0         1       
---------------------------------------------
[2 cs x 4 feat]
Ignore -- unknown row is left all-zero:
=============================================
     | region_0  region_1  region_2  region_3
---------------------------------------------
0    | 0         1         0         0       
1    | 0         0         0         0       
---------------------------------------------
[2 cs x 4 feat]
```

### `learn_map`

```cpp
DataFrame learn_map(const pluss::table::DataFrame& frame,
                    const TrainOptions& learn_options = {},
                    const InferOptions& infer_options = {});
```

`learn(frame)` then `map(frame)` in one call, returning the transformed frame.
The common shape when you only need the training frame transformed once. The
`infer_options` argument carries the same `unknown_categorical` choice as
[`map`](#map) — relevant here when a tight `max_categories` folds rare
categories into the unknown slot at fit time.

### `learned`

```cpp
bool learned() const noexcept;
```

`true` once [`learn`](#learn) or [`learn_map`](#learn_map) has run. Calling
[`map`](#map) before that is an error. Use it to guard a reused encoder — see the
`false` → `true` transition in the [`learn` example](#learn).

### `summary`

```cpp
TransformerSummary summary() const;
```

### `TransformerSummary` fields

| Field | Type | Default |
|---|---|---|
| `feature_names` | `std::vector<std::string>` | — |
| `state` | `std::vector<Series>` | — |

The generic fitted-state view every encoder in the subfamily returns. For a
readable layout use the [Attributes](#attributes) below instead.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Slate(pluss::make_series({0., 1., 2., 0., 1., 2.}, {6}, f64))}});

  lymph::prep::OneHotEncoder encoder;
  encoder.features({"region"}).learn(train);

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary.feature_names[0] = " << s.feature_names[0] << "\n";
  std::cout << "summary.state tensors    = " << s.state.size() << "\n\n";

  std::cout << "categories_[0] :";
  for (const auto& c : encoder.categories_[0]) std::cout << " " << c;
  std::cout << "\nmapped_names_  :";
  for (const auto& n : encoder.mapped_names_) std::cout << " " << n;
  std::cout << "\n";

  return 0;
}
```

Output

```text
summary.feature_names[0] = region
summary.state tensors    = 5

categories_[0] : 0 1 2
mapped_names_  : region_0 region_1 region_2 region_unknown
```

## Single-column overloads

`OneHotEncoder`, `OrdinalEncoder`, `LabelEncoder` and `FrequencyEncoder` add a
`Slate`-in / `Series`-out overload of each of `learn` / `map` / `learn_map`:

```cpp
OneHotEncoder& learn(const pluss::table::Slate& feature, const TrainOptions& = {});
Series         map(const pluss::table::Slate& feature, const InferOptions& = {}) const;
Series         learn_map(const pluss::table::Slate& feature,
                         const TrainOptions& = {}, const InferOptions& = {});
```

No `features()` pin is needed, and a text (note-backed) column keeps its string
category labels in `mapped_names_` — the `DataFrame` path over a note-backed
column hashes them instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::Slate;

  const Slate region(pluss::make_note({"north", "west", "north", "south", "east"}));

  lymph::prep::OneHotEncoder encoder;
  const pluss::Series indicators = encoder.learn_map(region);
  std::cout << "one-hot matrix (rows x categories, last column is unknown):\n"
            << indicators << "\n";

  const Slate query(pluss::make_note({"south", "north"}));
  std::cout << "map(query):\n" << encoder.map(query) << "\n";
  std::cout << "column labels: ";
  for (const auto& n : encoder.mapped_names_) std::cout << n << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
one-hot matrix (rows x categories, last column is unknown):
pluss::Series(shape=[5,5], strides=[5,1], dtype=float64, platform=cpu, numel=25, contiguous=true)
data=[[0, 0, 0, 1, 0],
  [1, 0, 0, 0, 0],
  [0, 0, 0, 1, 0],
  [0, 1, 0, 0, 0],
  [0, 0, 1, 0, 0]]
map(query):
pluss::Series(shape=[2,5], strides=[5,1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[[0, 1, 0, 0, 0],
  [0, 0, 0, 1, 0]]
column labels: feature_0_west feature_0_south feature_0_east feature_0_north feature_0_unknown 
```

## Attributes

After `learn`, these public members carry the human-readable layout (one entry
per pinned feature):

| Member | Contents |
| --- | --- |
| `std::vector<std::vector<std::string>> categories_` | the learned category set of each feature, in code order |
| `std::vector<std::string> mapped_names_` | the output column labels — `<name>_<category>` per category, then `<name>_unknown` |
| `std::vector<std::vector<double>> category_keys_` | the raw numeric keys backing each category |
| `std::vector<std::vector<double>> frequencies_` | fit-time counts per category (used by `FrequencyEncoder`) |

The `DataFrame` returned by `map` names its columns positionally (`<name>_0`,
`<name>_1`, …); `mapped_names_` is where the `<name>_<category>` spelling lives.

## Notes

- The encoder holds no reference to any frame — it is a snapshot of the fitted
  category layout. Mutating the training frame afterwards does not change a
  fitted encoder.
- `map` returns a new frame; it does not mutate its argument.
- To persist a fitted encoder as part of a model, attach the transform through
  [`Model::append`](../../models/Model/append.md) with
  [`prep::stage::one_hot_encoder`](../stage/one_hot_encoder.md) instead — the
  standalone object has no `save` / `load` of its own.

## Changelog

- **0.1** — standalone `OneHotEncoder` with `features` / `learn` / `map` /
  `learn_map` / `learned` / `summary`, the single-column `Slate` overloads, and
  the `categories_` / `mapped_names_` attributes.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

OneHotEncoderOptions
```
<!-- apiref:toctree:end -->
