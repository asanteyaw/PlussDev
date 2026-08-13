# `ModelPrep::add`

## API Signature

- `ModelPrep::add(stage: Stage, router: ColumnRouter) -> ModelPrep&`

## Notes

Instance method on `self`.

## Inputs

- `router` (`ColumnRouter`)
  Method argument; see signature type for accepted form.
- `stage` (`Stage`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ModelPrep&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelPrep::add(/* stage */, /* router */);
out.info("add", value);
```

**Expected output**

```text
add returned ModelPrep&
```

### Example 2 (Demo Pattern)

```cpp
  const auto category = pluss::make_note({"red", "blue", "red", "green", "blue", "red"});
  const Series epoch_nanos = pluss::make_series(
      std::vector<std::int64_t>{
          1'700'000'000'000'000'000LL, 1'700'000'001'000'000'000LL, 1'700'000'002'000'000'000LL,
          1'700'000'003'000'000'000LL, 1'700'000'004'000'000'000LL, 1'700'000'005'000'000'000LL},
      {6},
      Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const Thread ts = Thread(epoch_nanos).cast().timeline();
  const Series y = x0.mul(2.0).sub(x1.mul(0.5)).add(1.0);
  const Series w = pluss::make_series(std::vector<double>{1.0, 1.0, 0.8, 1.2, 1.0, 0.9}, {6}, f64());

  return DataFrame({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"category", Thread(category)},
      {"ts", ts},
      {"target", Thread(y)},
      {"weight", Thread(w)},
  });
}
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.51681e-17 1
NodeJ DataFrame bridge demo completed.
```

### Example 3 (Demo Pattern)

```cpp
#include <vector>

int main() {
  using namespace pluss::ai;

  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64);
  const pluss::table::Thread cat = pluss::table::Thread(pluss::make_note({"red", "blue", "red", "blue", "red", "blue"}));
  const pluss::Series y = x.mul(2.0).add(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0, 0.0, 1.0}, {6}, f64));

  const pluss::table::DataFrame housing({
      {"median_income", pluss::table::Thread(x)},
      {"ocean_proximity", cat},
      {"target", pluss::table::Thread(y)},
  });

  const auto numeric = housing.feataxis().series().features();
  const auto categorical = housing.feataxis().note().features();

  pluss::ai::nodej::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"median_income", "ocean_proximity", "target"};
```

**Expected output**

```text
numeric features: median_income target
categorical features: ocean_proximity
ordinal mapped names: ocean_proximity_ordinal
label mapped names: ocean_proximity_label
ordinal sample: ======================
     | ocean_proximity
----------------------
0    | 2
1    | 1
2    | 2
3    | 1
4    | 2
...
```
