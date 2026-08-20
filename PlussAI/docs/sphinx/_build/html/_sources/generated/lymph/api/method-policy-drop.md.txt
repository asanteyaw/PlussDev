# `Policy::drop`

## API Signature

- `Policy::drop(names: std::initializer_list<std::string>) -> Policy&`
- `Policy::drop(names: std::vector<std::string>) -> Policy&`

## Notes

Instance method on `self`.

## Inputs

- `names` (`std::initializer_list<std::string> | std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `Policy&`.

## Options

Option semantics used by this method:

- `Policy` (`struct`):
  - `std::vector<std::string> drop_columns{}`
    Option field used by this method.
  - `std::vector<std::string> require_columns{}`
    Option field used by this method.
  - `std::vector<std::string> forbid_columns{}`
    Option field used by this method.
  - `std::vector<std::string> reject_missing{}`
    Option field used by this method.
  - `std::vector<std::string> allow_missing{}`
    Option field used by this method.
  - `bool filter_nulls = false`
    Option field used by this method.
  - `std::vector<std::string> null_filter_columns{}`
    Option field used by this method.
  - `bool drop_only_if_all_null = false`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Policy::drop(/* names */);
out.info("drop", value);
```

**Expected output**

```text
drop returned Policy&
```

### Example 2 (Demo Pattern)

```cpp
      housing,
      lymph::HoldoutSplitOptions{
          .test_size = 0.2,
          .stratify_feature = "income_cat",
          .random_state = 42,
          .deterministic = true,
      });

  strat_train_set = strat_train_set.drop({"income_cat"});
  strat_test_set = strat_test_set.drop({"income_cat"});
  out.writef("train rows={} test rows={}", strat_train_set.ncases(), strat_test_set.ncases()).endln();
  //========== End Cell 2 ========

  //========== Cell 3: EDA + Correlation Snapshot ========
  // Quick, notebook-style checks and one correlation ranking view.
  auto corr_ranked =
      strat_train_set.feataxis().series().corr().arrange({"median_house_value"}, false).features({"median_house_value"});
  out.writeln("top correlations with median_house_value");
  corr_ranked.head(10);
  //========== End Cell 3 ========
```

**Expected output**

```text
drop returned Policy&
```
