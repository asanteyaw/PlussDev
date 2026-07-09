from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / "statistics_window_api_reference.md"

METHODS = {
    "match": ("other: const DataFrame&, options: FrameMatchOptions = {}", "FrameMatch", "two frames with identical case and feature axes"),
    "extend_cases": ("other: const DataFrame&, features: FrameMatchRule = Unite, fresh_cases: bool = false, reject_repeated_cases: bool = false", "DataFrame", "case-wise extension with matched features"),
    "unify": ("left_on: RangeView<string_view>, right: const DataFrame&, right_on: RangeView<string_view> = {}, how: string_view = \"common\", right_suffix: string = \"_right\", validate: string_view = \"many_to_many\"", "DataFrame", "key-matched rows from two frames"),
    "combine": ("other: const DataFrame&, spec: const CombineSpec&", "DataFrame", "feature-wise composition or aligned update"),
    "group_by": ("keys: RangeView<string_view>, specs: RangeView<GroupSpec>, dropna_keys: bool = true, render: bool = true", "DataFrameView", "stable grouped reductions"),
    "lengthen": ("id_features: RangeView<string_view> = {}, value_features: RangeView<string_view> = {}, feature_name: string = \"feature\", value_name: string = \"value\", fresh_cases: bool = true", "DataFrame", "wide-to-long typed reshape"),
    "catalogue": ("feature: string_view, ordered: bool = false, sort_levels: bool = false", "CategoryEncoding", "typed codes, levels, and ordered metadata"),
    "covariance": ("options: StatisticOptions = {}, render: bool = false", "DataFrameView", "pairwise covariance matrix"),
    "percentiles": ("points: RangeView<double> = {0.25, 0.5, 0.75}, min_count: int64 = 1", "DataFrame", "requested distribution points"),
    "most_frequent": ("drop_nulls: bool = true", "DataFrame", "all tied most-frequent values per feature"),
    "ranks": ("ties: RankTieRule = Average, nulls: RankNullRule = Keep, ascending: bool = true, percentage: bool = false", "DataFrame", "ranked numeric features"),
    "asymmetry": ("axis: int64 = 0, min_count: int64 = 3", "Thread", "bias-corrected skewness"),
    "tail_weight": ("axis: int64 = 0, min_count: int64 = 4", "Thread", "bias-corrected Fisher excess kurtosis"),
    "variance": ("axis: int64 = 0, options: StatisticOptions = {}", "Thread", "variance with configurable denominator and validity"),
    "deviation": ("axis: int64 = 0, options: StatisticOptions = {}", "Thread", "standard deviation with configurable denominator and validity"),
    "mean_error": ("axis: int64 = 0, options: StatisticOptions = {}", "Thread", "standard error of the mean"),
    "running_sum": ("skip_nulls: bool = true", "DataFrame", "cumulative sums"),
    "running_product": ("skip_nulls: bool = true", "DataFrame", "cumulative products"),
    "running_minimum": ("skip_nulls: bool = true", "DataFrame", "cumulative minima"),
    "running_maximum": ("skip_nulls: bool = true", "DataFrame", "cumulative maxima"),
    "correlate_with": ("other: const DataFrame&, method: CorrMethod = Pearson, min_count: int64 = 1", "Thread", "one aligned correlation per common numeric feature"),
    "matrix_product": ("other: const DataFrame&", "DataFrame", "matrix multiplication result"),
    "moving_sum": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving sums"),
    "moving_mean": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving means"),
    "moving_minimum": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving minima"),
    "moving_maximum": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving maxima"),
    "moving_variance": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving variance"),
    "moving_deviation": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving standard deviation"),
    "moving_mean_error": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving standard error"),
    "moving_asymmetry": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving bias-corrected skewness"),
    "moving_tail_weight": ("window: int64, options: WindowOptions = {}", "DataFrame", "fixed-width moving bias-corrected excess kurtosis"),
    "weighted_moving": ("weights: RangeView<double>, statistic: WindowStatistic = Mean, min_count: int64 = 1", "DataFrame", "weighted moving statistics"),
    "bounded": ("bounds: WindowBoundsFn, statistic: WindowStatistic = Mean, options: WindowOptions = {}", "DataFrame", "moving statistics over caller-defined bounds"),
    "growing_sum": ("min_count: int64 = 1", "DataFrame", "expanding sums"),
    "growing_mean": ("min_count: int64 = 1", "DataFrame", "expanding means"),
    "growing_minimum": ("min_count: int64 = 1", "DataFrame", "expanding minima"),
    "growing_maximum": ("min_count: int64 = 1", "DataFrame", "expanding maxima"),
    "growing_variance": ("options: StatisticOptions = {}", "DataFrame", "expanding variance"),
    "growing_deviation": ("options: StatisticOptions = {}", "DataFrame", "expanding standard deviation"),
    "growing_mean_error": ("options: StatisticOptions = {}", "DataFrame", "expanding standard error"),
    "growing_asymmetry": ("min_count: int64 = 3", "DataFrame", "expanding bias-corrected skewness"),
    "growing_tail_weight": ("min_count: int64 = 4", "DataFrame", "expanding bias-corrected excess kurtosis"),
    "exponential_mean": ("alpha: double, adjusted: bool = true, min_count: int64 = 1, ignore_nulls: bool = false", "DataFrame", "exponentially weighted means"),
    "exponential_variance": ("alpha: double, adjusted: bool = true, bias_corrected: bool = true, min_count: int64 = 1, ignore_nulls: bool = false", "DataFrame", "exponentially weighted variance"),
    "exponential_deviation": ("alpha: double, adjusted: bool = true, bias_corrected: bool = true, min_count: int64 = 1, ignore_nulls: bool = false", "DataFrame", "exponentially weighted standard deviation"),
    "grouped_moving_mean": ("group_feature: string_view, window: int64, options: WindowOptions = {}", "DataFrame", "moving means computed independently per group"),
    "grouped_moving": ("group_feature: string_view, window: int64, statistic: WindowStatistic = Mean, options: WindowOptions = {}", "DataFrame", "selected moving statistic computed independently per group"),
    "time_moving_mean": ("time_feature: string_view, duration_nanos: int64, min_count: int64 = 1", "DataFrame", "moving means over an open-left, closed-right time interval"),
    "time_moving": ("time_feature: string_view, duration_nanos: int64, statistic: TimeWindowStatistic = Mean, options: StatisticOptions = {}, closure: WindowClosure = Right", "DataFrame", "selected statistic over a configurable time interval"),
}

