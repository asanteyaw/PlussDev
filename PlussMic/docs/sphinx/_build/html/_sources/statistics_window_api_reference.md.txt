# Stabilization, Statistics, and Window API Reference

The primary numeric outputs below are verified by `plussmic_table_statistics_window_tests`. Examples use `df.x=[1,2,3,4]`, `df.y=[2,4,6,8]`; `reversed.x=[4,3,2,1]`; `repeated.x=[20,10,20]`; `nullable.x=[1,NA,3,4]`; and the grouped/time fixtures shown in the test.

## `matched_with`

### API Signature

```text
DataFrame::matched_with(other: const DataFrame&, options: FrameMatchOptions = {}) -> FrameMatch
```

### Notes

Returns two frames with identical case and feature axes. Numeric bulk operations preserve the input Series platform.

### Inputs

`other: const DataFrame&`
    Second frame aligned or multiplied with the current frame.

`options: FrameMatchOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`FrameMatch` containing two frames with identical case and feature axes.

### Options

- Axis rules: `Unite`, `Common`, `KeepLeft`, and `KeepRight`.
- Repeated cases: `PairInOrder` or `Reject`.

### Examples

Example 1.

```cpp
auto out = left.matched_with(right);
```

#### Expected output

```text
out.left and out.right have identical stable-union axes
```

Example 2.

```cpp
auto out = left.matched_with(right, {.cases=FrameMatchRule::Common, .features=FrameMatchRule::Common});
```

#### Expected output

```text
both axes contain only labels shared by left and right
```

Example 3.

```cpp
auto out = left.matched_with(right, {.cases=FrameMatchRule::KeepLeft, .features=FrameMatchRule::KeepLeft});
```

#### Expected output

```text
both outputs use the left axes; unmatched right cells are NA
```

Example 4.

```cpp
auto out = left.matched_with(right, {.cases=FrameMatchRule::KeepRight, .features=FrameMatchRule::KeepRight});
```

#### Expected output

```text
both outputs use the right axes; unmatched left cells are NA
```

Example 5.

```cpp
auto out = left.matched_with(right, {.repeated_cases=RepeatedCaseRule::Reject});
```

#### Expected output

```text
the call fails if either case axis contains repeated labels
```

## `extend_cases`

### API Signature

```text
DataFrame::extend_cases(other: const DataFrame&, features: FrameMatchRule = Unite, fresh_cases: bool = false, reject_repeated_cases: bool = false) -> DataFrame
```

### Notes

Returns case-wise extension with matched features. Numeric bulk operations preserve the input Series platform.

### Inputs

`other: const DataFrame&`
    Second frame aligned or multiplied with the current frame.

`features: FrameMatchRule = Unite`
    Rule selecting union or intersection behavior on the feature axis.

`fresh_cases: bool = false`
    Creates contiguous positional cases when true.

`reject_repeated_cases: bool = false`
    Fails when extension would preserve repeated case labels.

### Output

`DataFrame` containing case-wise extension with matched features.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = north.extend_cases(south);
```

#### Expected output

```text
south cases follow north cases; one-sided features are null-filled
```

Example 2.

```cpp
auto out = north.extend_cases(south, FrameMatchRule::Common);
```

#### Expected output

```text
only features shared by both frames are retained
```

Example 3.

```cpp
auto out = north.extend_cases(south, FrameMatchRule::KeepLeft);
```

#### Expected output

```text
the north schema is retained and missing south cells are NA
```

Example 4.

```cpp
auto out = north.extend_cases(south, FrameMatchRule::Unite, true);
```

#### Expected output

```text
output cases are contiguous integers
```

Example 5.

```cpp
auto out = north.extend_cases(south, FrameMatchRule::Unite, false, true);
```

#### Expected output

```text
the call fails if preserved case labels repeat
```

## `unify`

### API Signature

```text
DataFrame::unify(left_on: string_view, right: const DataFrame&, right_on: string_view = "", how: string_view = "common", right_suffix: string = "_right", validate: string_view = "many_to_many") -> DataFrame
DataFrame::unify(left_on: RangeView<string_view>, right: const DataFrame&, right_on: RangeView<string_view> = {}, how: string_view = "common", right_suffix: string = "_right", validate: string_view = "many_to_many") -> DataFrame
DataFrame::unify(left_on: initializer_list<string_view>, right: const DataFrame&, right_on: initializer_list<string_view> = {}, how: string_view = "common", right_suffix: string = "_right", validate: string_view = "many_to_many") -> DataFrame
```

### Notes

Returns key-matched rows from two frames. Numeric bulk operations preserve the input Series platform.

### Inputs

`left_on: RangeView<string_view>`
    Left key feature or features.

`right: const DataFrame&`
    Right frame participating in the operation.

`right_on: RangeView<string_view> = {}`
    Right key features; empty reuses the left key names.

`how: string_view = "common"`
    Key retention mode: common, keep_left, keep_right, unite, cross, left_only, or right_only.

`right_suffix: string = "_right"`
    Suffix for colliding right feature names.

`validate: string_view = "many_to_many"`
    Optional key-cardinality rule.

### Output

`DataFrame` containing key-matched rows from two frames.

### Options

- Retention: `common`, `keep_left`, `keep_right`, `unite`, `cross`, `left_only`, or `right_only`.
- Validation: `many_to_many`, `one_to_one`, `one_to_many`, or `many_to_one`.

### Examples

Example 1.

```cpp
auto out = left.unify("id", right);
```

#### Expected output

```text
only matching id rows are returned
```

Example 2.

```cpp
auto out = left.unify("id", right, "id", "keep_left");
```

#### Expected output

```text
every left key remains; unmatched right fields are NA
```

Example 3.

```cpp
auto out = left.unify("id", right, "id", "unite");
```

#### Expected output

```text
the stable union of keys is returned
```

Example 4.

```cpp
auto out = left.unify({"site", "id"}, right, {"site", "id"}, "common");
```

#### Expected output

```text
rows match on the composite site/id key
```

Example 5.

```cpp
auto out = left.unify("id", right, "id", "common", "_r", "one_to_one");
```

#### Expected output

```text
duplicate id values on either side cause a validation failure
```

## `combine`

### API Signature

```text
DataFrame::combine(other: const DataFrame&) -> DataFrame
DataFrame::combine(other: const DataFrame&, spec: const CombineSpec&) -> DataFrame
```

### Notes

Returns feature-wise composition or aligned update. Numeric bulk operations preserve the input Series platform.

### Inputs

`other: const DataFrame&`
    Second frame aligned or multiplied with the current frame.

`spec: const CombineSpec&`
    Typed composition policy.

### Output

