# Hierarchical Cases

Hierarchical cases are PlussMic's structured case-axis engine. They are the
Pluss-native equivalent of a pandas MultiIndex on rows, but the public language
uses `cases` and `levels`.

The case hierarchy is stored as typed level metadata. Display labels may look
like joined strings, but operations use the structured level payloads.

## API Signature

`DataFrame::set_cases(options: SetCasesOptions) -> DataFrame`

`DataFrame::set_cases(feature_name: string_view, drop_feature: bool = false) -> DataFrame`

`DataFrame::set_cases(feature_names: initializer_list<string_view>, drop_features: bool = false, separator: string_view = "\x1F") -> DataFrame`

`DataFrame::has_levels() -> bool`

`DataFrame::levels() -> vector<string>`

`DataFrame::level(name: string_view) -> Thread`

`DataFrame::level(position: int64) -> Thread`

`DataFrame::case_study(pick: CasePick) -> DataFrame`

`DataFrame::sort_cases(options: SortCasesOptions) -> DataFrame`

`DataFrame::group_by(keys: CaseGroup, specs: RangeView<GroupSpec>, dropna_keys: bool = true, render: bool = false) -> DataFrameView`

`DataFrame::rename_levels(renames: RangeView<LevelRename>) -> DataFrame`

`DataFrame::reorder_levels(names: RangeView<string_view>) -> DataFrame`

`DataFrame::drop_levels(names: RangeView<string_view>) -> DataFrame`

`DataFrame::reset_cases(options: ResetCasesOptions) -> DataFrame`

`DataFrame::stack(feature_name: string = "feature", value_name: string = "value") -> DataFrame`

`DataFrame::unstack(options: UnstackOptions) -> DataFrame`

`DataFrame::unstack(level: string_view) -> DataFrame`

## Options

`SetCasesOptions`

- `features`: features promoted into structured case levels.
- `drop`: removes promoted features from the ordinary feature axis when `true`.
- `verify`: checks for duplicate full structured keys when `true`.
- `sort`: sorts by the selected levels after construction when `true`.
- `separator`: display-label separator only; structured operations do not depend on it.

`CasePick`

- `level` and `value`: select by one named level.
- `levels` and `values`: select by several named levels.
- `key`: select by the full structured key in level order.
- `drop`: drops matched levels after selection when `true`.

`SortCasesOptions`

- `levels`: level names used for typed stable sorting. Empty means sort by the flat case axis.
- `ascending`: sort direction.
- `nulls_last`: null ordering.

`CaseGroup`

- `level`: group by one case level.
- `levels`: group by several case levels.

`ResetCasesOptions`

- `drop`: discards case levels when `true`.
- `names`: replacement feature names for materialized case levels.
- `prefix`: fallback name prefix when level names are empty.

`UnstackOptions`

- `level`: case level moved into feature names.
- `drop`: drops remaining case-level materialization from ordinary features after rebuilding the case axis.
- `sort`: sorts unstacked feature labels.
- `separator`: joins value-feature names and unstacked labels when several value features exist.

## Notes

- A hierarchical case axis is typed. Numeric levels remain numeric, text levels remain `Note`, and timeline levels remain temporal.
- `features(...)`, `select(...)`, `drop(...)`, transforms, and feature reordering preserve case-level metadata.
- `case_study(CasePick)` supports partial-level and full-key lookup.
- `group_by(CaseGroup{...})` groups by case levels without requiring those levels to exist as ordinary features.
- `unstack(...)` rejects duplicate output cells. Aggregate first when multiple rows map to the same output cell.
- `stack(...)` moves feature names into the case hierarchy and preserves homogeneous typed values.

## Examples

### Example 1: create structured cases

```cpp
auto indexed = sales.set_cases(DataFrame::SetCasesOptions{
    .features = {"region", "account", "quarter"},
    .drop = false,
    .verify = true,
});
auto names = indexed.levels();
auto regions = indexed.level("region");
```

Expected output:

- `names == {"region", "account", "quarter"}`
- `regions` is a `Note` Thread with one value per case.

### Example 2: select by a partial level

```cpp
auto west = indexed.case_study(DataFrame::CasePick{
    .level = "region",
    .value = "West",
});
```

Expected output:

- all cases whose `region` level equals `West`
- all original features preserved
- structured levels preserved

### Example 3: select by full structured key

```cpp
auto row = indexed.case_study(DataFrame::CasePick{
    .key = {"East", "201", "Q2"},
});
```

Expected output:

- one matching case when the full key is unique
- every matching case when duplicate full keys are allowed

### Example 4: group by a case level

```cpp
std::vector<DataFrame::GroupSpec> specs{
    {"total_sales", "sales", DataFrame::GroupOp::Sum},
    {"n", "", DataFrame::GroupOp::Count},
};
auto out = static_cast<DataFrame>(
    indexed.group_by(DataFrame::CaseGroup{.level = "region"}, specs));
```

Expected output:

```text
region | total_sales | n
West   | 30          | 2
East   | 70          | 2
```

### Example 5: unstack and stack

```cpp
auto compact = sales.set_cases({"region", "account", "quarter"}, true);
auto wide = compact.unstack("quarter");
auto long_again = wide.stack("quarter", "sales");
```

Expected output:

- `wide` has structured case levels `{"region", "account"}`.
- `wide` has one feature per unstacked quarter when there is one value feature.
- `long_again` moves those feature names back into the case hierarchy.

### Example 6: rename, reorder, drop, and reset levels

```cpp
auto renamed = indexed.rename_levels({{"account", "acct"}});
auto reordered = renamed.reorder_levels({"region", "quarter", "acct"});
auto dropped = reordered.drop_levels({"quarter"});
auto reset = indexed.reset_cases(DataFrame::ResetCasesOptions{
    .drop = false,
    .names = {"region_case", "account_case", "quarter_case"},
});
```

Expected output:

- `renamed.levels() == {"region", "acct", "quarter"}`
- `reordered.levels() == {"region", "quarter", "acct"}`
- `dropped.levels() == {"region", "acct"}`
- `reset` has ordinary features named `region_case`, `account_case`, and `quarter_case`, and no structured case levels.

## Demo

See `plussmic_hierarchical_cases_demo` for a complete raw API walkthrough.