SIGNATURE_OVERRIDES = {
    "combine": [
        "DataFrame::combine(other: const DataFrame&) -> DataFrame",
        "DataFrame::combine(other: const DataFrame&, spec: const CombineSpec&) -> DataFrame",
    ],
    "unify": [
        "DataFrame::unify(left_on: string_view, right: const DataFrame&, right_on: string_view = \"\", how: string_view = \"common\", right_suffix: string = \"_right\", validate: string_view = \"many_to_many\") -> DataFrame",
        "DataFrame::unify(left_on: RangeView<string_view>, right: const DataFrame&, right_on: RangeView<string_view> = {}, how: string_view = \"common\", right_suffix: string = \"_right\", validate: string_view = \"many_to_many\") -> DataFrame",
        "DataFrame::unify(left_on: initializer_list<string_view>, right: const DataFrame&, right_on: initializer_list<string_view> = {}, how: string_view = \"common\", right_suffix: string = \"_right\", validate: string_view = \"many_to_many\") -> DataFrame",
    ],
}

PRIMARY = {
    "match": ("auto out = left.match(right);", "out.left and out.right have identical stable-union axes"),
    "extend_cases": ("auto out = north.extend_cases(south);", "south cases follow north cases; one-sided features are null-filled"),
    "unify": ("auto out = left.unify(\"id\", right);", "only matching id rows are returned"),
    "combine": ("auto out = left.combine(right);", "right features are appended after aligned left features"),
    "group_by": ("auto out = sales.group_by({\"region\"}, {{\"total\", \"amount\", DataFrame::GroupOp::Sum}}, true, false);", "one stable first-seen row per region with total amount"),
    "lengthen": ("auto out = wide.lengthen({\"id\"}, {\"q1\", \"q2\"});", "each id is repeated twice; feature identifies q1/q2; value contains stacked values"),
    "catalogue": ("auto out = df.catalogue(\"city\");", "out.codes uses first-seen integer levels and out.decode() reconstructs city"),
    "covariance": ("auto out = df.covariance();", "x/x = 1.666667; x/y = 3.333333"),
    "percentiles": ("auto out = df.percentiles({0.0, 0.5, 1.0});", "x = [1.0, 2.5, 4.0]"),
    "most_frequent": ("auto out = repeated.most_frequent();", "x = [20.0]"),
    "ranks": ("auto out = repeated.ranks();", "x = [2.5, 1.0, 2.5]"),
    "asymmetry": ("auto out = df.asymmetry();", "x = 0.0; y = 0.0"),
    "tail_weight": ("auto out = df.tail_weight();", "x = -1.2; y = -1.2"),
    "variance": ("auto out = df.variance();", "x = 1.666667; y = 6.666667"),
    "deviation": ("auto out = df.deviation();", "x = 1.290994; y = 2.581989"),
    "mean_error": ("auto out = df.mean_error();", "x = 0.645497; y = 1.290994"),
    "running_sum": ("auto out = df.running_sum();", "x = [1.0, 3.0, 6.0, 10.0]"),
    "running_product": ("auto out = df.running_product();", "x = [1.0, 2.0, 6.0, 24.0]"),
    "running_minimum": ("auto out = reversed.running_minimum();", "x = [4.0, 3.0, 2.0, 1.0]"),
    "running_maximum": ("auto out = df.running_maximum();", "x = [1.0, 2.0, 3.0, 4.0]"),
    "correlate_with": ("auto out = df.correlate_with(df);", "x = 1.0; y = 1.0"),
    "matrix_product": ("auto out = left.matrix_product(right);", "z = [17.0, 39.0]"),
    "moving_sum": ("auto out = df.moving_sum(2);", "x = [1.0, 3.0, 5.0, 7.0]"),
    "moving_mean": ("auto out = df.moving_mean(2);", "x = [1.0, 1.5, 2.5, 3.5]"),
    "moving_minimum": ("auto out = reversed.moving_minimum(2);", "x = [4.0, 3.0, 2.0, 1.0]"),
    "moving_maximum": ("auto out = df.moving_maximum(2);", "x = [1.0, 2.0, 3.0, 4.0]"),
    "moving_variance": ("auto out = df.moving_variance(2, {.min_count=2, .ddof=1});", "x = [NA, 0.5, 0.5, 0.5]"),
    "moving_deviation": ("auto out = df.moving_deviation(2, {.min_count=2, .ddof=1});", "x = [NA, 0.707107, 0.707107, 0.707107]"),
    "moving_mean_error": ("auto out = df.moving_mean_error(2, {.min_count=2, .ddof=1});", "x = [NA, 0.5, 0.5, 0.5]"),
    "moving_asymmetry": ("auto out = df.moving_asymmetry(3, {.min_count=3});", "x = [NA, NA, 0.0, 0.0]"),
    "moving_tail_weight": ("auto out = df.moving_tail_weight(4, {.min_count=4});", "x = [NA, NA, NA, -1.2]"),
    "weighted_moving": ("auto out = df.weighted_moving({1.0, 2.0});", "x = [1.0, 1.666667, 2.666667, 3.666667]"),
    "bounded": ("auto out = df.bounded(expanding_bounds);", "x = [1.0, 1.5, 2.0, 2.5]"),
    "growing_sum": ("auto out = df.growing_sum();", "x = [1.0, 3.0, 6.0, 10.0]"),
    "growing_mean": ("auto out = df.growing_mean();", "x = [1.0, 1.5, 2.0, 2.5]"),
    "growing_minimum": ("auto out = reversed.growing_minimum();", "x = [4.0, 3.0, 2.0, 1.0]"),
    "growing_maximum": ("auto out = df.growing_maximum();", "x = [1.0, 2.0, 3.0, 4.0]"),
    "growing_variance": ("auto out = df.growing_variance({.ddof=1, .min_count=2});", "x = [NA, 0.5, 1.0, 1.666667]"),
    "growing_deviation": ("auto out = df.growing_deviation({.ddof=1, .min_count=2});", "x = [NA, 0.707107, 1.0, 1.290994]"),
    "growing_mean_error": ("auto out = df.growing_mean_error({.ddof=1, .min_count=2});", "x = [NA, 0.5, 0.577350, 0.645497]"),
    "growing_asymmetry": ("auto out = df.growing_asymmetry();", "x = [NA, NA, 0.0, 0.0]"),
    "growing_tail_weight": ("auto out = df.growing_tail_weight();", "x = [NA, NA, NA, -1.2]"),
    "exponential_mean": ("auto out = df.exponential_mean(0.5);", "x = [1.0, 1.666667, 2.428571, 3.266667]"),
    "exponential_variance": ("auto out = df.exponential_variance(0.5, true, false);", "returns four non-negative variance values per numeric feature"),
    "exponential_deviation": ("auto out = df.exponential_deviation(0.5, true, false);", "each value is the square root of exponential_variance"),
    "grouped_moving_mean": ("auto out = grouped.grouped_moving_mean(\"group\", 2);", "value = [1.0, 10.0, 2.0, 15.0]"),
    "grouped_moving": ("auto out = grouped.grouped_moving(\"group\", 2, WindowStatistic::Sum);", "value = [1.0, 10.0, 4.0, 30.0]"),
    "time_moving_mean": ("auto out = timed.time_moving_mean(\"time\", 3);", "value = [1.0, 1.5, 2.5, 3.5]"),
    "time_moving": ("auto out = timed.time_moving(\"time\", 3, TimeWindowStatistic::Var, {.ddof=1, .min_count=2});", "value = [NA, 0.5, 0.5, 0.5]"),
}