`DataFrame` containing feature-wise composition or aligned update.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = left.combine(right);
```

#### Expected output

```text
right features are appended after aligned left features
```

Example 2.

```cpp
auto out = left.combine(right, {DataFrame::CombineOp::Features, "_r", true});
```

#### Expected output

```text
colliding right feature names receive _r
```

Example 3.

```cpp
auto out = left.combine(right, {DataFrame::CombineOp::Update, "_r", true});
```

#### Expected output

```text
observed right values overwrite aligned left values
```

Example 4.

```cpp
auto out = left.combine(right, {DataFrame::CombineOp::Update, "_r", false});
```

#### Expected output

```text
existing left observations are preserved
```

Example 5.

```cpp
auto out = left.join(right, "_rhs");
```

#### Expected output

```text
the feature-combine convenience path uses _rhs for collisions
```

## `group_by`

### API Signature

```text
DataFrame::group_by(keys: RangeView<string_view>, specs: RangeView<GroupSpec>, dropna_keys: bool = true, render: bool = true) -> DataFrameView
```

### Notes

Returns stable grouped reductions. Numeric bulk operations preserve the input Series platform.

### Inputs

`keys: RangeView<string_view>`
    Feature names whose distinct tuples define groups.

`specs: RangeView<GroupSpec>`
    Output/source/operation descriptions for grouped reductions.

`dropna_keys: bool = true`
    Excludes groups containing null keys when true.

`render: bool = true`
    When true, the returned view may auto-render in temporary terminal contexts.

### Output

`DataFrameView` containing stable grouped reductions.

### Options

- Group operations: Count, Nunique, Sum, Mean, Min, Max, Std, and Var.

### Examples

Example 1.

```cpp
auto out = sales.group_by({"region"}, {{"total", "amount", DataFrame::GroupOp::Sum}}, true, false);
```

#### Expected output

```text
one stable first-seen row per region with total amount
```

Example 2.

```cpp
auto out = sales.group_by({"region"}, {{"avg", "amount", DataFrame::GroupOp::Mean}}, true, false);
```

#### Expected output

```text
one mean amount is returned per region
```

Example 3.

```cpp
auto out = sales.group_by({"region", "year"}, {{"n", "", DataFrame::GroupOp::Count}}, true, false);
```

#### Expected output

```text
one row count is returned per region/year tuple
```

Example 4.

```cpp
auto out = sales.group_by({"region"}, {{"low", "amount", DataFrame::GroupOp::Min}, {"high", "amount", DataFrame::GroupOp::Max}}, true, false);
```

#### Expected output

```text
low and high are computed in one grouping pass
```

Example 5.

```cpp
auto out = sales.group_by({"region"}, {{"spread", "amount", DataFrame::GroupOp::Std}}, false, false);
```

#### Expected output

```text
null region keys participate and spread is sample standard deviation
```

## `lengthen`

### API Signature

```text
DataFrame::lengthen(id_features: RangeView<string_view> = {}, value_features: RangeView<string_view> = {}, feature_name: string = "feature", value_name: string = "value", fresh_cases: bool = true) -> DataFrame
```

### Notes

Returns wide-to-long typed reshape. Numeric bulk operations preserve the input Series platform.

### Inputs

`id_features: RangeView<string_view> = {}`
    Features repeated without conversion in long output.

`value_features: RangeView<string_view> = {}`
    Homogeneous features stacked into one typed value feature.

`feature_name: string = "feature"`
    Output name carrying original feature labels.

`value_name: string = "value"`
    Output name carrying stacked values.

`fresh_cases: bool = true`
    Creates contiguous positional cases when true.

### Output

`DataFrame` containing wide-to-long typed reshape.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = wide.lengthen({"id"}, {"q1", "q2"});
```

#### Expected output

```text
each id is repeated twice; feature identifies q1/q2; value contains stacked values
```

Example 2.

```cpp
auto out = wide.lengthen({}, {"q1", "q2"});
```

#### Expected output

```text
only feature and value are emitted
```

Example 3.

```cpp
auto out = wide.lengthen({"id"}, {"q1", "q2"}, "quarter", "sales");
```

#### Expected output

```text
output names are id, quarter, and sales
```

Example 4.

```cpp
auto out = wide.lengthen({"id"}, {});
```

#### Expected output

```text
all non-id homogeneous features are stacked
```

Example 5.

```cpp
auto out = wide.lengthen({"id"}, {"q1", "q2"}, "feature", "value", false);
```

#### Expected output

```text
original cases repeat once for every stacked feature
```

## `catalogue`

### API Signature

```text
DataFrame::catalogue(feature: string_view, ordered: bool = false, sort_levels: bool = false) -> CategoryEncoding
```

### Notes

Returns typed codes, levels, and ordered metadata. Numeric bulk operations preserve the input Series platform.

### Inputs

`feature: string_view`
    Target feature name.

`ordered: bool = false`
    Records whether category level order has semantic meaning.

`sort_levels: bool = false`
    Sorts levels before assigning codes when true.

### Output

`CategoryEncoding` containing typed codes, levels, and ordered metadata.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.catalogue("city");
```

#### Expected output

```text
out.codes uses first-seen integer levels and out.decode() reconstructs city
```

Example 2.

```cpp
auto out = df.catalogue("city", true);
```

#### Expected output

```text
out.ordered is true and first-seen level order is retained
```

Example 3.

```cpp
auto out = df.catalogue("city", false, true);
```

#### Expected output

```text
levels are sorted before integer codes are assigned
```

Example 4.

```cpp
auto decoded = df.catalogue("city").decode();
```

#### Expected output

```text
decoded values and missingness equal the original city feature
```

Example 5.

```cpp
auto out = df.catalogue("status");
```

#### Expected output

```text
missing source values have invalid codes and payload code -1
```

## `covariance`

### API Signature

```text
DataFrame::covariance(options: StatisticOptions = {}, render: bool = false) -> DataFrameView
```

### Notes

Returns pairwise covariance matrix. Numeric bulk operations preserve the input Series platform.

### Inputs

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

`render: bool = false`
    When true, the returned view may auto-render in temporary terminal contexts.

### Output

`DataFrameView` containing pairwise covariance matrix.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.covariance();
```

#### Expected output

```text
x/x = 1.666667; x/y = 3.333333
```

Example 2.

```cpp
auto out = df.covariance({.ddof=0});
```

#### Expected output

```text
x/x = 1.25; x/y = 2.5
```

Example 3.

```cpp
auto out = df.head(3, false).covariance();
```

#### Expected output

```text
x/x = 1.0; x/y = 2.0
```

Example 4.

```cpp
auto out = nullable.covariance({.ddof=1, .min_count=3});
```

#### Expected output

```text
pairs with fewer than three complete cases are NA
```

Example 5.

```cpp
auto out = df.covariance({}, true);
```

#### Expected output

```text
the same covariance values are returned and the view may auto-render
```

## `percentiles`

### API Signature

```text
DataFrame::percentiles(points: RangeView<double> = {0.25, 0.5, 0.75}, min_count: int64 = 1) -> DataFrame
```

### Notes

Returns requested distribution points. Numeric bulk operations preserve the input Series platform.

### Inputs

