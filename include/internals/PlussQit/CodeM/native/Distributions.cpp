#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/Dispatch.h>
#include <PlussQit/TensorIterator.h>
#include <PlussQit/TensorOperators.h>
#include <MSharp/util/Exception.h>
#include <optional>

#include <PlussQit/CPUGeneratorImpl.h>
#include <PlussQit/core/DistributionsHelper.h>
#include <PlussQit/native/Distributions.h>
#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/native/UnaryOps.h>
#include <PlussQit/native/DistributionTemplates.h>
#include <PlussQit/NamedTensorUtils.h>
#include <PlussQit/native/cpu/Loops.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_dirichlet_grad_native.h>
#include <PlussQit/ops/_sample_dirichlet_native.h>
#include <PlussQit/ops/_standard_gamma_grad_native.h>
#include <PlussQit/ops/_standard_gamma_native.h>
#include <PlussQit/ops/_assert_async.h>
#include <PlussQit/ops/argmax.h>
#include <PlussQit/ops/bernoulli_native.h>
#include <PlussQit/ops/binomial_native.h>
#include <PlussQit/ops/cauchy_native.h>
#include <PlussQit/ops/div.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/empty_like.h>
#include <PlussQit/ops/exponential_native.h>
#include <PlussQit/ops/geometric_native.h>
#include <PlussQit/ops/log_normal_native.h>
#include <PlussQit/ops/multinomial_native.h>
#include <PlussQit/ops/normal_native.h>
#include <PlussQit/ops/poisson_native.h>
#include <PlussQit/ops/random_native.h>
#include <PlussQit/ops/topk.h>
#include <PlussQit/ops/uniform_native.h>
#include <PlussQit/ops/zeros.h>
#endif

#include <utility>