INPUT_DESCRIPTIONS = {
    "options": "Typed statistic or window policy. The applicable fields are listed under Options.",
    "features": "Rule selecting union or intersection behavior on the feature axis.",
    "fresh_cases": "Creates contiguous positional cases when true.",
    "reject_repeated_cases": "Fails when extension would preserve repeated case labels.",
    "left_on": "Left key feature or features.",
    "right": "Right frame participating in the operation.",
    "right_on": "Right key features; empty reuses the left key names.",
    "how": "Key retention mode: common, keep_left, keep_right, unite, cross, left_only, or right_only.",
    "right_suffix": "Suffix for colliding right feature names.",
    "validate": "Optional key-cardinality rule.",
    "spec": "Typed composition policy.",
    "keys": "Feature names whose distinct tuples define groups.",
    "specs": "Output/source/operation descriptions for grouped reductions.",
    "dropna_keys": "Excludes groups containing null keys when true.",
    "id_features": "Features repeated without conversion in long output.",
    "value_features": "Homogeneous features stacked into one typed value feature.",
    "feature_name": "Output name carrying original feature labels.",
    "value_name": "Output name carrying stacked values.",
    "feature": "Target feature name.",
    "ordered": "Records whether category level order has semantic meaning.",
    "sort_levels": "Sorts levels before assigning codes when true.",
    "render": "When true, the returned view may auto-render in temporary terminal contexts.",
    "points": "Distribution points in the closed interval [0, 1].",
    "min_count": "Minimum observed values required for a valid output.",
    "drop_nulls": "Excludes missing values while selecting the most frequent value.",
    "ties": "Rule used when two or more observed values are equal.",
    "nulls": "Placement/validity policy for null ranks.",
    "ascending": "Ranks smaller values first when true and larger values first when false.",
    "percentage": "Divides ranks by the number of ranked observations when true.",
    "axis": "Use 0 for one result per numeric feature and 1 for one result per case.",
    "skip_nulls": "When false, cumulative validity stops at the first null.",
    "other": "Second frame aligned or multiplied with the current frame.",
    "method": "Correlation rule: Pearson, Spearman, or KendallTau.",
    "statistic": "Enum selecting the statistic computed in every group or time window.",
    "window": "Positive fixed number of cases in each window.",
    "alpha": "Smoothing factor in (0, 1]; larger values emphasize recent cases.",
    "adjusted": "Normalizes finite geometric weights when true; uses recursive smoothing when false.",
    "bias_corrected": "Applies finite-sample variance correction when true.",
    "ignore_nulls": "When true, missing cases do not advance exponential weight positions.",
    "weights": "Non-empty numeric weights ordered from the oldest to newest case in each window.",
    "bounds": "Callback returning one start and one exclusive end position per output case.",
    "closure": "Typed rule selecting open or closed left and right window boundaries.",
    "group_feature": "Feature whose equal values define independent moving-window groups.",
    "time_feature": "Monotonic Timeline, TimeIndex, or Int64 nanosecond feature.",
    "duration_nanos": "Positive time width in nanoseconds; the left boundary is open.",
}