`points: RangeView<double> = {0.25, 0.5, 0.75}`
    Distribution points in the closed interval [0, 1].

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing requested distribution points.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.percentiles({0.0, 0.5, 1.0});
```

#### Expected output

```text
x = [1.0, 2.5, 4.0]
```

Example 2.

```cpp
auto out = df.percentiles();
```

#### Expected output

```text
x = [1.75, 2.5, 3.25]
```

Example 3.

```cpp
auto out = df.percentiles({0.25});
```

#### Expected output

```text
x = [1.75]
```

Example 4.

```cpp
auto out = nullable.percentiles({0.5});
```

#### Expected output

```text
x contains the median of observed values only
```

Example 5.

```cpp
auto out = df.percentiles({0.5}, 5);
```

#### Expected output

```text
x and y are NA because min_count is not met
```

## `most_frequent`

### API Signature

```text
DataFrame::most_frequent(drop_nulls: bool = true) -> DataFrame
```

### Notes

Returns all tied most-frequent values per feature. Numeric bulk operations preserve the input Series platform.

### Inputs

`drop_nulls: bool = true`
    Excludes missing values while selecting the most frequent value.

### Output

`DataFrame` containing all tied most-frequent values per feature.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = repeated.most_frequent();
```

#### Expected output

```text
x = [20.0]
```

Example 2.

```cpp
auto out = tied.most_frequent();
```

#### Expected output

```text
x = [1.0, 2.0]; both tied modes are returned
```

Example 3.

```cpp
auto out = notes.most_frequent();
```

#### Expected output

```text
the most frequent Note value is returned
```

Example 4.

```cpp
auto out = nullable.most_frequent(false);
```

#### Expected output

```text
null is included when its frequency ties the best observed frequency
```

Example 5.

```cpp
auto out = wide.most_frequent();
```

#### Expected output

```text
each supported feature is padded to the largest mode count
```

## `ranks`

### API Signature

```text
DataFrame::ranks(ties: RankTieRule = Average, nulls: RankNullRule = Keep, ascending: bool = true, percentage: bool = false) -> DataFrame
```

### Notes

Returns ranked numeric features. Numeric bulk operations preserve the input Series platform.

### Inputs

`ties: RankTieRule = Average`
    Rule used when two or more observed values are equal.

`nulls: RankNullRule = Keep`
    Placement/validity policy for null ranks.

`ascending: bool = true`
    Ranks smaller values first when true and larger values first when false.

`percentage: bool = false`
    Divides ranks by the number of ranked observations when true.

### Output

`DataFrame` containing ranked numeric features.

### Options

- `Average`, `Lowest`, `Highest`, `First`, and `Dense` choose tie ranks.
- `Keep`, `Top`, and `Bottom` choose null rank behavior.

### Examples

Example 1.

```cpp
auto out = repeated.ranks();
```

#### Expected output

```text
x = [2.5, 1.0, 2.5]
```

Example 2.

```cpp
auto out = repeated.ranks(RankTieRule::Lowest);
```

#### Expected output

```text
x = [2.0, 1.0, 2.0]
```

Example 3.

```cpp
auto out = repeated.ranks(RankTieRule::Highest);
```

#### Expected output

```text
x = [3.0, 1.0, 3.0]
```

Example 4.

```cpp
auto out = repeated.ranks(RankTieRule::Dense);
```

#### Expected output

```text
x = [2.0, 1.0, 2.0]
```

Example 5.

```cpp
auto out = repeated.ranks(RankTieRule::Average, RankNullRule::Keep, true, true);
```

#### Expected output

```text
x = [0.833333, 0.333333, 0.833333]
```

## `asymmetry`

### API Signature

```text
DataFrame::asymmetry(axis: int64 = 0, min_count: int64 = 3) -> Thread
```

### Notes

Returns bias-corrected skewness. Numeric bulk operations preserve the input Series platform.

### Inputs

`axis: int64 = 0`
    Use 0 for one result per numeric feature and 1 for one result per case.

`min_count: int64 = 3`
    Minimum observed values required for a valid output.

### Output

`Thread` containing bias-corrected skewness.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.asymmetry();
```

#### Expected output

```text
x = 0.0; y = 0.0
```

Example 2.

```cpp
auto out = small.asymmetry();
```

#### Expected output

```text
output uses the same documented contract on a one-feature frame
```

Example 3.

```cpp
auto out = wide.asymmetry();
```

#### Expected output

```text
all compatible numeric features participate
```

Example 4.

```cpp
auto out = nullable.asymmetry();
```

#### Expected output

```text
validity reflects the method's null and minimum-count policy
```

Example 5.

```cpp
auto out = df.head(3, false).asymmetry();
```

#### Expected output

```text
output is computed from the selected three cases only
```

## `tail_weight`

### API Signature

```text
DataFrame::tail_weight(axis: int64 = 0, min_count: int64 = 4) -> Thread
```

### Notes

Returns bias-corrected Fisher excess kurtosis. Numeric bulk operations preserve the input Series platform.

### Inputs

`axis: int64 = 0`
    Use 0 for one result per numeric feature and 1 for one result per case.

`min_count: int64 = 4`
    Minimum observed values required for a valid output.

### Output

`Thread` containing bias-corrected Fisher excess kurtosis.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.tail_weight();
```

#### Expected output

```text
x = -1.2; y = -1.2
```

Example 2.

```cpp
auto out = small.tail_weight();
```

#### Expected output

```text
output uses the same documented contract on a one-feature frame
```

Example 3.

```cpp
auto out = wide.tail_weight();
```

#### Expected output

```text
all compatible numeric features participate
```

Example 4.

```cpp
auto out = nullable.tail_weight();
```

#### Expected output

```text
validity reflects the method's null and minimum-count policy
```

Example 5.

```cpp
auto out = df.head(3, false).tail_weight();
```

#### Expected output

```text
output is computed from the selected three cases only
```

## `variance`

### API Signature

```text
DataFrame::variance(axis: int64 = 0, options: StatisticOptions = {}) -> Thread
```

### Notes

Returns variance with configurable denominator and validity. Numeric bulk operations preserve the input Series platform.

### Inputs

`axis: int64 = 0`
    Use 0 for one result per numeric feature and 1 for one result per case.

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`Thread` containing variance with configurable denominator and validity.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.variance();
```

#### Expected output

```text
x = 1.666667; y = 6.666667
```

Example 2.

```cpp
auto out = small.variance();
```

#### Expected output

```text
output uses the same documented contract on a one-feature frame
```

Example 3.

```cpp
auto out = wide.variance();
```

#### Expected output

```text
all compatible numeric features participate
```

Example 4.

```cpp
auto out = nullable.variance();
```

#### Expected output

```text
validity reflects the method's null and minimum-count policy
```

Example 5.

```cpp
auto out = df.head(3, false).variance();
```

#### Expected output

```text
output is computed from the selected three cases only
```

## `deviation`

### API Signature

```text
DataFrame::deviation(axis: int64 = 0, options: StatisticOptions = {}) -> Thread
```

### Notes

Returns standard deviation with configurable denominator and validity. Numeric bulk operations preserve the input Series platform.

### Inputs

`axis: int64 = 0`
    Use 0 for one result per numeric feature and 1 for one result per case.

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`Thread` containing standard deviation with configurable denominator and validity.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.deviation();
```

