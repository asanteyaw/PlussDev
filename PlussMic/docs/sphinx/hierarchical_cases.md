# Hierarchical Cases

Hierarchical cases are PlussMic's structured case-axis engine. They are the
Pluss-native equivalent of a pandas MultiIndex on rows, but the public language
uses `cases` and `labels`.

The case hierarchy is stored as typed case-label metadata. Display labels may
look like joined strings, but operations use the structured case-label payloads.

## API Signature

`DataFrame::label().set(options: SetLabelOptions) -> DataFrame`

`DataFrame::label().set(feature_name: string_view, drop_feature: bool = true) -> DataFrame`

`DataFrame::label().set(feature_names: initializer_list<string_view>, drop_features: bool = true, separator: string_view = "\x1F") -> DataFrame`

`DataFrame::label().has() -> bool`

`DataFrame::label().names() -> vector<string>`

`DataFrame::label().part(name: string_view) -> Slate`

`DataFrame::label().part(position: int64) -> Slate`

`DataFrameCaseStudyAccessor::pick(const CasePick& pick) const -> DataFrame`

`DataFrame::label().sort(options: SortLabelOptions) -> DataFrame`

`DataFrame::group().by(keys: LabelGroup).agg(specs) -> DataFrame`

`DataFrame::label().rename(renames: RangeView<LabelRename>) -> DataFrame`

`DataFrame::label().order(names: RangeView<string_view>) -> DataFrame`

`DataFrame::label().drop(names: RangeView<string_view>) -> DataFrame`

`DataFrame::label().reset(options: ResetLabelOptions) -> DataFrame`

`DataFrame::stack(feature_name: string = "feature", value_name: string = "value") -> DataFrame`

`DataFrame::unstack(options: UnstackOptions) -> DataFrame`

`DataFrame::unstack(label: string_view) -> DataFrame`

## Options

`SetLabelOptions`

- `features`: features promoted into structured case labels.
- `drop`: removes promoted features from the ordinary feature axis when `true`.
- `verify`: checks for duplicate full structured keys when `true`.
- `sort`: sorts by the selected case labels after construction when `true`.
- `separator`: display-label separator only; structured operations do not depend on it.

`CasePick`

- `caselbl` and `value`: select by one named case label.
- `caselbls` and `values`: select by several named case labels.
- `key`: select by the full structured key in case-label order.
- `drop`: drops matched case labels after selection when `true`.

`SortCaselblOptions`

- `caselbls`: case-label names used for typed stable sorting. Empty means sort by the flat case axis.
- `ascending`: sort direction.
- `nulls_last`: null ordering.

`LabelGroup`

- `caselbl`: group by one case label.
- `caselbls`: group by several case labels.

`ResetLabelOptions`

- `drop`: discards case labels when `true`.
- `names`: replacement feature names for materialized case labels.
- `prefix`: fallback name prefix when case-label names are empty.

`UnstackOptions`

- `caselbl`: case label moved into feature names.
- `drop`: drops remaining case-label materialization from ordinary features after rebuilding the case axis.
- `sort`: sorts unstacked feature labels.
- `separator`: joins value-feature names and unstacked labels when several value features exist.

## Notes

- A hierarchical case axis is typed. Numeric case labels remain numeric, text case labels remain `Note`, and timeline case labels remain temporal.
- `features(...)`, `select(...)`, `drop(...)`, transforms, and feature reordering preserve case-label metadata.
- `case_study().pick(CasePick)` supports partial-label and full-key lookup.
- `group().by(LabelGroup{...}).agg(...)` groups by case labels without requiring those labels to exist as ordinary features.
- `unstack(...)` rejects duplicate output cells. Aggregate first when multiple rows map to the same output cell.
- `stack(...)` moves feature names into the case hierarchy and preserves homogeneous typed values.

## Examples

### Example 1: create structured cases

```cpp
auto indexed = sales.label().set(DataFrame::SetLabelOptions{
    .features = {"region", "account", "quarter"},
    .drop = false,
    .verify = true,
});
auto names = indexed.label().names();
auto regions = indexed.caselbl("region");
```

Expected output:

- `names == {"region", "account", "quarter"}`
- `regions` is a `Note` Slate with one value per case.

### Example 2: select by a partial case label

```cpp
auto west = indexed.case_study().pick(DataFrame::CasePick{
    .label = "region",
    .value = "West",
});
```

Expected output:

- all cases whose `region` case label equals `West`
- all original features preserved
- structured case labels preserved

### Example 3: select by full structured key

```cpp
auto row = indexed.case_study().pick(DataFrame::CasePick{
    .key = {"East", "201", "Q2"},
});
```

Expected output:

- one matching case when the full key is unique
- every matching case when duplicate full keys are allowed

### Example 4: group by a case label

```cpp
std::vector<DataFrame::GroupSpec> specs{
    {"total_sales", "sales", DataFrame::GroupOp::Sum},
    {"n", "", DataFrame::GroupOp::Count},
};
auto out =
    indexed.group().by(DataFrame::LabelGroup{.label = "region"}).agg(specs);
```

Expected output:

```text
region | total_sales | n
West   | 30          | 2
East   | 70          | 2
```

### Example 5: unstack and stack

```cpp
auto compact = sales.label().set({"region", "account", "quarter"}, true);
auto wide = compact.unstack("quarter");
auto long_again = wide.stack("quarter", "sales");
```

Expected output:

- `wide` has structured case labels `{"region", "account"}`.
- `wide` has one feature per unstacked quarter when there is one value feature.
- `long_again` moves those feature names back into the case hierarchy.

### Example 6: rename, reorder, drop, and reset case labels

```cpp
auto renamed = indexed.label().rename({{"account", "acct"}});
auto reordered = renamed.label().order({"region", "quarter", "acct"});
auto dropped = reordered.label().drop({"quarter"});
auto reset = indexed.label().reset(DataFrame::ResetLabelOptions{
    .drop = false,
    .names = {"region_case", "account_case", "quarter_case"},
});
```

Expected output:

- `renamed.label().names() == {"region", "acct", "quarter"}`
- `reordered.label().names() == {"region", "quarter", "acct"}`
- `dropped.label().names() == {"region", "acct"}`
- `reset` has ordinary features named `region_case`, `account_case`, and `quarter_case`, and no structured case labels.

## Demo

See `plussmic_hierarchical_cases_demo` for a complete raw API walkthrough.