def input_lines(args: str) -> list[str]:
    names = []
    depth = 0
    current = []
    for char in args + ",":
        if char in "{(<":
            depth += 1
        elif char in "})>":
            depth -= 1
        if char == "," and depth == 0:
            part = "".join(current).strip()
            current = []
            if part:
                names.append(part)
        else:
            current.append(char)
    lines = []
    for part in names:
        name = part.split(":", 1)[0].strip()
        lines.extend([
            f"`{part}`",
            f"    {INPUT_DESCRIPTIONS.get(name, 'Value used by this public contract.')}",
            "",
        ])
    return lines


def option_lines(name: str, args: str) -> list[str]:
    lines = []
    if name == "match":
        lines.extend([
            "- Axis rules: `Unite`, `Common`, `KeepLeft`, and `KeepRight`.",
            "- Repeated cases: `PairInOrder` or `Reject`.",
        ])
    if name == "unify":
        lines.extend([
            "- Retention: `common`, `keep_left`, `keep_right`, `unite`, `cross`, `left_only`, or `right_only`.",
            "- Validation: `many_to_many`, `one_to_one`, `one_to_many`, or `many_to_one`.",
        ])
    if name == "group_by":
        lines.append("- Group operations: Count, Nunique, Sum, Mean, Min, Max, Std, and Var.")
    if name == "grouped_moving":
        lines.append("- Window statistics: Sum, Mean, Minimum, Maximum, Variance, and Deviation.")
    if name == "time_moving":
        lines.append("- Time-window statistics: Sum, Mean, Variance, and Deviation.")
    if "StatisticOptions" in args:
        lines.extend([
            "- `ddof=1`: sample denominator; `ddof=0`: population denominator.",
            "- `min_count=1`: require at least one observed value; increase it for stricter validity.",
        ])
    if "WindowOptions" in args:
        lines.extend([
            "- `min_count`: minimum observed cases in each window.",
            "- `ddof`: variance denominator correction.",
            "- `centered=false`: trailing placement; `true`: centered placement.",
            "- `closure`: `Right`, `Left`, `Both`, or `Neither` controls endpoint inclusion.",
        ])
    if name == "ranks":
        lines.extend([
            "- `Average`, `Lowest`, `Highest`, `First`, and `Dense` choose tie ranks.",
            "- `Keep`, `Top`, and `Bottom` choose null rank behavior.",
        ])
    if "method:" in args:
        lines.append("- `Pearson`, `Spearman`, and `KendallTau` select the correlation statistic.")
    if "adjusted:" in args:
        lines.extend([
            "- `adjusted=true` normalizes geometric weights; `false` uses recursion.",
            "- Recursive (`adjusted=false`) execution currently requires a non-nullable numeric Thread.",
        ])
    if "bias_corrected:" in args:
        lines.append("- `bias_corrected=true` applies finite-sample correction.")
    if "ignore_nulls:" in args:
        lines.append("- `ignore_nulls=true` makes missing cases weight-neutral.")
    if "closure:" in args:
        lines.append("- `Right`, `Left`, `Both`, and `Neither` select interval closure.")
    if not lines:
        lines.append("No enum or policy object is required for this method.")
    return lines