#### Expected output

```text
x = 1.290994; y = 2.581989
```

Example 2.

```cpp
auto out = small.deviation();
```

#### Expected output

```text
output uses the same documented contract on a one-feature frame
```

Example 3.

```cpp
auto out = wide.deviation();
```

#### Expected output

```text
all compatible numeric features participate
```

Example 4.

```cpp
auto out = nullable.deviation();
```

#### Expected output

```text
validity reflects the method's null and minimum-count policy
```

Example 5.

```cpp
auto out = df.head(3, false).deviation();
```

#### Expected output

```text
output is computed from the selected three cases only
```

## `mean_error`

### API Signature

```text
DataFrame::mean_error(axis: int64 = 0, options: StatisticOptions = {}) -> Thread
```

### Notes

Returns standard error of the mean. Numeric bulk operations preserve the input Series platform.

### Inputs

`axis: int64 = 0`
    Use 0 for one result per numeric feature and 1 for one result per case.

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`Thread` containing standard error of the mean.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.mean_error();
```

#### Expected output

```text
x = 0.645497; y = 1.290994
```

Example 2.

```cpp
auto out = small.mean_error();
```

#### Expected output

```text
output uses the same documented contract on a one-feature frame
```

Example 3.

```cpp
auto out = wide.mean_error();
```

#### Expected output

```text
all compatible numeric features participate
```

Example 4.

```cpp
auto out = nullable.mean_error();
```

#### Expected output

```text
validity reflects the method's null and minimum-count policy
```

Example 5.

```cpp
auto out = df.head(3, false).mean_error();
```

#### Expected output

```text
output is computed from the selected three cases only
```

## `running_sum`

### API Signature

```text
DataFrame::running_sum(skip_nulls: bool = true) -> DataFrame
```

### Notes

Returns cumulative sums. Numeric bulk operations preserve the input Series platform.

### Inputs

`skip_nulls: bool = true`
    When false, cumulative validity stops at the first null.

### Output

`DataFrame` containing cumulative sums.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.running_sum();
```

#### Expected output

```text
x = [1.0, 3.0, 6.0, 10.0]
```

Example 2.

```cpp
auto out = nullable.running_sum(true);
```

#### Expected output

```text
x = [1.0, NA, 4.0, 8.0]
```

Example 3.

```cpp
auto out = nullable.running_sum(false);
```

#### Expected output

```text
x = [1.0, NA, NA, NA]
```

Example 4.

```cpp
auto out = wide.running_sum();
```

#### Expected output

```text
every numeric feature is processed independently
```

Example 5.

```cpp
auto out = df.head(1, false).running_sum();
```

#### Expected output

```text
one observed result is returned per numeric feature
```

## `running_product`

### API Signature

```text
DataFrame::running_product(skip_nulls: bool = true) -> DataFrame
```

### Notes

Returns cumulative products. Numeric bulk operations preserve the input Series platform.

### Inputs

`skip_nulls: bool = true`
    When false, cumulative validity stops at the first null.

### Output

`DataFrame` containing cumulative products.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.running_product();
```

#### Expected output

```text
x = [1.0, 2.0, 6.0, 24.0]
```

Example 2.

```cpp
auto out = nullable.running_product(true);
```

#### Expected output

```text
x = [1.0, NA, 3.0, 12.0]
```

Example 3.

```cpp
auto out = nullable.running_product(false);
```

#### Expected output

```text
x = [1.0, NA, NA, NA]
```

Example 4.

```cpp
auto out = wide.running_product();
```

#### Expected output

```text
every numeric feature is processed independently
```

Example 5.

```cpp
auto out = df.head(1, false).running_product();
```

#### Expected output

```text
one observed result is returned per numeric feature
```

## `running_minimum`

### API Signature

```text
DataFrame::running_minimum(skip_nulls: bool = true) -> DataFrame
```

### Notes

Returns cumulative minima. Numeric bulk operations preserve the input Series platform.

### Inputs

`skip_nulls: bool = true`
    When false, cumulative validity stops at the first null.

### Output

`DataFrame` containing cumulative minima.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = reversed.running_minimum();
```

#### Expected output

```text
x = [4.0, 3.0, 2.0, 1.0]
```

Example 2.

```cpp
auto out = nullable.running_minimum(true);
```

#### Expected output

```text
x = [1.0, NA, 1.0, 1.0]
```

Example 3.

```cpp
auto out = nullable.running_minimum(false);
```

#### Expected output

```text
x = [1.0, NA, NA, NA]
```

Example 4.

```cpp
auto out = wide.running_minimum();
```

#### Expected output

```text
every numeric feature is processed independently
```

Example 5.

```cpp
auto out = df.head(1, false).running_minimum();
```

#### Expected output

```text
one observed result is returned per numeric feature
```

## `running_maximum`

### API Signature

```text
DataFrame::running_maximum(skip_nulls: bool = true) -> DataFrame
```

### Notes

Returns cumulative maxima. Numeric bulk operations preserve the input Series platform.

### Inputs

`skip_nulls: bool = true`
    When false, cumulative validity stops at the first null.

### Output

`DataFrame` containing cumulative maxima.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.running_maximum();
```

#### Expected output

```text
x = [1.0, 2.0, 3.0, 4.0]
```

Example 2.

```cpp
auto out = nullable.running_maximum(true);
```

#### Expected output

```text
x = [1.0, NA, 3.0, 4.0]
```

Example 3.

```cpp
auto out = nullable.running_maximum(false);
```

#### Expected output

```text
x = [1.0, NA, NA, NA]
```

Example 4.

```cpp
auto out = wide.running_maximum();
```

#### Expected output

```text
every numeric feature is processed independently
```

Example 5.

```cpp
auto out = df.head(1, false).running_maximum();
```

#### Expected output

```text
one observed result is returned per numeric feature
```

## `correlate_with`

### API Signature

```text
DataFrame::correlate_with(other: const DataFrame&, method: CorrMethod = Pearson, min_count: int64 = 1) -> Thread
```

### Notes

Returns one aligned correlation per common numeric feature. Numeric bulk operations preserve the input Series platform.

### Inputs

`other: const DataFrame&`
    Second frame aligned or multiplied with the current frame.

`method: CorrMethod = Pearson`
    Correlation rule: Pearson, Spearman, or KendallTau.

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`Thread` containing one aligned correlation per common numeric feature.

### Options

- `Pearson`, `Spearman`, and `KendallTau` select the correlation statistic.

### Examples

Example 1.

```cpp
auto out = df.correlate_with(df);
```

#### Expected output

```text
x = 1.0; y = 1.0
```

Example 2.

```cpp
auto repeated = df.correlate_with(df);
```

#### Expected output

```text
x = 1.0; y = 1.0
```

Example 3.

```cpp
auto out = df.correlate_with(df);
```

#### Expected output

```text
x = 1.0; y = 1.0
```

Example 4.

```cpp
auto out = df.correlate_with(df);
```

#### Expected output

```text
x = 1.0; y = 1.0
```

Example 5.

```cpp
auto out = df.correlate_with(df);
```

#### Expected output

```text
x = 1.0; y = 1.0
```

## `matrix_product`

### API Signature

```text
DataFrame::matrix_product(other: const DataFrame&) -> DataFrame
```

### Notes

Returns matrix multiplication result. Numeric bulk operations preserve the input Series platform.

### Inputs

`other: const DataFrame&`
    Second frame aligned or multiplied with the current frame.

### Output

`DataFrame` containing matrix multiplication result.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = left.matrix_product(right);
```

