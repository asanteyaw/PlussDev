#pragma once

#include <pluss/nn_detail.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace pluss::ai::nn {

enum class LossReduction {
  None,
  Mean,
  Sum,
};

namespace losses {

struct MSEOptions {
  LossReduction reduction{LossReduction::Mean};
};

struct RelativeMSEOptions {
  LossReduction reduction{LossReduction::Mean};
  double epsilon{1e-12};
  bool use_absolute_divisor{true};
};

struct HuberOptions {
  double delta{1.0};
  LossReduction reduction{LossReduction::Mean};
};

struct L1Options {
  LossReduction reduction{LossReduction::Mean};
};

struct SmoothL1Options {
  double beta{1.0};
  LossReduction reduction{LossReduction::Mean};
};

struct BCEWithLogitsOptions {
  LossReduction reduction{LossReduction::Mean};
  Series weight{};
  Series positive_class_weight{};
};

struct NLLOptions {
  LossReduction reduction{LossReduction::Mean};
  std::int64_t class_dim{-1};
  Series class_weight{};
  bool use_ignore_index{false};
  std::int64_t ignore_index{-100};
};

struct CrossEntropyOptions {
  LossReduction reduction{LossReduction::Mean};
  std::int64_t class_dim{-1};
  double label_smoothing{0.0};
  Series class_weight{};
  bool use_ignore_index{false};
  std::int64_t ignore_index{-100};
};

namespace detail {

inline Series scalar_on_(double value, const Series& like) {
  return Series::full({}, value, like.scalar_type()).to(like.platform());
}

inline bool same_shape_(const Series& a, const Series& b) {
  if (a.dim() != b.dim()) return false;
  for (std::int64_t i = 0; i < a.dim(); ++i) {
    if (a.sizes()[i] != b.sizes()[i]) return false;
  }
  return true;
}

inline void require_same_shape_(const Series& a, const Series& b, const char* what) {
  PLUSS_CHECK(same_shape_(a, b), std::string("pluss::") + what + ": expected matching shapes");
}

inline std::int64_t normalize_dim_(std::int64_t dim, std::int64_t rank, const char* what) {
  PLUSS_CHECK(rank > 0, std::string("pluss::") + what + ": expected rank > 0");
  std::int64_t out = dim;
  if (out < 0) out += rank;
  PLUSS_CHECK(out >= 0 && out < rank, std::string("pluss::") + what + ": class_dim out of range");
  return out;
}

inline void require_same_platform_(const Series& a, const Series& b, const char* what) {
  PLUSS_CHECK(a.platform() == b.platform(), std::string("pluss::") + what + ": platform mismatch");
}

inline Series reduce_loss_(const Series& per, LossReduction reduction, const char* what) {
  ::pluss::detail::nn::require_defined_(per, what);
  switch (reduction) {
    case LossReduction::None:
      return per;
    case LossReduction::Sum:
      return per.sum();
    case LossReduction::Mean:
    default:
      return per.mean();
  }
}

inline Series reduce_loss_with_denom_(const Series& per, LossReduction reduction, const Series& denom, const char* what) {
  ::pluss::detail::nn::require_defined_(per, what);
  ::pluss::detail::nn::require_defined_(denom, what);
  switch (reduction) {
    case LossReduction::None:
      return per;
    case LossReduction::Sum:
      return per.sum();
    case LossReduction::Mean:
    default: {
      const Series safe_denom = denom.clamp_min(1.0);
      return per.sum().div(safe_denom);
    }
  }
}

inline void require_class_weight_(const Series& class_weight,
                                  const Series& logits_like,
                                  std::int64_t class_dim,
                                  const char* what) {
  if (!class_weight.defined()) return;
  ::pluss::detail::nn::require_training_float_dtype_(class_weight.scalar_type(), what, "class_weight");
  PLUSS_CHECK(class_weight.dim() == 1, std::string("pluss::") + what + ": class_weight must be rank-1");
  PLUSS_CHECK(
      class_weight.sizes()[0] == logits_like.sizes()[class_dim],
      std::string("pluss::") + what + ": class_weight size must match class dimension");
  PLUSS_CHECK(
      class_weight.platform() == logits_like.platform(),
      std::string("pluss::") + what + ": class_weight platform mismatch");
}

inline Series class_weight_cast_(const Series& class_weight, const Series& like) {
  if (!class_weight.defined()) return class_weight;
  if (class_weight.scalar_type() == like.scalar_type()) return class_weight;
  return class_weight.to(like.scalar_type());
}

inline Series class_weight_for_index_target_(const Series& class_weight,
                                             const Series& safe_target_index,
                                             const Series& like) {
  if (!class_weight.defined()) {
    return pluss::ones(safe_target_index.sizes(), Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
  }
  const Series flat = safe_target_index.reshape({safe_target_index.numel()});
  const Series gathered = class_weight.gather(0, flat);
  return gathered.reshape(safe_target_index.sizes()).to(like.scalar_type());
}

inline Series class_weight_view_(const Series& class_weight, const Series& logits_like, std::int64_t class_dim) {
  std::vector<std::int64_t> weight_shape(static_cast<std::size_t>(logits_like.dim()), 1);
  weight_shape[static_cast<std::size_t>(class_dim)] = logits_like.sizes()[class_dim];
  return class_weight.reshape(weight_shape);
}

inline Series stable_log_probs_(const Series& logits, std::int64_t class_dim) {
  const Series max_logits = logits.amax(class_dim, true);
  const Series shifted = logits.sub(max_logits.expand(logits.sizes(), true));
  const Series log_norm = shifted.exp().sum(class_dim, true).log();
  return shifted.sub(log_norm.expand(logits.sizes(), true));
}

inline void require_target_matches_logits_except_class_(const Series& target,
                                                        const Series& logits,
                                                        std::int64_t class_dim,
                                                        const char* what) {
  PLUSS_CHECK(
      target.dim() + 1 == logits.dim(),
      std::string("pluss::") + what + ": index target rank must be logits rank - 1");
  for (std::int64_t li = 0, ti = 0; li < logits.dim(); ++li) {
    if (li == class_dim) continue;
    PLUSS_CHECK(
        logits.sizes()[li] == target.sizes()[ti],
        std::string("pluss::") + what + ": index target shape mismatch with logits");
    ++ti;
  }
}

} // namespace detail

inline Series MSE(const Series& prediction, const Series& target, const MSEOptions& options = {}) {
  ::pluss::detail::nn::require_defined_(prediction, "nn::losses::MSE");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::MSE");
  ::pluss::detail::nn::require_training_float_dtype_(prediction.scalar_type(), "nn::losses::MSE", "prediction");
  ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::MSE", "target");
  detail::require_same_platform_(prediction, target, "nn::losses::MSE");
  const Series per = prediction.add(target.neg()).square();
  return detail::reduce_loss_(per, options.reduction, "nn::losses::MSE");
}

inline Series RelativeMSE(const Series& prediction,
                          const Series& target,
                          const Series& divisor,
                          const RelativeMSEOptions& options = {}) {
  ::pluss::detail::nn::require_defined_(prediction, "nn::losses::RelativeMSE");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::RelativeMSE");
  ::pluss::detail::nn::require_defined_(divisor, "nn::losses::RelativeMSE");
  ::pluss::detail::nn::require_training_float_dtype_(prediction.scalar_type(), "nn::losses::RelativeMSE", "prediction");
  ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::RelativeMSE", "target");
  ::pluss::detail::nn::require_training_float_dtype_(divisor.scalar_type(), "nn::losses::RelativeMSE", "divisor");
  detail::require_same_platform_(prediction, target, "nn::losses::RelativeMSE");
  detail::require_same_platform_(prediction, divisor, "nn::losses::RelativeMSE");
  PLUSS_CHECK(
      options.epsilon > 0.0 && std::isfinite(options.epsilon),
      "pluss::nn::losses::RelativeMSE: epsilon must be positive and finite");

  Series denom = (divisor.scalar_type() == prediction.scalar_type()) ? divisor : divisor.to(prediction.scalar_type());
  if (options.use_absolute_divisor) denom = denom.abs();
  denom = denom.clamp_min(options.epsilon);

  const Series per = prediction.add(target.neg()).square().div(denom);
  return detail::reduce_loss_(per, options.reduction, "nn::losses::RelativeMSE");
}

inline Series Huber(const Series& prediction, const Series& target, const HuberOptions& options = {}) {
  ::pluss::detail::nn::require_defined_(prediction, "nn::losses::Huber");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::Huber");
  ::pluss::detail::nn::require_training_float_dtype_(prediction.scalar_type(), "nn::losses::Huber", "prediction");
  ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::Huber", "target");
  detail::require_same_platform_(prediction, target, "nn::losses::Huber");
  PLUSS_CHECK(
      options.delta > 0.0 && std::isfinite(options.delta),
      "pluss::nn::losses::Huber: delta must be positive and finite");

  const Series err = prediction.add(target.neg());
  const Series abs_err = err.abs();
  const Series quadratic = abs_err.clamp(0.0, options.delta);
  const Series linear = abs_err.sub(quadratic);
  const Series per = quadratic.square().mul(0.5).add(linear.mul(options.delta));
  return detail::reduce_loss_(per, options.reduction, "nn::losses::Huber");
}

inline Series L1(const Series& prediction, const Series& target, const L1Options& options = {}) {
  ::pluss::detail::nn::require_defined_(prediction, "nn::losses::L1");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::L1");
  ::pluss::detail::nn::require_training_float_dtype_(prediction.scalar_type(), "nn::losses::L1", "prediction");
  ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::L1", "target");
  detail::require_same_platform_(prediction, target, "nn::losses::L1");
  const Series per = prediction.sub(target).abs();
  return detail::reduce_loss_(per, options.reduction, "nn::losses::L1");
}

inline Series SmoothL1(const Series& prediction, const Series& target, const SmoothL1Options& options = {}) {
  ::pluss::detail::nn::require_defined_(prediction, "nn::losses::SmoothL1");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::SmoothL1");
  ::pluss::detail::nn::require_training_float_dtype_(prediction.scalar_type(), "nn::losses::SmoothL1", "prediction");
  ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::SmoothL1", "target");
  detail::require_same_platform_(prediction, target, "nn::losses::SmoothL1");
  PLUSS_CHECK(
      options.beta >= 0.0 && std::isfinite(options.beta),
      "pluss::nn::losses::SmoothL1: beta must be finite and non-negative");
  if (options.beta == 0.0) {
    return L1(prediction, target, L1Options{.reduction = options.reduction});
  }
  const Series err = prediction.sub(target);
  const Series abs_err = err.abs();
  const Series quadratic = err.square().mul(0.5 / options.beta);
  const Series linear = abs_err.sub(0.5 * options.beta);
  const Series per = abs_err.lt(options.beta).where(quadratic, linear);
  return detail::reduce_loss_(per, options.reduction, "nn::losses::SmoothL1");
}

inline Series BCEWithLogits(const Series& logits, const Series& target, const BCEWithLogitsOptions& options = {}) {
  ::pluss::detail::nn::require_defined_(logits, "nn::losses::BCEWithLogits");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::BCEWithLogits");
  ::pluss::detail::nn::require_training_float_dtype_(logits.scalar_type(), "nn::losses::BCEWithLogits", "logits");
  ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::BCEWithLogits", "target");
  detail::require_same_platform_(logits, target, "nn::losses::BCEWithLogits");
  detail::require_same_shape_(logits, target, "nn::losses::BCEWithLogits");

  const Series tgt = (target.scalar_type() == logits.scalar_type()) ? target : target.to(logits.scalar_type());
  const Series base = logits.neg().softplus();
  const Series one = detail::scalar_on_(1.0, logits);

  Series log_weight = one;
  if (options.positive_class_weight.defined()) {
    detail::require_same_platform_(options.positive_class_weight, logits, "nn::losses::BCEWithLogits");
    ::pluss::detail::nn::require_training_float_dtype_(
        options.positive_class_weight.scalar_type(), "nn::losses::BCEWithLogits", "positive_class_weight");
    const Series pos_w = (options.positive_class_weight.scalar_type() == logits.scalar_type())
                             ? options.positive_class_weight
                             : options.positive_class_weight.to(logits.scalar_type());
    log_weight = pos_w.sub(one).mul(tgt).add(one);
  }

  Series per = logits.mul(one.sub(tgt)).add(log_weight.mul(base));
  if (options.weight.defined()) {
    detail::require_same_platform_(options.weight, logits, "nn::losses::BCEWithLogits");
    ::pluss::detail::nn::require_training_float_dtype_(options.weight.scalar_type(), "nn::losses::BCEWithLogits", "weight");
    const Series w = (options.weight.scalar_type() == logits.scalar_type())
                         ? options.weight
                         : options.weight.to(logits.scalar_type());
    per = per.mul(w);
  }
  return detail::reduce_loss_(per, options.reduction, "nn::losses::BCEWithLogits");
}

inline Series NLL(const Series& log_probs, const Series& target, const NLLOptions& options = {}) {
  ::pluss::detail::nn::require_defined_(log_probs, "nn::losses::NLL");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::NLL");
  ::pluss::detail::nn::require_training_float_dtype_(log_probs.scalar_type(), "nn::losses::NLL", "input");
  detail::require_same_platform_(log_probs, target, "nn::losses::NLL");
  PLUSS_CHECK(
      target.scalar_type() == ScalarType::Int32 || target.scalar_type() == ScalarType::Int64,
      "pluss::nn::losses::NLL: target must contain Int32/Int64 class indices");

  const std::int64_t class_dim = detail::normalize_dim_(options.class_dim, log_probs.dim(), "nn::losses::NLL");
  detail::require_target_matches_logits_except_class_(target, log_probs, class_dim, "nn::losses::NLL");
  detail::require_class_weight_(options.class_weight, log_probs, class_dim, "nn::losses::NLL");

  const Series target_i64 = target.to(ScalarType::Int64);
  Series active_mask{};
  Series safe_target = target_i64;
  if (options.use_ignore_index) {
    active_mask = target_i64.ne(options.ignore_index);
    safe_target = active_mask.where(target_i64, pluss::zeros_like(target_i64, ScalarType::Int64));
  }

  const Series gather_index = safe_target.unsqueeze(class_dim);
  Series per = log_probs.gather(class_dim, gather_index).squeeze(class_dim).neg();

  const Series class_weight = detail::class_weight_cast_(options.class_weight, log_probs);
  Series class_scale = detail::class_weight_for_index_target_(class_weight, safe_target, log_probs);
  per = per.mul(class_scale);

  Series denom{};
  if (options.class_weight.defined()) {
    denom = class_scale.sum();
  } else {
    denom = detail::scalar_on_(static_cast<double>(target.numel()), per);
  }
  if (options.use_ignore_index) {
    per = active_mask.where(per, pluss::zeros_like(per, per.scalar_type()));
    const Series active_f = active_mask.to(per.scalar_type());
    if (options.class_weight.defined()) {
      denom = class_scale.mul(active_f).sum();
    } else {
      denom = active_f.sum();
    }
  }

  return detail::reduce_loss_with_denom_(per, options.reduction, denom, "nn::losses::NLL");
}

inline Series CrossEntropy(const Series& logits, const Series& target, const CrossEntropyOptions& options = {}) {
  ::pluss::detail::nn::require_defined_(logits, "nn::losses::CrossEntropy");
  ::pluss::detail::nn::require_defined_(target, "nn::losses::CrossEntropy");
  ::pluss::detail::nn::require_training_float_dtype_(logits.scalar_type(), "nn::losses::CrossEntropy", "logits");
  detail::require_same_platform_(logits, target, "nn::losses::CrossEntropy");
  PLUSS_CHECK(
      options.label_smoothing >= 0.0 && options.label_smoothing <= 1.0 && std::isfinite(options.label_smoothing),
      "pluss::nn::losses::CrossEntropy: label_smoothing must be finite in [0,1]");

  const std::int64_t class_dim = detail::normalize_dim_(options.class_dim, logits.dim(), "nn::losses::CrossEntropy");
  detail::require_class_weight_(options.class_weight, logits, class_dim, "nn::losses::CrossEntropy");
  const Series log_probs = detail::stable_log_probs_(logits, class_dim);

  const bool target_is_prob = pluss::codem::dtype_of(target.scalar_type()).is_floating();
  if (target_is_prob) {
    ::pluss::detail::nn::require_training_float_dtype_(target.scalar_type(), "nn::losses::CrossEntropy", "target");
    PLUSS_CHECK(
        !options.use_ignore_index,
        "pluss::nn::losses::CrossEntropy: ignore_index is only valid for class-index targets");
    detail::require_same_shape_(target, logits, "nn::losses::CrossEntropy");
    Series probs = (target.scalar_type() == logits.scalar_type()) ? target : target.to(logits.scalar_type());
    if (options.label_smoothing > 0.0) {
      const double inv_classes = 1.0 / static_cast<double>(logits.sizes()[class_dim]);
      probs = probs.mul(1.0 - options.label_smoothing).add(options.label_smoothing * inv_classes);
    }
    if (options.class_weight.defined()) {
      const Series w = detail::class_weight_cast_(options.class_weight, logits);
      probs = probs.mul(detail::class_weight_view_(w, logits, class_dim));
    }
    const Series per = probs.mul(log_probs).sum(class_dim).neg();
    return detail::reduce_loss_(per, options.reduction, "nn::losses::CrossEntropy");
  }

  PLUSS_CHECK(
      target.scalar_type() == ScalarType::Int32 || target.scalar_type() == ScalarType::Int64,
      "pluss::nn::losses::CrossEntropy: target must be Int32/Int64 class indices or training-float probabilities");

  if (options.label_smoothing == 0.0) {
    return NLL(
        log_probs,
        target,
        NLLOptions{
            .reduction = options.reduction,
            .class_dim = class_dim,
            .class_weight = options.class_weight,
            .use_ignore_index = options.use_ignore_index,
            .ignore_index = options.ignore_index,
        });
  }

  detail::require_target_matches_logits_except_class_(target, logits, class_dim, "nn::losses::CrossEntropy");
  const Series target_i64 = target.to(ScalarType::Int64);
  Series active_mask{};
  Series safe_target = target_i64;
  if (options.use_ignore_index) {
    active_mask = target_i64.ne(options.ignore_index);
    safe_target = active_mask.where(target_i64, pluss::zeros_like(target_i64, ScalarType::Int64));
  }

  const Series gather_index = safe_target.unsqueeze(class_dim);
  Series nll_per = log_probs.gather(class_dim, gather_index).squeeze(class_dim).neg();

  const Series class_weight = detail::class_weight_cast_(options.class_weight, logits);
  Series class_scale = detail::class_weight_for_index_target_(class_weight, safe_target, log_probs);
  nll_per = nll_per.mul(class_scale);

  Series smooth_per{};
  if (class_weight.defined()) {
    const Series cw_norm = class_weight.div(class_weight.sum().clamp_min(1e-12));
    smooth_per = log_probs.mul(detail::class_weight_view_(cw_norm, logits, class_dim)).sum(class_dim).neg();
  } else {
    smooth_per = log_probs.mean(class_dim).neg();
  }

  Series per = nll_per.mul(1.0 - options.label_smoothing).add(smooth_per.mul(options.label_smoothing));
  Series denom = options.class_weight.defined()
                     ? class_scale.sum()
                     : detail::scalar_on_(static_cast<double>(target.numel()), per);

  if (options.use_ignore_index) {
    per = active_mask.where(per, pluss::zeros_like(per, per.scalar_type()));
    const Series active_f = active_mask.to(per.scalar_type());
    if (options.class_weight.defined()) {
      denom = class_scale.mul(active_f).sum();
    } else {
      denom = active_f.sum();
    }
  }

  return detail::reduce_loss_with_denom_(per, options.reduction, denom, "nn::losses::CrossEntropy");
}

} // namespace losses

} // namespace pluss::ai::nn
