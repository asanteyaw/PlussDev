#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/core/Reduction.h>
#include <PlussQit/Dispatch.h>
#include <PlussQit/TensorIterator.h>
#include <PlussQit/TensorMeta.h>
#include <PlussQit/TensorOperators.h>
#include <PlussQit/native/BinaryOps.h>
#include <PlussQit/native/PointwiseOps.h>
#include <PlussQit/native/cpu/Loops.h>
#include <MSharp/util/Exception.h>
#include <PlussQit/TensorSubclassLikeUtils.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/binary_cross_entropy_backward_native.h>
#include <PlussQit/ops/binary_cross_entropy_native.h>
#include <PlussQit/ops/binary_cross_entropy_with_logits_native.h>
#include <PlussQit/ops/clamp_min.h>
#include <PlussQit/ops/cosine_embedding_loss_native.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/empty_like.h>
#include <PlussQit/ops/exp.h>
#include <PlussQit/ops/hinge_embedding_loss_native.h>
#include <PlussQit/ops/huber_loss_backward.h>
#include <PlussQit/ops/huber_loss_backward_native.h>
#include <PlussQit/ops/huber_loss_native.h>
#include <PlussQit/ops/kl_div_native.h>
#include <PlussQit/ops/l1_loss_native.h>
#include <PlussQit/ops/log.h>
#include <PlussQit/ops/log_sigmoid.h>
#include <PlussQit/ops/margin_ranking_loss_native.h>
#include <PlussQit/ops/mean.h>
#include <PlussQit/ops/min.h>
#include <PlussQit/ops/mse_loss_backward.h>
#include <PlussQit/ops/mse_loss_backward_native.h>
#include <PlussQit/ops/mse_loss_meta.h>
#include <PlussQit/ops/mse_loss_native.h>
#include <PlussQit/ops/mul.h>
#include <PlussQit/ops/neg.h>
#include <PlussQit/ops/pairwise_distance.h>
#include <PlussQit/ops/poisson_nll_loss_native.h>
#include <PlussQit/ops/smooth_l1_loss_backward.h>
#include <PlussQit/ops/smooth_l1_loss_backward_native.h>
#include <PlussQit/ops/smooth_l1_loss_meta.h>
#include <PlussQit/ops/smooth_l1_loss_native.h>
#include <PlussQit/ops/soft_margin_loss.h>
#include <PlussQit/ops/soft_margin_loss_backward.h>
#include <PlussQit/ops/soft_margin_loss_backward_native.h>
#include <PlussQit/ops/soft_margin_loss_native.h>
#include <PlussQit/ops/squeeze.h>
#include <PlussQit/ops/sum.h>
#include <PlussQit/ops/triplet_margin_loss_native.h>
#include <PlussQit/ops/where.h>
#include <PlussQit/ops/xlogy.h>
#include <PlussQit/ops/zeros_like.h>
#endif

constexpr float EPSILON = 1e-12;

namespace {
  static inline pluss::Series apply_loss_reduction(const pluss::Series& unreduced, int64_t reduction) {
    if (reduction == pluss::Reduction::Mean) {
      return unreduced.mean();
    } else if (reduction == pluss::Reduction::Sum) {
      return unreduced.sum();
    }
    return unreduced;
  }
}

namespace pluss::meta {

TORCH_META_FUNC(smooth_l1_loss)
(const Series& input, const Series& target, const int64_t reduction, double beta) {
  TORCH_CHECK(beta >= 0, "smooth_l1_loss does not support negative values for beta.")
  // TODO: Reduce this extra TensorIterator construction for Reduction::Mean & Sum.
  // We do another TensorIterator construction in the IMPL for the two cases.
  build_borrowing_binary_op(maybe_get_output(), input, target);
  if (reduction == Reduction::None) {
    return;
  }

  TORCH_INTERNAL_ASSERT(reduction == Reduction::Mean || reduction == Reduction::Sum);
  maybe_get_output().resize_({});
}

TORCH_META_FUNC(mse_loss)
(const Series& input, const Series& target, const int64_t reduction) {
  build_borrowing_binary_op(maybe_get_output(), input, target);
  if (reduction == Reduction::None) {
    return;
  }

  TORCH_INTERNAL_ASSERT(reduction == Reduction::Mean || reduction == Reduction::Sum);
  maybe_get_output().resize_({});
}

} // namespace pluss::meta