#### Expected output

```text
z = [17.0, 39.0]
```

Example 2.

```cpp
auto repeated = left.matrix_product(right);
```

#### Expected output

```text
z = [17.0, 39.0]
```

Example 3.

```cpp
auto out = left.matrix_product(right);
```

#### Expected output

```text
z = [17.0, 39.0]
```

Example 4.

```cpp
auto out = left.matrix_product(right);
```

#### Expected output

```text
z = [17.0, 39.0]
```

Example 5.

```cpp
auto out = left.matrix_product(right);
```

#### Expected output

```text
z = [17.0, 39.0]
```

## `moving_sum`

### API Signature

```text
DataFrame::moving_sum(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving sums. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving sums.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_sum(2);
```

#### Expected output

```text
x = [1.0, 3.0, 5.0, 7.0]
```

Example 2.

```cpp
auto out = df.moving_sum(3, {.min_count=1});
```

#### Expected output

```text
the first 0 x results are NA
```

Example 3.

```cpp
auto out = df.moving_sum(1, {.min_count=1, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_sum(3, {.min_count=1, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_sum(1, {.min_count=1});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_mean`

### API Signature

```text
DataFrame::moving_mean(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving means. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving means.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_mean(2);
```

#### Expected output

```text
x = [1.0, 1.5, 2.5, 3.5]
```

Example 2.

```cpp
auto out = df.moving_mean(3, {.min_count=1});
```

#### Expected output

```text
the first 0 x results are NA
```

Example 3.

```cpp
auto out = df.moving_mean(1, {.min_count=1, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_mean(3, {.min_count=1, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_mean(1, {.min_count=1});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_minimum`

### API Signature

```text
DataFrame::moving_minimum(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving minima. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving minima.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = reversed.moving_minimum(2);
```

#### Expected output

```text
x = [4.0, 3.0, 2.0, 1.0]
```

Example 2.

```cpp
auto out = df.moving_minimum(3, {.min_count=1});
```

#### Expected output

```text
the first 0 x results are NA
```

Example 3.

```cpp
auto out = df.moving_minimum(1, {.min_count=1, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_minimum(3, {.min_count=1, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_minimum(1, {.min_count=1});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_maximum`

### API Signature

```text
DataFrame::moving_maximum(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving maxima. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving maxima.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_maximum(2);
```

#### Expected output

```text
x = [1.0, 2.0, 3.0, 4.0]
```

Example 2.

```cpp
auto out = df.moving_maximum(3, {.min_count=1});
```

#### Expected output

```text
the first 0 x results are NA
```

Example 3.

```cpp
auto out = df.moving_maximum(1, {.min_count=1, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_maximum(3, {.min_count=1, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_maximum(1, {.min_count=1});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_variance`

### API Signature

```text
DataFrame::moving_variance(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving variance. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving variance.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_variance(2, {.min_count=2, .ddof=1});
```

#### Expected output

```text
x = [NA, 0.5, 0.5, 0.5]
```

Example 2.

```cpp
auto out = df.moving_variance(3, {.min_count=2});
```

#### Expected output

```text
the first 1 x results are NA
```

Example 3.

```cpp
auto out = df.moving_variance(2, {.min_count=2, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_variance(3, {.min_count=2, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_variance(2, {.min_count=2});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_deviation`

### API Signature

```text
DataFrame::moving_deviation(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving standard deviation. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving standard deviation.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_deviation(2, {.min_count=2, .ddof=1});
```

#### Expected output

```text
x = [NA, 0.707107, 0.707107, 0.707107]
```

Example 2.

```cpp
auto out = df.moving_deviation(3, {.min_count=2});
```

#### Expected output

```text
the first 1 x results are NA
```

Example 3.

```cpp
auto out = df.moving_deviation(2, {.min_count=2, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_deviation(3, {.min_count=2, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_deviation(2, {.min_count=2});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_mean_error`

### API Signature

```text
DataFrame::moving_mean_error(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving standard error. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving standard error.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_mean_error(2, {.min_count=2, .ddof=1});
```

#### Expected output

```text
x = [NA, 0.5, 0.5, 0.5]
```

Example 2.

```cpp
auto out = df.moving_mean_error(3, {.min_count=2});
```

#### Expected output

```text
the first 1 x results are NA
```

Example 3.

```cpp
auto out = df.moving_mean_error(2, {.min_count=2, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_mean_error(3, {.min_count=2, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_mean_error(2, {.min_count=2});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_asymmetry`

### API Signature

```text
DataFrame::moving_asymmetry(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving bias-corrected skewness. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving bias-corrected skewness.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_asymmetry(3, {.min_count=3});
```

#### Expected output

```text
x = [NA, NA, 0.0, 0.0]
```

Example 2.

```cpp
auto out = df.moving_asymmetry(3, {.min_count=3});
```

#### Expected output

```text
the first 2 x results are NA
```

Example 3.

