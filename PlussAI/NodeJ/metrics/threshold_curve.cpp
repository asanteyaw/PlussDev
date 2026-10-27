#include "pluss/ai/nodej/metrics.h"

#include "pluss/codem/series_factory.h"

#include <vector>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] Config i64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
}

} // namespace

namespace detail {

StatusOr<BinaryScoreCurve> binary_score_curve(const Series& scores, const Series& labels01) {
  const Series y = labels01.to(ScalarType::Float64).reshape({labels01.numel()});
  const Series p = scores.to(ScalarType::Float64).reshape({scores.numel()});
  if (p.numel() == 0 || p.numel() != y.numel()) {
    return StatusOr<BinaryScoreCurve>(
        Status::shape_mismatch("binary_score_curve requires equal non-empty score/label vectors."));
  }

  const std::int64_t n = p.numel();
  const Platform platform = p.platform();

  const Series order = p.argsort(0, /*descending=*/true, /*stable=*/true);
  const Series p_sorted = p.index_select(0, order);
  const Series y_sorted = y.index_select(0, order);
  const Series one_vec = pluss::series::ones({n}, f64_on(platform));
  const Series tp_cum = y_sorted.cumsum(0, false);
  const Series fp_cum = one_vec.sub(y_sorted).cumsum(0, false);

  // A sorted position is a real threshold boundary only if the next score
  // differs from it -- ties collapse onto their last (lowest-recall) position
  // instead of reporting one threshold per tied sample. The final position is
  // always a boundary, handled here by comparing against a sentinel value
  // that can never equal a real score.
  Series idx_next = pluss::series::arange(1, n, i64_on(platform));
  if (idx_next.numel() == 0) {
    idx_next = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_on(platform));
  }
  Series p_next = p_sorted.index_select(0, idx_next).reshape({idx_next.numel()});
  if (n == 1) {
    p_next = p_sorted.add(1.0);
  } else {
    p_next = p_next.unite(
        {p_sorted.reshape({n}).index_select(0, pluss::series::arange(n - 1, n, i64_on(platform))).add(1.0)}, 0);
  }
  const Series boundary = p_sorted.ne(p_next).reshape({n});

  return StatusOr<BinaryScoreCurve>(BinaryScoreCurve{
      .score_sorted = p_sorted,
      .true_positive_cumulative = tp_cum,
      .false_positive_cumulative = fp_cum,
      .is_threshold_boundary = boundary,
      .positive_count = y.sum(),
      .negative_count = y.mul(-1.0).add(1.0).sum(),
  });
}

} // namespace detail

StatusOr<ThresholdCurve> threshold_curve(const Series& scores, const Series& labels) {
  StatusOr<detail::BinaryScoreCurve> curve_or = detail::binary_score_curve(scores, labels);
  if (!curve_or.ok()) return StatusOr<ThresholdCurve>(curve_or.status());
  const detail::BinaryScoreCurve curve = curve_or.move_value();

  // predicted_positive[i] == i+1 for every sorted position (each sample is
  // either a true or false positive at each step), so it is never zero --
  // only recall's denominator (positive_count, which can legitimately be 0
  // for an all-negative label vector) needs the clamp-to-1 zero-division
  // guard.
  const Series predicted_positive = curve.true_positive_cumulative.add(curve.false_positive_cumulative);
  const Series positive_safe = curve.positive_count.clamp_min(1.0);

  const Series precision_curve = curve.true_positive_cumulative.div(predicted_positive);
  const Series recall_curve = curve.true_positive_cumulative.div(positive_safe);
  // f1_den = precision + recall ranges over [0, 2] here (a curve, not a
  // single threshold's scalar precision/recall), so the zero-division guard
  // must be a near-zero epsilon, not clamp_min(1.0) -- clamping a
  // legitimate sub-1.0 sum up to 1.0 would silently shrink F1 whenever
  // precision + recall < 1, which is the common case, not the exception.
  const Series f1_den = precision_curve.add(recall_curve);
  const Series f1_mask = f1_den.gt(0.0).to(ScalarType::Float64);
  const Series f1_curve = precision_curve.mul(recall_curve).mul(2.0).div(f1_den.clamp_min(1e-12)).mul(f1_mask);

  return StatusOr<ThresholdCurve>(ThresholdCurve{
      .thresholds = curve.score_sorted.masked_select(curve.is_threshold_boundary),
      .precision = precision_curve.masked_select(curve.is_threshold_boundary),
      .recall = recall_curve.masked_select(curve.is_threshold_boundary),
      .f1 = f1_curve.masked_select(curve.is_threshold_boundary),
  });
}

StatusOr<double> best_threshold(const Series& scores, const Series& labels, MetricKind objective) {
  StatusOr<ThresholdCurve> curve_or = threshold_curve(scores, labels);
  if (!curve_or.ok()) return StatusOr<double>(curve_or.status());
  const ThresholdCurve curve = curve_or.move_value();

  const Series& objective_curve = (objective == MetricKind::Precision)   ? curve.precision
                                  : (objective == MetricKind::Recall ||
                                     objective == MetricKind::Sensitivity) ? curve.recall
                                                                            : curve.f1;
  if (objective_curve.numel() == 0) {
    return StatusOr<double>(Status::invalid_argument("best_threshold: empty score/label input."));
  }

  const Series best_index =
      objective_curve.argmax(0, false).reshape({1}).to(scores.platform(), ScalarType::Int64);
  const Series best_value = curve.thresholds.index_select(0, best_index).reshape({1});
  return StatusOr<double>(best_value.to(pluss::pCPU, ScalarType::Float64).host<double>().front());
}

StatusOr<double> average_precision(const Series& scores, const Series& labels) {
  StatusOr<ThresholdCurve> curve_or = threshold_curve(scores, labels);
  if (!curve_or.ok()) return StatusOr<double>(curve_or.status());
  const ThresholdCurve curve = curve_or.move_value();

  const std::int64_t m = curve.recall.numel();
  if (m == 0) {
    return StatusOr<double>(Status::invalid_argument("average_precision: empty score/label input."));
  }
  const Platform platform = scores.platform();
  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);

  // recall along the descending-threshold sweep is non-decreasing; prepend a
  // leading 0 so the first term contributes (R_1 - 0) * P_1. When there are no
  // positives threshold_curve returns an all-zero recall, so AP falls out as 0.
  const Series recall_prev =
      (m > 1) ? pluss::series::zeros({1}, f64_on(platform))
                    .unite({curve.recall.index_select(0, pluss::series::arange(0, m - 1, i64_cfg))}, 0)
              : pluss::series::zeros({1}, f64_on(platform));
  const Series ap = curve.recall.sub(recall_prev).mul(curve.precision).sum().clamp(0.0, 1.0).reshape({1});
  return StatusOr<double>(ap.to(pluss::pCPU, ScalarType::Float64).host<double>().front());
}

} // namespace pluss::ai::nodej