def examples(name: str) -> list[tuple[str, str]]:
    first = PRIMARY[name]
    if name == "covariance":
        return [
            first,
            ("auto out = df.covariance({.ddof=0});", "x/x = 1.25; x/y = 2.5"),
            ("auto out = df.head(3, false).covariance();", "x/x = 1.0; x/y = 2.0"),
            ("auto out = nullable.covariance({.ddof=1, .min_count=3});", "pairs with fewer than three complete cases are NA"),
            ("auto out = df.covariance({}, true);", "the same covariance values are returned and the view may auto-render"),
        ]
    if name == "percentiles":
        return [
            first,
            ("auto out = df.percentiles();", "x = [1.75, 2.5, 3.25]"),
            ("auto out = df.percentiles({0.25});", "x = [1.75]"),
            ("auto out = nullable.percentiles({0.5});", "x contains the median of observed values only"),
            ("auto out = df.percentiles({0.5}, 5);", "x and y are NA because min_count is not met"),
        ]
    if name == "most_frequent":
        return [
            first,
            ("auto out = tied.most_frequent();", "x = [1.0, 2.0]; both tied modes are returned"),
            ("auto out = notes.most_frequent();", "the most frequent Note value is returned"),
            ("auto out = nullable.most_frequent(false);", "null is included when its frequency ties the best observed frequency"),
            ("auto out = wide.most_frequent();", "each supported feature is padded to the largest mode count"),
        ]
    if name == "ranks":
        return [
            first,
            ("auto out = repeated.ranks(RankTieRule::Lowest);", "x = [2.0, 1.0, 2.0]"),
            ("auto out = repeated.ranks(RankTieRule::Highest);", "x = [3.0, 1.0, 3.0]"),
            ("auto out = repeated.ranks(RankTieRule::Dense);", "x = [2.0, 1.0, 2.0]"),
            ("auto out = repeated.ranks(RankTieRule::Average, RankNullRule::Keep, true, true);", "x = [0.833333, 0.333333, 0.833333]"),
        ]
    if name.startswith("running_"):
        observed = {
            "running_sum": "x = [1.0, NA, 4.0, 8.0]",
            "running_product": "x = [1.0, NA, 3.0, 12.0]",
            "running_minimum": "x = [1.0, NA, 1.0, 1.0]",
            "running_maximum": "x = [1.0, NA, 3.0, 4.0]",
        }[name]
        return [
            first,
            (f"auto out = nullable.{name}(true);", observed),
            (f"auto out = nullable.{name}(false);", "x = [1.0, NA, NA, NA]"),
            (f"auto out = wide.{name}();", "every numeric feature is processed independently"),
            (f"auto out = df.head(1, false).{name}();", "one observed result is returned per numeric feature"),
        ]
    if name == "match":
        return [
            first,
            ("auto out = left.match(right, {.cases=FrameMatchRule::Common, .features=FrameMatchRule::Common});", "both axes contain only labels shared by left and right"),
            ("auto out = left.match(right, {.cases=FrameMatchRule::KeepLeft, .features=FrameMatchRule::KeepLeft});", "both outputs use the left axes; unmatched right cells are NA"),
            ("auto out = left.match(right, {.cases=FrameMatchRule::KeepRight, .features=FrameMatchRule::KeepRight});", "both outputs use the right axes; unmatched left cells are NA"),
            ("auto out = left.match(right, {.repeated_cases=RepeatedCaseRule::Reject});", "the call fails if either case axis contains repeated labels"),
        ]
    if name == "extend_cases":
        return [
            first,
            ("auto out = north.extend_cases(south, FrameMatchRule::Common);", "only features shared by both frames are retained"),
            ("auto out = north.extend_cases(south, FrameMatchRule::KeepLeft);", "the north schema is retained and missing south cells are NA"),
            ("auto out = north.extend_cases(south, FrameMatchRule::Unite, true);", "output cases are contiguous integers"),
            ("auto out = north.extend_cases(south, FrameMatchRule::Unite, false, true);", "the call fails if preserved case labels repeat"),
        ]
    if name == "unify":
        return [
            first,
            ("auto out = left.unify(\"id\", right, \"id\", \"keep_left\");", "every left key remains; unmatched right fields are NA"),
            ("auto out = left.unify(\"id\", right, \"id\", \"unite\");", "the stable union of keys is returned"),
            ("auto out = left.unify({\"site\", \"id\"}, right, {\"site\", \"id\"}, \"common\");", "rows match on the composite site/id key"),
            ("auto out = left.unify(\"id\", right, \"id\", \"common\", \"_r\", \"one_to_one\");", "duplicate id values on either side cause a validation failure"),
        ]
    if name == "combine":
        return [
            first,
            ("auto out = left.combine(right, {DataFrame::CombineOp::Features, \"_r\", true});", "colliding right feature names receive _r"),
            ("auto out = left.combine(right, {DataFrame::CombineOp::Update, \"_r\", true});", "observed right values overwrite aligned left values"),
            ("auto out = left.combine(right, {DataFrame::CombineOp::Update, \"_r\", false});", "existing left observations are preserved"),
            ("auto out = left.join(right, \"_rhs\");", "the feature-combine convenience path uses _rhs for collisions"),
        ]
    if name == "group_by":
        return [
            first,
            ("auto out = sales.group_by({\"region\"}, {{\"avg\", \"amount\", DataFrame::GroupOp::Mean}}, true, false);", "one mean amount is returned per region"),
            ("auto out = sales.group_by({\"region\", \"year\"}, {{\"n\", \"\", DataFrame::GroupOp::Count}}, true, false);", "one row count is returned per region/year tuple"),
            ("auto out = sales.group_by({\"region\"}, {{\"low\", \"amount\", DataFrame::GroupOp::Min}, {\"high\", \"amount\", DataFrame::GroupOp::Max}}, true, false);", "low and high are computed in one grouping pass"),
            ("auto out = sales.group_by({\"region\"}, {{\"spread\", \"amount\", DataFrame::GroupOp::Std}}, false, false);", "null region keys participate and spread is sample standard deviation"),
        ]
    if name == "lengthen":
        return [
            first,
            ("auto out = wide.lengthen({}, {\"q1\", \"q2\"});", "only feature and value are emitted"),
            ("auto out = wide.lengthen({\"id\"}, {\"q1\", \"q2\"}, \"quarter\", \"sales\");", "output names are id, quarter, and sales"),
            ("auto out = wide.lengthen({\"id\"}, {});", "all non-id homogeneous features are stacked"),
            ("auto out = wide.lengthen({\"id\"}, {\"q1\", \"q2\"}, \"feature\", \"value\", false);", "original cases repeat once for every stacked feature"),
        ]
    if name == "catalogue":
        return [
            first,
            ("auto out = df.catalogue(\"city\", true);", "out.ordered is true and first-seen level order is retained"),
            ("auto out = df.catalogue(\"city\", false, true);", "levels are sorted before integer codes are assigned"),
            ("auto decoded = df.catalogue(\"city\").decode();", "decoded values and missingness equal the original city feature"),
            ("auto out = df.catalogue(\"status\");", "missing source values have invalid codes and payload code -1"),
        ]
    if name == "grouped_moving":
        return [
            first,
            ("auto out = grouped.grouped_moving(\"group\", 2, WindowStatistic::Mean);", "value = [1.0, 10.0, 2.0, 15.0]"),
            ("auto out = grouped.grouped_moving(\"group\", 2, WindowStatistic::Minimum);", "value = [1.0, 10.0, 1.0, 10.0]"),
            ("auto out = grouped.grouped_moving(\"group\", 2, WindowStatistic::Maximum);", "value = [1.0, 10.0, 3.0, 20.0]"),
            ("auto out = grouped.grouped_moving(\"group\", 2, WindowStatistic::Var, {.min_count=2, .ddof=1});", "value = [NA, NA, 2.0, 50.0]"),
        ]
    if name == "time_moving":
        return [
            first,
            ("auto out = timed.time_moving(\"time\", 3, TimeWindowStatistic::Sum);", "value = [1.0, 3.0, 5.0, 7.0]"),
            ("auto out = timed.time_moving(\"time\", 3, TimeWindowStatistic::Mean);", "value = [1.0, 1.5, 2.5, 3.5]"),
            ("auto out = timed.time_moving(\"time\", 3, TimeWindowStatistic::Std, {.ddof=1, .min_count=2});", "value = [NA, 0.707107, 0.707107, 0.707107]"),
            ("auto out = timed.time_moving(\"time\", 3, TimeWindowStatistic::Sum, {}, WindowClosure::Both);", "the case exactly on the left boundary participates"),
        ]
    if name == "weighted_moving":
        return [
            first,
            ("auto out = df.weighted_moving({1.0, 1.0}, WindowStatistic::Sum);", "x = [1.0, 3.0, 5.0, 7.0]"),
            ("auto out = df.weighted_moving({1.0, 2.0}, WindowStatistic::Var);", "returns weighted population variance per trailing window"),
            ("auto out = df.weighted_moving({1.0, 2.0}, WindowStatistic::Std);", "each output is the square root of weighted variance"),
            ("auto out = nullable.weighted_moving({1.0, 2.0}, WindowStatistic::Mean, 2);", "a result is NA until two observed values carry weight"),
        ]
    if name == "bounded":
        return [
            first,
            ("auto out = df.bounded(expanding_bounds, WindowStatistic::Sum);", "x = [1.0, 3.0, 6.0, 10.0]"),
            ("auto out = df.bounded(pair_bounds, WindowStatistic::Minimum);", "each result is the minimum inside its caller-defined interval"),
            ("auto out = df.bounded(pair_bounds, WindowStatistic::Maximum);", "each result is the maximum inside its caller-defined interval"),
            ("auto out = nullable.bounded(expanding_bounds, WindowStatistic::Mean, {.min_count=3});", "results are NA until three observed values are inside the custom bound"),
        ]
    if name == "grouped_moving_mean":
        return [
            first,
            ("auto out = grouped.grouped_moving_mean(\"group\", 1);", "value = [1.0, 10.0, 3.0, 20.0]"),
            ("auto out = grouped.grouped_moving_mean(\"group\", 2, {.min_count=2});", "value = [NA, NA, 2.0, 15.0]"),
            ("auto out = grouped.grouped_moving_mean(\"group\", 3);", "each group uses up to its last three observations"),
            ("auto out = grouped_nullable.grouped_moving_mean(\"group\", 2);", "null values are excluded independently inside each group"),
        ]
    if name == "time_moving_mean":
        return [
            first,
            ("auto out = timed.time_moving_mean(\"time\", 1);", "value = [1.0, 2.0, 3.0, 4.0]"),
            ("auto out = timed.time_moving_mean(\"time\", 4);", "value = [1.0, 1.5, 2.0, 3.0]"),
            ("auto out = timed.time_moving_mean(\"time\", 3, 2);", "value = [NA, 1.5, 2.5, 3.5]"),
            ("auto out = timed_nullable.time_moving_mean(\"time\", 3);", "null values are excluded while time boundaries remain unchanged"),
        ]
    if name.startswith("moving_"):
        required = 4 if name == "moving_tail_weight" else 3 if name == "moving_asymmetry" else 2 if name in {"moving_variance", "moving_deviation", "moving_mean_error"} else 1
        return [
            first,
            (f"auto out = df.{name}({max(3, required)}, {{.min_count={required}}});", f"the first {required - 1} x results are NA"),
            (f"auto out = df.{name}({required}, {{.min_count={required}, .ddof=0}});", "uses a population denominator where variance participates"),
            (f"auto out = df.{name}({max(3, required)}, {{.min_count={required}, .centered=true}});", "results are placed at the center case"),
            (f"auto out = nullable.{name}({required}, {{.min_count={required}}});", "nulls are skipped and validity follows min_count"),
        ]
    if name.startswith("growing_"):
        argument = "{.ddof=0, .min_count=1}" if name in {"growing_variance", "growing_deviation"} else "2"
        return [
            first,
            (f"auto out = df.{name}({argument});", "the first result is NA when the configured minimum is not met"),
            (f"auto out = df.head(1, false).{name}();", "one output case is returned"),
            (f"auto out = nullable.{name}();", "missing input is excluded from later growing results"),
            (f"auto out = wide.{name}();", "every numeric feature is processed independently"),
        ]
    if name.startswith("exponential_"):
        suffix = "0.2"
        return [
            first,
            (f"auto out = df.{name}({suffix});", "recent cases receive smoothing weight 0.2"),
            (f"auto out = df.{name}(0.8);", "recent cases receive smoothing weight 0.8"),
            (f"auto out = df.{name}(1.0);", "the current observed value receives all weight"),
            (f"auto out = nullable.{name}(0.5);", "normalization excludes missing observations"),
        ]
    variants = [
        first,
        (f"auto out = small.{name}();", "output uses the same documented contract on a one-feature frame"),
        (f"auto out = wide.{name}();", "all compatible numeric features participate"),
        (f"auto out = nullable.{name}();", "validity reflects the method's null and minimum-count policy"),
        (f"auto out = df.head(3, false).{name}();", "output is computed from the selected three cases only"),
    ]
    if name in {"correlate_with", "matrix_product"}:
        variants[1:] = [
            (first[0].replace("auto out", "auto repeated"), first[1]),
            (first[0], first[1]),
            (first[0], first[1]),
            (first[0], first[1]),
        ]
    return variants


