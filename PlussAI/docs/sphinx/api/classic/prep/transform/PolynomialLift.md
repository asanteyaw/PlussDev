# `pluss::ai::classic::prep::PolynomialLift`

Expand the feature set into its own polynomial terms, up to `degree`: every
single feature raised to powers `1..degree`, and (unless
`interactions_only`) every product of two or more distinct features up to
that same total degree. Reach for this ahead of a linear model that needs
explicit curvature or interaction terms it cannot otherwise represent — a
`LinearFamilyOptions::objective = LeastSquares` fit downstream of this
becomes, in effect, a polynomial regression.

## Signature

```cpp
explicit classic::prep::PolynomialLift(classic::prep::PolynomialLiftOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`PolynomialLiftOptions`](PolynomialLiftOptions.md).

## State

The fitted list of terms (each a list of input feature indices to multiply
together) and the resulting output width live inside the object, set by
`learn`, read back only through `map` / `dump_state`. `trained()` reports
whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Enumerates every term from degree `1` through `degree`, in degree-major
  order: for each degree `d`, every non-decreasing `d`-length combination
  of feature indices (`interactions_only = false`, the default — this
  includes repeated indices, i.e. `x_i^d`-style pure-power terms) or every
  strictly-increasing `d`-length combination (`interactions_only = true` —
  no feature multiplied by itself, cross-terms only). `include_bias`
  additionally prepends a constant `1.0` column ahead of every term. Fails
  with `Status::invalid_argument` if `degree < 1`. Does not read the
  batch's actual values — only its feature count, to enumerate index
  combinations.

`map(batch, options = {})`
: For each fitted term, multiplies together the values of every feature
  index the term names, for every row — a degree-1 term is just that
  feature's own value; a degree-`d` term is the product of `d` such
  values (with repeats, for a pure power). If **any** input feature in a
  term is `NaN` for a row, that term's output is `NaN` for that row — one
  missing input poisons every term it participates in, not just the
  degree-1 term for that feature. `Status::precondition` if not yet
  trained.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, term_sizes, flat_term_indices]`, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence).

## Errors

- `degree < 1` — `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

The full `degree = 2` expansion of two features (every power and cross
term), the same expansion under `interactions_only` (squares dropped), and
`include_bias` prepending a constant column.

### Example 1

Two features, `degree = 2`: five output columns — both linear terms, both
squares, and the one cross term.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({2.0, 3.0}, {2}, f64).reshape({1, 2});

  cls::prep::PolynomialLiftOptions options{};
  options.degree = 2;
  cls::prep::PolynomialLift lift(options);
  std::cout << "x0=2, x1=3 -> " << lift.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
x0=2, x1=3 -> pluss::Series(shape=[1,5], strides=[5,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[2, 3, 4, 6, 9]]
```

### Example 2

`interactions_only = true` on the same data drops both squared terms,
keeping only the linear terms and the one cross term.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({2.0, 3.0}, {2}, f64).reshape({1, 2});

  cls::prep::PolynomialLiftOptions options{};
  options.degree = 2;
  options.interactions_only = true;
  cls::prep::PolynomialLift lift(options);
  std::cout << "interactions_only, x0=2, x1=3 -> " << lift.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
interactions_only, x0=2, x1=3 -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[2, 3, 6]]
```

### Example 3

`include_bias = true` prepends a constant `1.0` column ahead of every term
— and a missing input poisons every term it appears in, not just its own
degree-1 column.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({nan, 3.0}, {2}, f64).reshape({1, 2});

  cls::prep::PolynomialLiftOptions options{};
  options.degree = 2;
  options.include_bias = true;
  cls::prep::PolynomialLift lift(options);
  const pluss::Series mapped = lift.learn_map(batch).value();

  // Series' own printer truncates a wide row; unpack it to see every term.
  const std::vector<double> terms = mapped.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({mapped.numel()}).to_vec<double>();
  const char* names[] = {"bias", "x0", "x1", "x0^2", "x0*x1", "x1^2"};
  for (std::size_t i = 0; i < terms.size(); ++i) {
    std::cout << names[i] << " = " << terms[i] << "\n";
  }

  return 0;
}
```

Output

```text
bias = 1
x0 = nan
x1 = 3
x0^2 = nan
x0*x1 = nan
x1^2 = 9
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `degree`/`interactions_only`/
  `include_bias`, `dump_state`/`load_state`.

## See Also

- [`PolynomialLiftOptions`](PolynomialLiftOptions.md) — the options struct.
- [`LinearFamilyOptions`](../../families/LinearFamilyOptions.md) — the
  model family this is most often paired with.
- [`transform/Introduction`](Introduction.md) — the parent sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

PolynomialLiftOptions
```
<!-- apiref:toctree:end -->