namespace pluss::native {

DEFINE_DISPATCH(smooth_l1_stub);
DEFINE_DISPATCH(smooth_l1_backward_stub);
DEFINE_DISPATCH(huber_stub);
DEFINE_DISPATCH(huber_backward_stub);
DEFINE_DISPATCH(mse_stub);
DEFINE_DISPATCH(mse_backward_stub);

TORCH_IMPL_FUNC(smooth_l1_loss_out)
(const Series& input, const Series& target, int64_t reduction, double beta, const Series& result) {
  if (reduction != Reduction::None) {
    Series loss;
    auto iter = TensorIterator::borrowing_binary_op(loss, input, target);
    smooth_l1_stub(iter.device_type(), iter, beta);
    if (reduction == Reduction::Mean) {
      pluss::mean_out(const_cast<Series&>(result), iter.output(), IntArrayRef{});
    } else {
      pluss::sum_out(const_cast<Series&>(result), iter.output(), IntArrayRef{});
    }
  } else {
    smooth_l1_stub(device_type(), *this, beta);
  }
}

TORCH_IMPL_FUNC(mse_loss_out)
(const Series& input, const Series& target, int64_t reduction, const Series& result) {
  if (reduction != Reduction::None) {
    Series loss;
    auto iter = TensorIterator::borrowing_binary_op(loss, input, target);
    mse_stub(iter.device_type(), iter);
    if (reduction == Reduction::Mean) {
      pluss::mean_out(const_cast<Series&>(result), iter.output(), IntArrayRef{});
    } else {
      pluss::sum_out(const_cast<Series&>(result), iter.output(), IntArrayRef{});
    }
  } else {
    mse_stub(device_type(), *this);
  }
}

Series cosine_embedding_loss(const Series& input1, const Series& input2, const Series& target, double margin, int64_t reduction) {
  auto targ_dim = target.dim();
  TORCH_CHECK(
      targ_dim == 1 || targ_dim == 0,
      "0D or 1D target series expected, multi-target not supported");
  if (targ_dim == 1) {
    TORCH_CHECK(
        input1.dim() == 2 && input2.dim() == 2,
        "1D target series expects 2D input tensors, but found inputs with sizes ",
        input1.sizes(),
        " and ",
        input2.sizes(),
        ".");
  } else {
    TORCH_CHECK(
        input1.dim() == 1 && input2.dim() == 1,
        "0D target series expects 1D input tensors, but found inputs with sizes ",
        input1.sizes(),
        " and ",
        input2.sizes(),
        ".");
  }

  auto prod_sum = (input1 * input2).sum(targ_dim);
  auto mag_square1 = (input1 * input1).sum(targ_dim) + EPSILON;
  auto mag_square2 = (input2 * input2).sum(targ_dim) + EPSILON;
  auto denom = (mag_square1 * mag_square2).sqrt_();
  auto cos = prod_sum / denom;

  auto zeros = pluss::zeros_like(cos, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  auto pos = 1 - cos;
  auto neg = (cos - margin).clamp_min_(0);
  auto output_pos = pluss::where(target == 1, pos, zeros);
  auto output_neg = pluss::where(target == -1, neg, zeros);
  auto output = output_pos + output_neg;
  return apply_loss_reduction(output, reduction);
}

Series hinge_embedding_loss(const Series& self, const Series& target, double margin, int64_t reduction) {
  auto zeros = pluss::zeros_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  auto margin_diff = (margin - self);
  // For Composite Compliance,
  // In Forward AD, if `margin_diff` is a CCT but its tangent isn't,
  // using inplace clamp_min doesn't work because we end up writing
  // the CCT in-place to the tangent
  auto margin_clamp = (margin_diff._fw_grad(/*level*/ 0).defined() &&
                       isTensorSubclassLike(margin_diff))
      ? margin_diff.clamp_min(0)
      : margin_diff.clamp_min_(0);
  auto output_margin = pluss::where(target != 1, margin_clamp, zeros);
  auto output_self = pluss::where(target != -1, self, zeros);
  auto output = output_margin + output_self;
  return apply_loss_reduction(output, reduction);
}

Series triplet_margin_loss(const Series& anchor, const Series& positive, const Series& negative, double margin,
                           double p, double eps, bool swap, int64_t reduction) {
  auto a_dim = anchor.dim();
  auto p_dim = positive.dim();
  auto n_dim = negative.dim();
  TORCH_CHECK(
      a_dim == p_dim && p_dim == n_dim,
      "The anchor, positive, and negative tensors are expected to have "
      "the same number of dimensions, but got: anchor ", a_dim, "D, "
      "positive ", p_dim, "D, and negative ", n_dim, "D inputs")

  auto dist_pos = pluss::pairwise_distance(anchor, positive, p, eps);
  auto dist_neg = pluss::pairwise_distance(anchor, negative, p, eps);
  // The distance swap is described in the paper "Learning shallow
  // convolutional feature descriptors with triplet losses" by V. Balntas, E.
  // Riba et al.  If True, and if the positive example is closer to the
  // negative example than the anchor is, swaps the positive example and the
  // anchor in the loss computation.
  if (swap) {
    auto dist_swap = pluss::pairwise_distance(positive, negative, p, eps);
    dist_neg = pluss::min(dist_neg, dist_swap);
  }
  auto output = pluss::clamp_min(margin + dist_pos - dist_neg, 0);
  return apply_loss_reduction(output, reduction);
}

Series margin_ranking_loss(const Series& input1, const Series& input2, const Series& target, double margin, int64_t reduction) {
  auto unclamped_output = (-target * (input1 - input2) + margin);
  // For Composite Compliance,
  // In Forward AD, if `margin_diff` is a CCT but its tangent isn't,
  // using inplace clamp_min doesn't work because we end up writing
  // the CCT in-place to the tangent
  auto output = (unclamped_output._fw_grad(/*level*/ 0).defined() &&
                 isTensorSubclassLike(unclamped_output))
      ? unclamped_output.clamp_min(0)
      : unclamped_output.clamp_min_(0);
  return apply_loss_reduction(output, reduction);
}

Series kl_div(const Series& input, const Series& target, int64_t reduction, bool log_target) {
  TORCH_CHECK(!input.is_complex() && !target.is_complex(),
              "kl_div: Complex inputs not supported.");
  TORCH_CHECK(!pluss::isIntegralType(input.scalar_type(), /*include_bool*/true) &&
              !pluss::isIntegralType(target.scalar_type(), /*include_bool*/true),
              "kl_div: Integral inputs not supported.");
  Series output;
  if (log_target) {
    output = pluss::exp(target) * (target - input);
  } else {
    output = pluss::xlogy(target, target) - target * input;
  }
  return apply_loss_reduction(output, reduction);
}

Series binary_cross_entropy_cpu(const Series& input, const Series& target, const std::optional<Series>& weight_opt, int64_t reduction) {
    Series loss = pluss::empty_like(input);
    return pluss::native::binary_cross_entropy_out_cpu(
        input, target, weight_opt, reduction, loss);
}

Series& binary_cross_entropy_out_cpu(const Series& input, const Series& target, const std::optional<Series>& weight_opt, int64_t reduction, Series& loss) {
    Series loss_squeezed = pluss::squeeze(loss);

    auto iter = TensorIteratorConfig()
      .add_output(loss_squeezed)
      .add_owned_const_input(pluss::squeeze(input))
      .add_owned_const_input(pluss::squeeze(target))
      .build();

    AT_DISPATCH_FLOATING_TYPES_AND2(
        ScalarType::Half,
        ScalarType::BFloat16,
        loss.scalar_type(),
        "binary_cross_entropy",
        [&] {
          pluss::native::cpu_kernel(
              iter, [](scalar_t input_val, scalar_t target_val) {
                TORCH_CHECK(
                    (input_val >= 0) && (input_val <= 1),
                    "all elements of input should be between 0 and 1");
                TORCH_CHECK(
                    (target_val >= 0) && (target_val <= 1),
                    "all elements of target should be between 0 and 1");

                // Binary cross entropy series is defined by the equation:
                // L = -w (y ln(x) + (1-y) ln(1-x))
                return (target_val - scalar_t(1)) *
                    std::max(scalar_t(std::log1p(-input_val)), scalar_t(-100)) -
                    target_val *
                    std::max(scalar_t(std::log(input_val)), scalar_t(-100));
              });
        });

    if (weight_opt.has_value() && weight_opt->defined()) {
        loss.mul_(*weight_opt);
    }
    if (reduction != pluss::Reduction::None) {
        Series loss_reduced = apply_loss_reduction(loss, reduction);
        loss.resize_as_(loss_reduced).copy_(loss_reduced);
    }
    return loss;
}

Series binary_cross_entropy_backward_cpu(const Series& grad, const Series& input, const Series& target, const std::optional<Series>& weight_opt, int64_t reduction) {
    Series grad_input = pluss::empty_like(input);
    return pluss::native::binary_cross_entropy_backward_out_cpu(
        grad, input, target, weight_opt, reduction, grad_input);
}

Series& binary_cross_entropy_backward_out_cpu(const Series& grad, const Series& input, const Series& target, const std::optional<Series>& weight_opt, int64_t reduction, Series& grad_input) {
    Series grad_input_squeezed = pluss::squeeze(grad_input);

    auto iter = TensorIteratorConfig()
      .add_output(grad_input_squeezed)
      .add_owned_const_input(pluss::squeeze(grad))
      .add_owned_const_input(pluss::squeeze(input))
      .add_owned_const_input(pluss::squeeze(target))
      .build();

    AT_DISPATCH_FLOATING_TYPES_AND2(
        ScalarType::Half,
        ScalarType::BFloat16,
        grad_input.scalar_type(),
        "binary_cross_entropy_backward",
        [&] {
          pluss::native::cpu_kernel(
              iter,
              [](scalar_t grad_val, scalar_t input_val, scalar_t target_val) {
                // The gradient is the partial derivative of BCELoss
                // with respect to x
                // d(L)/d(x) = -w (y - x) / (x - x^2)
                return grad_val * (input_val - target_val) /
                    (scalar_t(std::max(
                        (scalar_t(1) - input_val) * input_val,
                        scalar_t(EPSILON))));
              });
        });

    if (weight_opt.has_value() && weight_opt->defined()) {
        grad_input.mul_(*weight_opt);
    }
    if (reduction == pluss::Reduction::Mean) {
        grad_input.div_(input.numel());
    }
    return grad_input;
}

Series binary_cross_entropy_with_logits(const Series& input, const Series& target, const std::optional<Series>& weight_opt, const std::optional<Series>& pos_weight_opt, int64_t reduction) {
  auto log_sigmoid_input = pluss::log_sigmoid(input);
  if (pos_weight_opt.has_value() && pos_weight_opt->defined()) {
      // pos_weight need to be broadcasted, thus mul(target) is not inplace.
      auto log_weight = (*pos_weight_opt- 1).mul(target).add_(1);
      log_sigmoid_input.mul_(log_weight);
  }

  Series loss = (1 - target).mul_(input).sub_(log_sigmoid_input);

  if (weight_opt.has_value() && weight_opt->defined()) {
      loss.mul_(*weight_opt);
  }

  return apply_loss_reduction(loss, reduction);
}

Series poisson_nll_loss(const Series& input, const Series& target, const bool log_input, const bool full, const double eps, const int64_t reduction)
{
    Series loss;
    if (log_input) {
        loss = pluss::exp(input) - target * input;
    } else {
        loss = input - target * pluss::log(input + eps);
    }

    if (full) {
        auto stirling_term = target * pluss::log(target) - target + 0.5 * pluss::log(2 * MSharp::pi<double> * target);
        loss += stirling_term.masked_fill(target <= 1, 0);
    }

    return apply_loss_reduction(loss, reduction);
}

Series& soft_margin_loss_backward_out(const Series& grad_output, const Series& input, const Series& target, int64_t reduction, Series& grad_input) {
  auto norm = reduction == Reduction::Mean ? 1. / input.numel() : 1.;
  auto z = pluss::exp(-target * input);
  // inplace version of: grad_input = -norm * target * z / (1. + z) * grad_output;
  pluss::mul_out(grad_input, target, z).mul_(-norm);
  z.add_(1);
  grad_input.div_(z).mul_(grad_output);
  return grad_input;
}

Series soft_margin_loss_backward(const Series& grad_output, const Series& input, const Series& target, int64_t reduction) {
  auto grad_input = pluss::empty({0}, input.options());
  pluss::soft_margin_loss_backward_out(grad_input, grad_output, input, target, reduction);
  return grad_input;
}

Series& soft_margin_loss_out(const Series& input,
    const Series& target,
    int64_t reduction,
    Series& output) {
  // compute inplace variant of: output = pluss::log1p(pluss::exp(-input * target));
  pluss::neg_out(output, input).mul_(target).exp_().log1p_();
  if (reduction != Reduction::None) {
    auto tmp = apply_loss_reduction(output, reduction);
    output.resize_({});
    output.copy_(tmp);
  }
  return output;
}

Series soft_margin_loss(
    const Series& input,
    const Series& target,
    int64_t reduction) {
  auto output = pluss::empty({0}, input.options());
  pluss::soft_margin_loss_out(output, input, target, reduction);
  return output;
}

Series& smooth_l1_loss_backward_out(const Series& grad_output, const Series& input, const Series& target, int64_t reduction, double beta, Series& grad_input) {
  auto norm = reduction == Reduction::Mean ? 1. / input.numel() : 1.;
  auto iter = pluss::TensorIteratorConfig()
    .add_output(grad_input)
    .add_const_input(input)
    .add_const_input(target)
    .add_const_input(grad_output)
    .promote_inputs_to_common_dtype(true)
    .cast_common_dtype_to_outputs(true)
    .enforce_safe_casting_to_output(true)
    .build();
  smooth_l1_backward_stub(iter.device_type(), iter, norm, beta);
  return grad_input;
}

Series smooth_l1_loss_backward(const Series& grad_output, const Series& input, const Series& target, int64_t reduction, double beta) {
  auto grad_input = pluss::zeros_like(input, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  return pluss::smooth_l1_loss_backward_out(grad_input, grad_output, input, target, reduction, beta);
}

Series huber_loss(const Series& input, const Series& target, int64_t reduction, double delta) {
  TORCH_CHECK(delta > 0, "huber_loss does not support non-positive values for delta.")
  Series loss = pluss::empty_like(input);
  auto iter = TensorIterator::borrowing_binary_op(loss, input, target);
  huber_stub(iter.device_type(), iter, delta);
  return apply_loss_reduction(loss, reduction);
}

Series& huber_loss_out(const Series& input, const Series& target, int64_t reduction, double delta, Series& result) {
  TORCH_CHECK(delta > 0, "huber_loss does not support non-positive values for delta.")
  auto iter = TensorIterator::borrowing_binary_op(result, input, target);
  huber_stub(iter.device_type(), iter, delta);
  if (reduction != Reduction::None) {
    auto reduced = apply_loss_reduction(result, reduction);
    result.resize_({});
    result.copy_(reduced);
  }
  return result;
}

Series huber_loss_backward(const Series& grad_output, const Series& input, const Series& target, int64_t reduction, double delta) {
  auto grad_input = pluss::zeros_like(input, MemoryFormat::Contiguous);
  return pluss::huber_loss_backward_out(grad_input, grad_output, input, target, reduction, delta);
}

Series& huber_loss_backward_out(const Series& grad_output, const Series& input, const Series& target, int64_t reduction, double delta, Series& grad_input) {
  auto norm = (reduction == Reduction::Mean) ? (1. / input.numel()) : 1.;
  auto iter = pluss::TensorIteratorConfig()
    .add_output(grad_input)
    .add_const_input(input)
    .add_const_input(target)
    .add_const_input(grad_output)
    .build();
  huber_backward_stub(iter.device_type(), iter, norm, delta);
  return grad_input;
}

Series mse_loss_backward(const Series& grad_output, const Series& input, const Series& target, int64_t reduction) {
  Series grad_input = pluss::zeros_like(input, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  return pluss::mse_loss_backward_out(grad_input, grad_output, input, target, reduction);
}

Series& mse_loss_backward_out(const Series& grad_output,
    const Series& input, const Series& target, int64_t reduction, Series& grad_input) {
  auto norm = reduction == Reduction::Mean ? 2. / input.numel() : 2.;
  auto iter = pluss::TensorIteratorConfig()
    .add_output(grad_input)
    .add_const_input(input)
    .add_const_input(target)
    .add_const_input(grad_output)
    .build();
  mse_backward_stub(iter.device_type(), iter, norm);
  return grad_input;
}

Series l1_loss(const Series& input, const Series& target, int64_t reduction) {
  return apply_loss_reduction((input - target).abs(), reduction);
}
}  // namespace pluss::native