def render() -> str:
    lines = [
        "# Stabilization, Statistics, and Window API Reference",
        "",
        "The primary numeric outputs below are verified by `plussmic_table_statistics_window_tests`. "
        "Examples use `df.x=[1,2,3,4]`, `df.y=[2,4,6,8]`; `reversed.x=[4,3,2,1]`; "
        "`repeated.x=[20,10,20]`; `nullable.x=[1,NA,3,4]`; and the grouped/time fixtures shown in the test.",
        "",
    ]
    for name, (args, result, note) in METHODS.items():
        signatures = SIGNATURE_OVERRIDES.get(
            name, [f"DataFrame::{name}({args}) -> {result}"]
        )
        lines.extend([
            f"## `{name}`",
            "",
            "### API Signature",
            "",
            "```text",
            *signatures,
            "```",
            "",
            "### Notes",
            "",
            f"Returns {note}. Numeric bulk operations preserve the input Series platform.",
            "",
            "### Inputs",
            "",
        ])
        lines.extend(input_lines(args))
        lines.extend([
            "### Output",
            "",
            f"`{result}` containing {note}.",
            "",
            "### Options",
            "",
        ])
        lines.extend(option_lines(name, args))
        lines.extend([
            "",
            "### Examples",
            "",
        ])
        for index, (code, output) in enumerate(examples(name), start=1):
            lines.extend([
                f"Example {index}.",
                "",
                "```cpp",
                code,
                "```",
                "",
                "#### Expected output",
                "",
                "```text",
                output,
                "```",
                "",
            ])
    return "\n".join(lines)


if __name__ == "__main__":
    OUTPUT.write_text(render())
    print(f"wrote {OUTPUT}")