```cpp
auto out = df.moving_asymmetry(3, {.min_count=3, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_asymmetry(3, {.min_count=3, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_asymmetry(3, {.min_count=3});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `moving_tail_weight`

### API Signature

```text
DataFrame::moving_tail_weight(window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns fixed-width moving bias-corrected excess kurtosis. Numeric bulk operations preserve the input Series platform.

### Inputs

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing fixed-width moving bias-corrected excess kurtosis.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.moving_tail_weight(4, {.min_count=4});
```

#### Expected output

```text
x = [NA, NA, NA, -1.2]
```

Example 2.

```cpp
auto out = df.moving_tail_weight(4, {.min_count=4});
```

#### Expected output

```text
the first 3 x results are NA
```

Example 3.

```cpp
auto out = df.moving_tail_weight(4, {.min_count=4, .ddof=0});
```

#### Expected output

```text
uses a population denominator where variance participates
```

Example 4.

```cpp
auto out = df.moving_tail_weight(4, {.min_count=4, .centered=true});
```

#### Expected output

```text
results are placed at the center case
```

Example 5.

```cpp
auto out = nullable.moving_tail_weight(4, {.min_count=4});
```

#### Expected output

```text
nulls are skipped and validity follows min_count
```

## `weighted_moving`

### API Signature

```text
DataFrame::weighted_moving(weights: RangeView<double>, statistic: WindowStatistic = Mean, min_count: int64 = 1) -> DataFrame
```

### Notes

Returns weighted moving statistics. Numeric bulk operations preserve the input Series platform.

### Inputs

`weights: RangeView<double>`
    Non-empty numeric weights ordered from the oldest to newest case in each window.

`statistic: WindowStatistic = Mean`
    Enum selecting the statistic computed in every group or time window.

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing weighted moving statistics.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.weighted_moving({1.0, 2.0});
```

#### Expected output

```text
x = [1.0, 1.666667, 2.666667, 3.666667]
```

Example 2.

```cpp
auto out = df.weighted_moving({1.0, 1.0}, WindowStatistic::Sum);
```

#### Expected output

```text
x = [1.0, 3.0, 5.0, 7.0]
```

Example 3.

```cpp
auto out = df.weighted_moving({1.0, 2.0}, WindowStatistic::Variance);
```

#### Expected output

```text
returns weighted population variance per trailing window
```

Example 4.

```cpp
auto out = df.weighted_moving({1.0, 2.0}, WindowStatistic::Deviation);
```

#### Expected output

```text
each output is the square root of weighted variance
```

Example 5.

```cpp
auto out = nullable.weighted_moving({1.0, 2.0}, WindowStatistic::Mean, 2);
```

#### Expected output

```text
a result is NA until two observed values carry weight
```

## `indexed_moving`

### API Signature

```text
DataFrame::indexed_moving(bounds: WindowBoundsFn, statistic: WindowStatistic = Mean, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns moving statistics over caller-defined bounds. Numeric bulk operations preserve the input Series platform.

### Inputs

`bounds: WindowBoundsFn`
    Callback returning one start and one exclusive end position per output case.

`statistic: WindowStatistic = Mean`
    Enum selecting the statistic computed in every group or time window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing moving statistics over caller-defined bounds.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = df.indexed_moving(expanding_bounds);
```

#### Expected output

```text
x = [1.0, 1.5, 2.0, 2.5]
```

Example 2.

```cpp
auto out = df.indexed_moving(expanding_bounds, WindowStatistic::Sum);
```

#### Expected output

```text
x = [1.0, 3.0, 6.0, 10.0]
```

Example 3.

```cpp
auto out = df.indexed_moving(pair_bounds, WindowStatistic::Minimum);
```

#### Expected output

```text
each result is the minimum inside its caller-defined interval
```

Example 4.

```cpp
auto out = df.indexed_moving(pair_bounds, WindowStatistic::Maximum);
```

#### Expected output

```text
each result is the maximum inside its caller-defined interval
```

Example 5.

```cpp
auto out = nullable.indexed_moving(expanding_bounds, WindowStatistic::Mean, {.min_count=3});
```

#### Expected output

```text
results are NA until three observed values are inside the custom bound
```

## `growing_sum`

### API Signature

```text
DataFrame::growing_sum(min_count: int64 = 1) -> DataFrame
```

### Notes

Returns expanding sums. Numeric bulk operations preserve the input Series platform.

### Inputs

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing expanding sums.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.growing_sum();
```

#### Expected output

```text
x = [1.0, 3.0, 6.0, 10.0]
```

Example 2.

```cpp
auto out = df.growing_sum(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_sum();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_sum();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_sum();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_mean`

### API Signature

```text
DataFrame::growing_mean(min_count: int64 = 1) -> DataFrame
```

### Notes

Returns expanding means. Numeric bulk operations preserve the input Series platform.

### Inputs

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing expanding means.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.growing_mean();
```

#### Expected output

```text
x = [1.0, 1.5, 2.0, 2.5]
```

Example 2.

```cpp
auto out = df.growing_mean(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_mean();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_mean();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_mean();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_minimum`

### API Signature

```text
DataFrame::growing_minimum(min_count: int64 = 1) -> DataFrame
```

### Notes

Returns expanding minima. Numeric bulk operations preserve the input Series platform.

### Inputs

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing expanding minima.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = reversed.growing_minimum();
```

#### Expected output

```text
x = [4.0, 3.0, 2.0, 1.0]
```

Example 2.

```cpp
auto out = df.growing_minimum(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_minimum();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_minimum();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_minimum();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_maximum`

### API Signature

```text
DataFrame::growing_maximum(min_count: int64 = 1) -> DataFrame
```

### Notes

Returns expanding maxima. Numeric bulk operations preserve the input Series platform.

### Inputs

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing expanding maxima.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.growing_maximum();
```

#### Expected output

```text
x = [1.0, 2.0, 3.0, 4.0]
```

Example 2.

```cpp
auto out = df.growing_maximum(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_maximum();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_maximum();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_maximum();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_variance`

### API Signature

```text
DataFrame::growing_variance(options: StatisticOptions = {}) -> DataFrame
```

### Notes

Returns expanding variance. Numeric bulk operations preserve the input Series platform.

### Inputs

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing expanding variance.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.growing_variance({.ddof=1, .min_count=2});
```

#### Expected output

```text
x = [NA, 0.5, 1.0, 1.666667]
```

Example 2.

```cpp
auto out = df.growing_variance({.ddof=0, .min_count=1});
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_variance();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_variance();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_variance();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_deviation`

### API Signature

```text
DataFrame::growing_deviation(options: StatisticOptions = {}) -> DataFrame
```

### Notes

Returns expanding standard deviation. Numeric bulk operations preserve the input Series platform.

### Inputs

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing expanding standard deviation.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.growing_deviation({.ddof=1, .min_count=2});
```

#### Expected output

```text
x = [NA, 0.707107, 1.0, 1.290994]
```

Example 2.

```cpp
auto out = df.growing_deviation({.ddof=0, .min_count=1});
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_deviation();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_deviation();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_deviation();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_mean_error`

### API Signature

```text
DataFrame::growing_mean_error(options: StatisticOptions = {}) -> DataFrame
```

### Notes

Returns expanding standard error. Numeric bulk operations preserve the input Series platform.

### Inputs

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing expanding standard error.

### Options

- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.

### Examples

Example 1.

```cpp
auto out = df.growing_mean_error({.ddof=1, .min_count=2});
```

#### Expected output

```text
x = [NA, 0.5, 0.577350, 0.645497]
```

Example 2.

```cpp
auto out = df.growing_mean_error(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_mean_error();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_mean_error();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_mean_error();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_asymmetry`

### API Signature

```text
DataFrame::growing_asymmetry(min_count: int64 = 3) -> DataFrame
```

### Notes

Returns expanding bias-corrected skewness. Numeric bulk operations preserve the input Series platform.

### Inputs

`min_count: int64 = 3`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing expanding bias-corrected skewness.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.growing_asymmetry();
```

#### Expected output

```text
x = [NA, NA, 0.0, 0.0]
```

Example 2.

```cpp
auto out = df.growing_asymmetry(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_asymmetry();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_asymmetry();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_asymmetry();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `growing_tail_weight`

### API Signature

```text
DataFrame::growing_tail_weight(min_count: int64 = 4) -> DataFrame
```

### Notes

Returns expanding bias-corrected excess kurtosis. Numeric bulk operations preserve the input Series platform.

### Inputs

`min_count: int64 = 4`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing expanding bias-corrected excess kurtosis.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = df.growing_tail_weight();
```

#### Expected output

```text
x = [NA, NA, NA, -1.2]
```

Example 2.

```cpp
auto out = df.growing_tail_weight(2);
```

#### Expected output

```text
the first result is NA when the configured minimum is not met
```

Example 3.

```cpp
auto out = df.head(1, false).growing_tail_weight();
```

#### Expected output

```text
one output case is returned
```

Example 4.

```cpp
auto out = nullable.growing_tail_weight();
```

#### Expected output

```text
missing input is excluded from later growing results
```

Example 5.

```cpp
auto out = wide.growing_tail_weight();
```

#### Expected output

```text
every numeric feature is processed independently
```

## `exponential_mean`

### API Signature

```text
DataFrame::exponential_mean(alpha: double, adjusted: bool = true, min_count: int64 = 1, ignore_nulls: bool = false) -> DataFrame
```

### Notes

Returns exponentially weighted means. Numeric bulk operations preserve the input Series platform.

### Inputs

`alpha: double`
    Smoothing factor in (0, 1]; larger values emphasize recent cases.

`adjusted: bool = true`
    Normalizes finite geometric weights when true; uses recursive smoothing when false.

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

`ignore_nulls: bool = false`
    When true, missing cases do not advance exponential weight positions.

### Output

`DataFrame` containing exponentially weighted means.

### Options

- `adjusted=true` normalizes geometric weights; `false` uses recursion.
- Recursive (`adjusted=false`) execution currently requires a non-nullable numeric Thread.
- `ignore_nulls=true` makes missing cases weight-neutral.

### Examples

Example 1.

```cpp
auto out = df.exponential_mean(0.5);
```

#### Expected output

```text
x = [1.0, 1.666667, 2.428571, 3.266667]
```

Example 2.

```cpp
auto out = df.exponential_mean(0.2);
```

#### Expected output

```text
recent cases receive smoothing weight 0.2
```

Example 3.

```cpp
auto out = df.exponential_mean(0.8);
```

#### Expected output

```text
recent cases receive smoothing weight 0.8
```

Example 4.

```cpp
auto out = df.exponential_mean(1.0);
```

#### Expected output

```text
the current observed value receives all weight
```

Example 5.

```cpp
auto out = nullable.exponential_mean(0.5);
```

#### Expected output

```text
normalization excludes missing observations
```

## `exponential_variance`

### API Signature

```text
DataFrame::exponential_variance(alpha: double, adjusted: bool = true, bias_corrected: bool = true, min_count: int64 = 1, ignore_nulls: bool = false) -> DataFrame
```

### Notes

Returns exponentially weighted variance. Numeric bulk operations preserve the input Series platform.

### Inputs

`alpha: double`
    Smoothing factor in (0, 1]; larger values emphasize recent cases.

`adjusted: bool = true`
    Normalizes finite geometric weights when true; uses recursive smoothing when false.

`bias_corrected: bool = true`
    Applies finite-sample variance correction when true.

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

`ignore_nulls: bool = false`
    When true, missing cases do not advance exponential weight positions.

### Output

`DataFrame` containing exponentially weighted variance.

### Options

- `adjusted=true` normalizes geometric weights; `false` uses recursion.
- Recursive (`adjusted=false`) execution currently requires a non-nullable numeric Thread.
- `bias_corrected=true` applies finite-sample correction.
- `ignore_nulls=true` makes missing cases weight-neutral.

### Examples

Example 1.

```cpp
auto out = df.exponential_variance(0.5, true, false);
```

#### Expected output

```text
returns four non-negative variance values per numeric feature
```

Example 2.

```cpp
auto out = df.exponential_variance(0.2);
```

#### Expected output

```text
recent cases receive smoothing weight 0.2
```

Example 3.

```cpp
auto out = df.exponential_variance(0.8);
```

#### Expected output

```text
recent cases receive smoothing weight 0.8
```

Example 4.

```cpp
auto out = df.exponential_variance(1.0);
```

#### Expected output

```text
the current observed value receives all weight
```

Example 5.

```cpp
auto out = nullable.exponential_variance(0.5);
```

#### Expected output

```text
normalization excludes missing observations
```

## `exponential_deviation`

### API Signature

```text
DataFrame::exponential_deviation(alpha: double, adjusted: bool = true, bias_corrected: bool = true, min_count: int64 = 1, ignore_nulls: bool = false) -> DataFrame
```

### Notes

Returns exponentially weighted standard deviation. Numeric bulk operations preserve the input Series platform.

### Inputs

`alpha: double`
    Smoothing factor in (0, 1]; larger values emphasize recent cases.

`adjusted: bool = true`
    Normalizes finite geometric weights when true; uses recursive smoothing when false.

`bias_corrected: bool = true`
    Applies finite-sample variance correction when true.

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

`ignore_nulls: bool = false`
    When true, missing cases do not advance exponential weight positions.

### Output

`DataFrame` containing exponentially weighted standard deviation.

### Options

- `adjusted=true` normalizes geometric weights; `false` uses recursion.
- Recursive (`adjusted=false`) execution currently requires a non-nullable numeric Thread.
- `bias_corrected=true` applies finite-sample correction.
- `ignore_nulls=true` makes missing cases weight-neutral.

### Examples

Example 1.

```cpp
auto out = df.exponential_deviation(0.5, true, false);
```

#### Expected output

```text
each value is the square root of exponential_variance
```

Example 2.

```cpp
auto out = df.exponential_deviation(0.2);
```

#### Expected output

```text
recent cases receive smoothing weight 0.2
```

Example 3.

```cpp
auto out = df.exponential_deviation(0.8);
```

#### Expected output

```text
recent cases receive smoothing weight 0.8
```

Example 4.

```cpp
auto out = df.exponential_deviation(1.0);
```

#### Expected output

```text
the current observed value receives all weight
```

Example 5.

```cpp
auto out = nullable.exponential_deviation(0.5);
```

#### Expected output

```text
normalization excludes missing observations
```

## `grouped_moving_mean`

### API Signature

```text
DataFrame::grouped_moving_mean(group_feature: string_view, window: int64, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns moving means computed independently per group. Numeric bulk operations preserve the input Series platform.

### Inputs

`group_feature: string_view`
    Feature whose equal values define independent moving-window groups.

`window: int64`
    Positive fixed number of cases in each window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing moving means computed independently per group.

### Options

- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = grouped.grouped_moving_mean("group", 2);
```

#### Expected output

```text
value = [1.0, 10.0, 2.0, 15.0]
```

Example 2.

```cpp
auto out = grouped.grouped_moving_mean("group", 1);
```

#### Expected output

```text
value = [1.0, 10.0, 3.0, 20.0]
```

Example 3.

```cpp
auto out = grouped.grouped_moving_mean("group", 2, {.min_count=2});
```

#### Expected output

```text
value = [NA, NA, 2.0, 15.0]
```

Example 4.

```cpp
auto out = grouped.grouped_moving_mean("group", 3);
```

#### Expected output

```text
each group uses up to its last three observations
```

Example 5.

```cpp
auto out = grouped_nullable.grouped_moving_mean("group", 2);
```

#### Expected output

```text
null values are excluded independently inside each group
```

## `grouped_moving`

### API Signature

```text
DataFrame::grouped_moving(group_feature: string_view, window: int64, statistic: WindowStatistic = Mean, options: WindowOptions = {}) -> DataFrame
```

### Notes

Returns selected moving statistic computed independently per group. Numeric bulk operations preserve the input Series platform.

### Inputs

`group_feature: string_view`
    Feature whose equal values define independent moving-window groups.

`window: int64`
    Positive fixed number of cases in each window.

`statistic: WindowStatistic = Mean`
    Enum selecting the statistic computed in every group or time window.

`options: WindowOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

### Output

`DataFrame` containing selected moving statistic computed independently per group.

### Options

- Window statistics: Sum, Mean, Minimum, Maximum, Variance, and Deviation.
- `min_count`: minimum observed cases in each window.
- `ddof`: variance denominator correction.
- `centered=false`: trailing placement; `true`: centered placement.
- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.

### Examples

Example 1.

```cpp
auto out = grouped.grouped_moving("group", 2, WindowStatistic::Sum);
```

#### Expected output

```text
value = [1.0, 10.0, 4.0, 30.0]
```

Example 2.

```cpp
auto out = grouped.grouped_moving("group", 2, WindowStatistic::Mean);
```

#### Expected output

```text
value = [1.0, 10.0, 2.0, 15.0]
```

Example 3.

```cpp
auto out = grouped.grouped_moving("group", 2, WindowStatistic::Minimum);
```

#### Expected output

```text
value = [1.0, 10.0, 1.0, 10.0]
```

Example 4.

```cpp
auto out = grouped.grouped_moving("group", 2, WindowStatistic::Maximum);
```

#### Expected output

```text
value = [1.0, 10.0, 3.0, 20.0]
```

Example 5.

```cpp
auto out = grouped.grouped_moving("group", 2, WindowStatistic::Variance, {.min_count=2, .ddof=1});
```

#### Expected output

```text
value = [NA, NA, 2.0, 50.0]
```

## `time_moving_mean`

### API Signature

```text
DataFrame::time_moving_mean(time_feature: string_view, duration_nanos: int64, min_count: int64 = 1) -> DataFrame
```

### Notes

Returns moving means over an open-left, closed-right time interval. Numeric bulk operations preserve the input Series platform.

### Inputs

`time_feature: string_view`
    Monotonic Timeline, TimeIndex, or Int64 nanosecond feature.

`duration_nanos: int64`
    Positive time width in nanoseconds; the left boundary is open.

`min_count: int64 = 1`
    Minimum observed values required for a valid output.

### Output

`DataFrame` containing moving means over an open-left, closed-right time interval.

### Options

No enum or policy object is required for this method.

### Examples

Example 1.

```cpp
auto out = timed.time_moving_mean("time", 3);
```

#### Expected output

```text
value = [1.0, 1.5, 2.5, 3.5]
```

Example 2.

```cpp
auto out = timed.time_moving_mean("time", 1);
```

#### Expected output

```text
value = [1.0, 2.0, 3.0, 4.0]
```

Example 3.

```cpp
auto out = timed.time_moving_mean("time", 4);
```

#### Expected output

```text
value = [1.0, 1.5, 2.0, 3.0]
```

Example 4.

```cpp
auto out = timed.time_moving_mean("time", 3, 2);
```

#### Expected output

```text
value = [NA, 1.5, 2.5, 3.5]
```

Example 5.

```cpp
auto out = timed_nullable.time_moving_mean("time", 3);
```

#### Expected output

```text
null values are excluded while time boundaries remain unchanged
```

## `time_moving`

### API Signature

```text
DataFrame::time_moving(time_feature: string_view, duration_nanos: int64, statistic: TimeWindowStatistic = Mean, options: StatisticOptions = {}, closure: WindowClosure = Right) -> DataFrame
```

### Notes

Returns selected statistic over a configurable time interval. Numeric bulk operations preserve the input Series platform.

### Inputs

`time_feature: string_view`
    Monotonic Timeline, TimeIndex, or Int64 nanosecond feature.

`duration_nanos: int64`
    Positive time width in nanoseconds; the left boundary is open.

`statistic: TimeWindowStatistic = Mean`
    Enum selecting the statistic computed in every group or time window.

`options: StatisticOptions = {}`
    Typed statistic or window policy. The applicable fields are listed under Options.

`closure: WindowClosure = Right`
    Typed rule selecting open or closed left and right window boundaries.

### Output

`DataFrame` containing selected statistic over a configurable time interval.

### Options

- Time-window statistics: Sum, Mean, Variance, and Deviation.
- `ddof=1`: sample denominator; `ddof=0`: population denominator.
- `min_count=1`: require at least one observed value; increase it for stricter validity.
- `Right`, `Left`, `Both`, and `Neither` select interval closure.

### Examples

Example 1.

```cpp
auto out = timed.time_moving("time", 3, TimeWindowStatistic::Variance, {.ddof=1, .min_count=2});
```

#### Expected output

```text
value = [NA, 0.5, 0.5, 0.5]
```

Example 2.

```cpp
auto out = timed.time_moving("time", 3, TimeWindowStatistic::Sum);
```

#### Expected output

```text
value = [1.0, 3.0, 5.0, 7.0]
```

Example 3.

```cpp
auto out = timed.time_moving("time", 3, TimeWindowStatistic::Mean);
```

#### Expected output

```text
value = [1.0, 1.5, 2.5, 3.5]
```

Example 4.

```cpp
auto out = timed.time_moving("time", 3, TimeWindowStatistic::Deviation, {.ddof=1, .min_count=2});
```

#### Expected output

```text
value = [NA, 0.707107, 0.707107, 0.707107]
```

Example 5.

```cpp
auto out = timed.time_moving("time", 3, TimeWindowStatistic::Sum, {}, WindowClosure::Both);
```

#### Expected output

```text
the case exactly on the left boundary participates
```