namespace {
/*
 * This section is a counterpart to Distributions.cu
 *
 */

// The function `sample_poisson`
// is adapted from Numpy's distributions.c implementation.
// It is MIT licensed, so here is the copyright:

/* Copyright 2005 Robert Kern (robert.kern@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


int64_t sample_poisson(double lambda, pluss::CPUGeneratorImpl* generator) {
  TORCH_CHECK(lambda >= 0, "invalid Poisson rate, expected rate to be non-negative");
  pluss::uniform_real_distribution<double> standard_uniform(0.0, 1.0);
  if (lambda >= 10) {
    // transformed rejection method, (Hoermann, 1993)

    double slam = std::sqrt(lambda);
    double loglam = std::log(lambda);
    double b = 0.931 + 2.53 * slam;
    double a = -0.059 + 0.02483 * b;
    double invalpha = 1.1239 + 1.1328 / (b - 3.4);
    double vr = 0.9277 - 3.6224 / (b - 2);

    while (true) {
      double U = standard_uniform(generator) - 0.5;
      double V = standard_uniform(generator);
      double us = 0.5 - std::fabs(U);
      auto k = std::floor((2 * a / us + b) * U + lambda + 0.43);
      if ((us >= 0.07) && (V <= vr)) {
        return static_cast<int64_t>(k);
      }
      if ((k < 0) || ((us < 0.013) && (V > us))) {
        continue;
      }
      if ((std::log(V) + std::log(invalpha) - std::log(a / (us * us) + b)) <=
          (-lambda + k * loglam - std::lgamma(k + 1))) {
        return static_cast<int64_t>(k);
      }
    }
  } else if (lambda == 0) {
    return 0;
  } else {
    auto enlam = std::exp(-lambda);
    int64_t X = 0;
    auto prod = 1.0;
    while (true) {
      auto U = standard_uniform(generator);
      prod *= U;
      if (prod > enlam) {
        X += 1;
      } else {
        return X;
      }
    }
  }
}

} // namespace

namespace pluss::native {

DEFINE_DISPATCH(bernoulli_tensor_stub);
DEFINE_DISPATCH(bernoulli_scalar_stub);
DEFINE_DISPATCH(cauchy_stub);
DEFINE_DISPATCH(exponential_stub);
DEFINE_DISPATCH(multinomial_with_replacement_stub);
DEFINE_DISPATCH(geometric_stub);
DEFINE_DISPATCH(log_normal_stub);
DEFINE_DISPATCH(uniform_stub);
DEFINE_DISPATCH(normal_stub);
DEFINE_DISPATCH(random_stub);
DEFINE_DISPATCH(random_from_to_stub);
DEFINE_DISPATCH(random_full_64_bits_range_stub);

// ==================================================== Bernoulli =====================================================

template<typename RNG>
struct BernoulliStub {
  void operator()(Series& self, const Series& p_, std::optional<Generator> gen) {
    bernoulli_tensor_stub(self.device().type(), self, p_, gen);
  }

  void operator()(Series& self, double p, std::optional<Generator> gen) {
    bernoulli_scalar_stub(self.device().type(), self, p, gen);
  }
};

Series bernoulli(const Series& self, std::optional<Generator> gen) {
  Series result = pluss::empty_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  result.bernoulli_(self, std::move(gen));
  return result;
}

Series bernoulli(const Series& self, double p, std::optional<Generator> gen) {
  Series result = pluss::empty_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  result.bernoulli_(p, std::move(gen));
  return result;
}

Series& bernoulli_out(const Series& self, std::optional<Generator> gen, Series& result) {
  return pluss::native::templates::bernoulli_out_impl<BernoulliStub, Generator>(result, self, std::move(gen));
}

Series& bernoulli_(Series& self, const Series& p_, std::optional<Generator> gen) {
  return pluss::native::templates::bernoulli_impl_<BernoulliStub, Generator>(self, p_, std::move(gen));
}

Series& bernoulli_(Series& self, double p, std::optional<Generator> gen) {
  return pluss::native::templates::bernoulli_impl_<BernoulliStub, Generator>(self, p, std::move(gen));
}

// ================================================== LogNormal =======================================================

template<typename RNG>
struct LogNormalStub {
  void operator()(TensorIteratorBase& iter, double mean, double std, std::optional<Generator> gen) {
    log_normal_stub(iter.device_type(), iter, mean, std, gen);
  }
};

Series& log_normal_(Series& self, double mean, double std, std::optional<Generator> gen) {
  return pluss::native::templates::log_normal_impl_<LogNormalStub, Generator>(self, mean, std, std::move(gen));
}

// ==================================================== Cauchy ========================================================

template<typename RNG>
struct CauchyStub {
  void operator()(TensorIteratorBase& iter, double median, double sigma, std::optional<Generator> gen) {
    cauchy_stub(iter.device_type(), iter, median, sigma, gen);
  }
};

Series& cauchy_(Series& self, double median, double sigma, std::optional<Generator> gen) {
  return pluss::native::templates::cauchy_impl_<CauchyStub, Generator>(self, median, sigma, std::move(gen));
}

// ================================================== Exponential =====================================================

template<typename RNG>
struct ExponentialStub {
  void operator()(TensorIteratorBase& iter, double lambda, std::optional<Generator> gen) {
    exponential_stub(iter.device_type(), iter, lambda, gen);
  }
};

Series& exponential_(Series& self, double lambda, std::optional<Generator> gen) {
  return pluss::native::templates::exponential_impl_<ExponentialStub, Generator>(self, lambda, std::move(gen));
}

// =================================================== Geometric ======================================================

template<typename RNG>
struct GeometricStub {
  void operator()(TensorIteratorBase& iter, double p, std::optional<Generator> gen) {
    geometric_stub(iter.device_type(), iter, p, gen);
  }
};

Series& geometric_(Series& self, double p, std::optional<Generator> gen) {
  return pluss::native::templates::geometric_impl_<GeometricStub, Generator>(self, p, std::move(gen));
}

// ==================================================== Uniform =======================================================

template<typename RNG>
struct UniformStub {
  void operator()(TensorIteratorBase& iter, double from, double to, std::optional<Generator> gen) {
    uniform_stub(iter.device_type(), iter, from, to, gen);
  }
};

template<typename RNG>
struct UniformMeta {
  // No-op!
  void operator()(TensorIteratorBase& iter, double from, double to, std::optional<Generator> gen) {
  }
};

Series& uniform_(Series& self, double from, double to, std::optional<Generator> gen) {
  return pluss::native::templates::uniform_impl_<UniformStub, Generator>(self, from, to, std::move(gen));
}

Series& uniform_meta_(Series& self, double from, double to, std::optional<Generator> gen) {
  return pluss::native::templates::uniform_impl_<UniformMeta, Generator>(self, from, to, std::move(gen));
}

// ==================================================== Normal ========================================================

template<typename RNG>
struct NormalStub {
  void operator()(Series& self, double mean, double std, std::optional<Generator> gen) {
    normal_stub(self.device().type(), self, mean, std, gen);
  }
};

template<typename RNG>
struct NormalMeta {
  // No-op!
  void operator()(Series& self, double mean, double std, std::optional<Generator> gen) {
  }
};

// inplace
Series& normal_(Series& self, double mean, double std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl_<NormalStub, Generator>(self, mean, std, std::move(gen));
}

Series& normal_meta_(Series& self, double mean, double std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl_<NormalMeta, Generator>(self, mean, std, std::move(gen));
}

// out series float
Series& normal_out(const Series& mean, double std, std::optional<Generator> gen, Series& output) {
  return pluss::native::templates::normal_out_impl<NormalStub, Generator>(output, mean, std, std::move(gen));
}

Series& normal_out_meta(const Series& mean, double std, std::optional<Generator> gen, Series& output) {
  return pluss::native::templates::normal_out_impl<NormalMeta, Generator>(output, mean, std, std::move(gen));
}

// out float series
Series& normal_out(double mean, const Series& std, std::optional<Generator> gen, Series& output) {
  return pluss::native::templates::normal_out_impl<NormalStub, Generator>(output, mean, std, std::move(gen));
}

Series& normal_out_meta(double mean, const Series& std, std::optional<Generator> gen, Series& output) {
  return pluss::native::templates::normal_out_impl<NormalMeta, Generator>(output, mean, std, std::move(gen));

}

// out series series
Series& normal_out(const Series& mean, const Series& std, std::optional<Generator> gen, Series& output) {
  return pluss::native::templates::normal_out_impl<NormalStub, Generator>(output, mean, std, std::move(gen));
}

Series& normal_out_meta(const Series& mean, const Series& std, std::optional<Generator> gen, Series& output) {
  return pluss::native::templates::normal_out_impl<NormalMeta, Generator>(output, mean, std, std::move(gen));
}

// functional series float
Series normal(const Series& mean, double std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl<NormalStub, Generator>(mean, std, std::move(gen));
}

Series normal_meta(const Series& mean, double std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl<NormalMeta, Generator>(mean, std, std::move(gen));
}

// functional float series
Series normal(double mean, const Series& std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl<NormalStub, Generator>(mean, std, std::move(gen));
}

Series normal_meta(double mean, const Series& std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl<NormalMeta, Generator>(mean, std, std::move(gen));
}

// functional series series
Series normal(const Series& mean, const Series& std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl<NormalStub, Generator>(mean, std, std::move(gen));
}

Series normal_meta(const Series& mean, const Series& std, std::optional<Generator> gen) {
  return pluss::native::templates::normal_impl<NormalMeta, Generator>(mean, std, std::move(gen));
}

// functional variant, only used by the functionalization pass.
Series normal_functional(const Series& self, double mean, double std, std::optional<pluss::Generator> generator) {
  return self.clone().normal_(mean, std, std::move(generator));
}

// ==================================================== Random ========================================================

template<typename RNG>
struct RandomStub {
  void operator()(TensorIteratorBase& iter, std::optional<Generator> gen) {
    random_stub(iter.device_type(), iter, gen);
  }
};

Series& random_(Series& self, std::optional<Generator> gen) {
  return pluss::native::templates::random_impl<RandomStub, Generator>(self, std::move(gen));
}

template<typename RNG>
struct RandomFromToStub {
  void operator()(TensorIteratorBase& iter, uint64_t range, int64_t from, std::optional<Generator> gen) {
    random_from_to_stub(iter.device_type(), iter, range, from, gen);
  }
  void operator()(TensorIteratorBase& iter, std::optional<Generator> gen) {
    random_full_64_bits_range_stub(iter.device_type(), iter, gen);
  }
};

Series& random_(Series& self, int64_t from, std::optional<int64_t> to, std::optional<Generator> gen) {
  return pluss::native::templates::random_from_to_impl<RandomFromToStub, Generator>(self, from, to, std::move(gen));
}

Series& random_(Series& self, int64_t to, std::optional<Generator> gen) {
  return random_(self, 0, to, std::move(gen));
}

Series& random_meta_(Series& self, std::optional<Generator> gen) {
  // No error checking yay
  return self;
}

Series& random_meta_(Series& self, int64_t from, std::optional<int64_t> to, std::optional<Generator> gen) {
  // No error checking yay
  return self;
}

Series& random_meta_(Series& self, int64_t to, std::optional<Generator> gen) {
  // No error checking yay
  return self;
}

// ====================================================================================================================

Series _standard_gamma_grad_cpu(const Series& self, const Series& output) {
  Series ret = pluss::empty(self.sizes(), self.options());
  auto iter = TensorIteratorConfig()
    .add_output(ret)
    .add_input(self)
    .add_input(output)
    .build();
  AT_DISPATCH_FLOATING_TYPES(self.scalar_type(), "_standard_gamma_grad_cpu", [&] {
    cpu_serial_kernel(iter, [](scalar_t self_val, scalar_t output_val) -> scalar_t{
      return standard_gamma_grad_one<scalar_t, double>(self_val, output_val);
    });
  });
  return ret;
}

Series _dirichlet_grad_cpu(const Series& x, const Series& alpha, const Series& total) {
  Series ret = pluss::empty(x.sizes(), x.options());
  auto iter = TensorIteratorConfig()
    .add_output(ret)
    .add_input(x)
    .add_input(alpha)
    .add_input(total)
    .build();
  AT_DISPATCH_FLOATING_TYPES(x.scalar_type(), "_dirichlet_grad_cpu", [&] {
    cpu_serial_kernel(iter, [](scalar_t x_val, scalar_t alpha_val, scalar_t total_val) -> scalar_t{
      return dirichlet_grad_one<scalar_t, double>(x_val, alpha_val, total_val);
    });
  });
  return ret;
}

/*
 * This section is a counterpart to Distributions.cu
 */

Series _s_binomial_cpu(const Series& count, const Series& prob, std::optional<Generator> gen) {
  Series ret = pluss::zeros(count.sizes(), count.options());
  auto iter = TensorIteratorConfig()
    .add_output(ret)
    .add_input(count)
    .add_input(prob)
    .build();
  AT_DISPATCH_FLOATING_TYPES(ret.scalar_type(), "binomial_cpu", [&] {
    CPUGeneratorImpl* generator = get_generator_or_default<CPUGeneratorImpl>(gen, detail::getDefaultCPUGenerator());
    // See Note [Acquire lock when using random generators]
    std::lock_guard<std::mutex> lock(generator->mutex_);
    cpu_serial_kernel(iter, [generator](scalar_t count_val, scalar_t prob_val) -> scalar_t{
      auto uniform_lambda = [generator] () {
        pluss::uniform_real_distribution<double> standard_uniform(0.0, 1.0);
        return standard_uniform(generator);
      };
      BaseSampler<double, decltype(uniform_lambda)> standard_uniform(uniform_lambda);

      auto sample = sample_binomial<scalar_t, double, decltype(uniform_lambda)>(count_val, prob_val, standard_uniform);
      return static_cast<scalar_t>(sample);
    });
  });
  return ret;
}

Series _s_poisson_cpu(const Series& lambda, std::optional<Generator> gen) {
  Series ret = pluss::zeros(lambda.sizes(), lambda.options());
  auto iter = TensorIteratorConfig()
    .add_output(ret)
    .add_input(lambda)
    .build();
  AT_DISPATCH_FLOATING_TYPES_AND2(pluss::ScalarType::BFloat16, pluss::ScalarType::Half, ret.scalar_type(), "poisson_cpu", [&] {
    CPUGeneratorImpl* generator = get_generator_or_default<CPUGeneratorImpl>(gen, detail::getDefaultCPUGenerator());
    // See Note [Acquire lock when using random generators]
    std::lock_guard<std::mutex> lock(generator->mutex_);
    cpu_serial_kernel(iter, [generator](scalar_t lambda_val) -> scalar_t{
      return static_cast<scalar_t>(sample_poisson(static_cast<double>(lambda_val), generator));
    });
  });
  return ret;
}

Series _s_gamma_cpu(const Series& alpha, std::optional<Generator> gen) {
  Series ret = pluss::zeros(alpha.sizes(), alpha.options());
  auto iter = TensorIteratorConfig()
    .add_output(ret)
    .add_input(alpha)
    .build();
  AT_DISPATCH_FLOATING_TYPES(ret.scalar_type(), "gamma_cpu", [&] {
    CPUGeneratorImpl* generator = get_generator_or_default<CPUGeneratorImpl>(gen, detail::getDefaultCPUGenerator());
    // See Note [Acquire lock when using random generators]
    std::lock_guard<std::mutex> lock(generator->mutex_);
    cpu_serial_kernel(iter, [generator](scalar_t alpha_val) -> scalar_t{
      auto uniform_lambda = [generator] () {
        pluss::uniform_real_distribution<double> standard_uniform(0.0, 1.0);
        return standard_uniform(generator);
      };
      BaseSampler<double, decltype(uniform_lambda)> standard_uniform(uniform_lambda);

      auto normal_lambda = [generator] () {
        pluss::normal_distribution<double> normal(0.0, 1.0);
        return normal(generator);
      };
      BaseSampler<double, decltype(normal_lambda)> standard_normal(normal_lambda);
      auto sample = sample_gamma<scalar_t, double, decltype(uniform_lambda), decltype(normal_lambda)>(alpha_val, standard_uniform, standard_normal);
      return std::max(std::numeric_limits<scalar_t>::min(), (scalar_t) sample);
    });
  });

  return ret;
}

Series _s_dirichlet_cpu(const Series& alpha, std::optional<Generator> gen) {
  Series ret = pluss::zeros(alpha.sizes(), alpha.options());
  AT_DISPATCH_FLOATING_TYPES(ret.scalar_type(), "dirichlet", [&] {
    Series gamma = pluss::zeros(alpha.sizes(), alpha.options().dtype(ScalarType::Double));
    CPUGeneratorImpl* generator = get_generator_or_default<CPUGeneratorImpl>(gen, detail::getDefaultCPUGenerator());
    // See Note [Acquire lock when using random generators]
    std::lock_guard<std::mutex> lock(generator->mutex_);
    /* Generate gamma sample by casting alpha to double to prevent underflow. */
    auto iter1 = TensorIteratorConfig()
      .add_output(gamma)
      .add_input(alpha)
      .check_all_same_dtype(false)
      .build();
    cpu_serial_kernel(iter1, [generator](scalar_t alpha_val) -> double{
      auto uniform_lambda = [generator] () {
        pluss::uniform_real_distribution<double> standard_uniform(0.0, 1.0);
        return standard_uniform(generator);
      };
      BaseSampler<double, decltype(uniform_lambda)> standard_uniform(uniform_lambda);

      auto normal_lambda = [generator] () {
        pluss::normal_distribution<double> normal(0.0, 1.0);
        return normal(generator);
      };
      BaseSampler<double, decltype(normal_lambda)> standard_normal(normal_lambda);
      auto sample = sample_gamma<double, double, decltype(uniform_lambda), decltype(normal_lambda)>
        (alpha_val, standard_uniform, standard_normal);
      return std::max(std::numeric_limits<double>::min(), sample);
    });
    /* Normalize and cast back to scalar_t. */
    Series gamma_sum = gamma.sum(-1, true).expand(alpha.sizes());
    auto iter2 = TensorIteratorConfig()
      .add_output(ret)
      .add_input(gamma)
      .add_input(gamma_sum)
      .check_all_same_dtype(false)
      .build();
    cpu_serial_kernel(iter2, [](double gamma_val, double gamma_sum_val) -> scalar_t{
      auto ret_val = gamma_val / gamma_sum_val;
      auto min_val = std::numeric_limits<scalar_t>::min();
      auto max_val = std::nexttoward(static_cast<scalar_t>(1.0f), 0.0f);
      return std::min(max_val, std::max(min_val, static_cast<scalar_t>(ret_val)));
    });
  });
  return ret;
}

/* The largest consecutive integer representable in float32 (2^24) */
constexpr int64_t FLOAT32_MAX_CONSECUTIVE_INT = 1 << (FLT_MANT_DIG);

Series& multinomial_out(const Series& self,
    int64_t n_sample,
    bool with_replacement,
    std::optional<Generator> gen,
    Series& result) {
  TORCH_CHECK(
      result.device() == self.device(),
      "multinomial arguments must have the same device");
  TORCH_CHECK(
      self.dim() > 0 && self.dim() <= 2, "prob_dist must be 1 or 2 dim");
  TORCH_CHECK(
      pluss::isFloatingType(self.scalar_type()),
      "multinomial only supports floating-point dtypes for input, got: ",
      self.scalar_type());
  TORCH_CHECK(result.scalar_type() == ScalarType::Long,
      "multinomial expects Long series out, got: ", result.scalar_type());
  TORCH_CHECK(n_sample > 0, "cannot sample n_sample <= 0 samples");
  int64_t n_categories = self.size(-1);
  TORCH_CHECK(with_replacement || (n_sample <= n_categories),
      "cannot sample n_sample > prob_dist.size(-1) samples without replacement");
  // Since the index series is float, numCategories cannot exceed max
  // float integer precision
  TORCH_CHECK(
      n_categories <= FLOAT32_MAX_CONSECUTIVE_INT,
      "number of categories cannot exceed 2^24");

  if (self.dim() == 1) {
    result.resize_({n_sample});
  } else {
    const int64_t n_dist = self.size(0);
    result.resize_({n_dist, n_sample});
  }
  if (result.numel() == 0) {
    return result;
  }

  // Fast-path for no replacement or if only one sample is drawn.
  // Reference:
  // https://github.com/pytorch/pytorch/issues/11931#issuecomment-625882503
  if (!with_replacement || n_sample == 1) {
    // Sanity checks on `self`.
    auto is_valid = ((self.max() < INFINITY) & (self.min() >= 0));
    pluss::_assert_async(is_valid, "probability series contains either `inf`, `nan` or element < 0");
    pluss::Series zero_prob_condition;
    if (self.dim() == 1){
      zero_prob_condition = (self.sum() == 0);
    } else {
      zero_prob_condition = (self.sum(1) == 0).any();
    }
    pluss::_assert_async(~zero_prob_condition, "invalid multinomial distribution (sum of probabilities <= 0)");

    // The algorithm is from gumbel softmax.
    // s = argmax( logp - log(-log(eps)) ) where eps ~ U(0, 1)
    // Here we can apply exp to the formula which will not affect result of
    // argmax or topk. Then we have
    // s = argmax( p / (-log(eps)) ) where eps ~ U(0, 1).
    // We can also simplify the formula above by
    // s = argmax( p / q ) where q ~ Exp(1)
    Series q = pluss::empty_like(self).exponential_(1, std::move(gen));
    // In theory the probability to generate 0 from exponential distribution is
    // 0. However, on CUDA side there is a protection to avoid 0s, but on CPU
    // side, there is a very low probability to generate 0 from
    // exponential<double>. The probability is about 2^(-DBL_MANT_DIG). We just
    // ignore it here, but there may be some risk to get invalid output on CPU.
    pluss::div_out(q, self, q);
    if (n_sample == 1) {
      pluss::argmax_out(result, q, /*dim=*/-1, /*keepdim=*/true);
    } else {
      Series vals = pluss::empty(result.sizes(), self.options());
      pluss::topk_out(vals, result, q, n_sample);
    }
    return result;
  }

  multinomial_with_replacement_stub(
      result.device().type(), result, self, n_sample, gen);
  return result;
}

Series multinomial(
    const Series& self,
    int64_t n_sample,
    bool with_replacement,
    std::optional<Generator> gen) {
  Series result = pluss::empty({0}, self.options().dtype(kLong));
  native::multinomial_out(self, n_sample, with_replacement, std::move(gen), result);
  return result;
}

} // namespace pluss::native
