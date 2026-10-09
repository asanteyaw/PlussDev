<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 91527e93b730 -->

# `lymph::models::builder::Builder::benchmarked`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`benchmarked` reports whether the workspace currently holds a remembered
comparison outcome — that is, whether [`best`](best.md) and [`worst`](worst.md)
can be called.

## Signature

```cpp
bool lymph::models::builder::Builder::benchmarked() const noexcept;
```

## What It Does

Every [`benchmark`](benchmark.md) call stores its `CompareResult` on the
`Builder`. `benchmarked()` returns `true` from that point on, and `false` before
the first `benchmark`. It is `noexcept` and does no work — a flag check.

Its purpose is to guard [`best()`](best.md) and [`worst()`](worst.md), which
throw when no comparison has been run. Re-running `benchmark` replaces the
remembered outcome; `benchmarked()` stays `true` and the registry is unaffected
(Example 3).

## Parameters

None.

## Returns

`true` if a `benchmark` outcome is remembered, `false` otherwise.

## Errors

None.

## Practical Examples

### Example 1

`false` before the first benchmark, `true` after.

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
  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // benchmarked reports whether a comparison outcome is currently remembered.
  std::cout << "before benchmark: benchmarked = " << std::boolalpha
            << builder.benchmarked() << "\n";

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult ignored = builder.benchmark(x, y, cv);
  (void)ignored;

  std::cout << "after benchmark:  benchmarked = " << builder.benchmarked() << "\n";

  return 0;
}
```

Output

```text
before benchmark: benchmarked = false
after benchmark:  benchmarked = true
```

### Example 2

Using it as a guard before reaching for a winner.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // best() and worst() throw if no comparison has been run. Guard them with
  // benchmarked() when a caller might reach for a winner before benchmarking.
  if (builder.benchmarked()) {
    std::cout << "winner is ready\n";
  } else {
    std::cout << "no benchmark yet -- call builder.benchmark(...) first\n";
  }

  return 0;
}
```

Output

```text
no benchmark yet -- call builder.benchmark(...) first
```

### Example 3

Re-benchmarking replaces the remembered outcome; the flag stays `true` and the
registry is unchanged. Uses the fully explicit spelling for construction.

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
  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult r1 = builder.benchmark(x, y, cv);
  const lymph::selection::CompareResult r2 = builder.benchmark(x, y, cv);
  (void)r1;
  (void)r2;

  // Re-benchmarking replaces the remembered outcome; benchmarked() stays true
  // and the registry still holds the same models.
  std::cout << "benchmarked = " << std::boolalpha << builder.benchmarked() << "\n";
  std::cout << "size        = " << builder.size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  " << name << "\n";

  return 0;
}
```

Output

```text
benchmarked = true
size        = 2
  logreg
  tree
```

## See Also

- [benchmark](benchmark.md) — the call that sets this flag.
- [best](best.md), [worst](worst.md) — what the flag guards.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="benchmark.html">
    <span class="label">Previous</span>
    <span class="title">benchmark</span>
  </a>
  <a class="next-link" href="best.html">
    <span class="label">Next</span>
    <span class="title">best</span>
  </a>
</div>
<!-- apiref:nav:end -->
