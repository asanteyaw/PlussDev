<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0745d65fd96d -->

# `lymph::models::Model::load`

[Back to `lymph::models::Model`](../Model.md)

`load` is the static factory that reads a bundle written by [`save`](save.md) back
into a fully trained `Model`.

## Signature

```cpp
static lymph::models::Model lymph::models::Model::load(const std::string& path);

static lymph::models::Model lymph::models::Model::restore(
    const lymph::io::FlowBundle& bundle);
```

## What It Does

`load` opens the bundle at `path`, checks its schema version, and reconstructs the
model: every fitted preparation stage and the estimator's learned parameters.
What comes back is not a template — it is a trained artifact. `trained()` is
`true` immediately, `stage_count()` matches the saved model, and the first call
can be [`infer`](infer.md) or [`assess`](assess.md) with no intervening `learn`
(Example 1).

Because the whole flow is restored, a loaded model transforms raw inputs exactly
as the original did — the reloaded scalers and encoders carry the same fitted
state, so predictions match to the last digit (Example 2).

`restore` does the same from an in-memory `io::FlowBundle` rather than a file
path; use it when the bytes arrive over a channel other than the filesystem.

`load` is a `static` member: call it as `lymph::models::Model::load(path)`, not on
an instance.

## Parameters

`const std::string& path`
: The bundle file to read. It must exist and hold a valid, schema-compatible
  bundle.

`const lymph::io::FlowBundle& bundle` (for `restore`)
: An already-parsed bundle held in memory.

## Returns

A trained `lymph::models::Model`. It behaves like any model produced by
[`learn`](learn.md): cheap to copy, ready to `infer`, `assess`, or `save` again.

## Errors

- A `path` that does not exist, or cannot be opened, raises `pluss::Error`
  ("load failed to open input path") — see Example 3.
- A file that is not a valid bundle, or whose schema version this build does not
  understand, raises.

## Practical Examples

### Example 1

A full round trip. Train and `save` in one expression, then `load` and confirm
the model is trained the instant it comes back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  // Train in one place, persist, then load elsewhere.
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_round_trip.flow";
  lymph::models::logistic_regression().learn(x, y).save(path.string());

  // load is a static factory: it returns a fully fitted Model read back from
  // the bundle. There is no separate "trained yet?" step and no re-fit.
  const lymph::models::Model model = lymph::models::Model::load(path.string());
  std::cout << "trained right after load = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame result({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << result;

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
trained right after load = true

=================================
     | income  balance  predicted
---------------------------------
0    | 1       0.3      0        
1    | 1.4     0.5      0        
2    | 1.8     0.7      0        
3    | 3.4     1.8      1        
4    | 3.8     2.1      1        
5    | 4.2     2.4      1        
---------------------------------
[6 cs x 3 feat]
```

### Example 2

A model with preparation. `load` restores the two stages, so the loaded model
scales and encodes raw inputs exactly as the original — predictions line up.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"region", Thread(region)}});

  // A model with two preparation stages plus the estimator.
  lymph::models::Model original = lymph::models::logistic_regression();
  original.prepare().scale({"income"}).encode({"region"});
  original.learn_(x, y);

  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_staged.flow";
  original.save(path.string());

  // load restores the whole flow, preparation stages included, so the loaded
  // model transforms raw inputs exactly as the original did.
  const lymph::models::Model loaded = lymph::models::Model::load(path.string());
  std::cout << "original stage_count = " << original.stage_count() << "\n";
  std::cout << "loaded   stage_count = " << loaded.stage_count() << "\n\n";

  const DataFrame check({
      {"income", Thread(income)},
      {"region", Thread(region)},
      {"original", Thread(original.infer(x))},
      {"loaded", Thread(loaded.infer(x))},
  });
  std::cout << check;

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
original stage_count = 2
loaded   stage_count = 2

=======================================
     | income  region  original  loaded
---------------------------------------
0    | 1200    0       0         0     
1    | 1800    1       0         0     
2    | 3400    2       1         1     
3    | 4100    0       0         0     
4    | 5200    1       1         1     
5    | 6000    2       1         1     
---------------------------------------
[6 cs x 4 feat]
```

### Example 3

The failure path. `load` on a path that does not exist raises; treat it as an
ordinary I/O error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // load fails loudly when the path does not exist or does not hold a valid
  // bundle. Handle it the way you would any other I/O error.
  try {
    const lymph::models::Model model =
        lymph::models::Model::load("/tmp/does-not-exist-lymph.flow");
    std::cout << "unexpectedly loaded a model\n";
  } catch (const std::exception& err) {
    std::cout << "load threw: " << err.what() << "\n";
  }

  return 0;
}
```

Output

```text
load threw: load: load failed to open input path.
```

## See Also

- [save](save.md) — writes the bundle `load` reads.
- [learn](learn.md) — the other way to obtain a trained model.
- [infer](infer.md) — usually the first call on a freshly loaded model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="prepare.html">
    <span class="label">Next</span>
    <span class="title">prepare</span>
  </a>
</div>
<!-- apiref:nav:end -->
