#pragma once

#include "../Module.h"

namespace pluss::ai::nn {

class PLUSS_API ReLUImpl final : public Module {
public:
  using Module::Module;
  Series forward(const Series& input) const { return input.relu(); }
  Series operator()(const Series& input) const { return forward(input); }
  ReLUImpl* operator->() noexcept { return this; }
  const ReLUImpl* operator->() const noexcept { return this; }
};
PLUSS_MODULE(ReLU);

class PLUSS_API SigmoidImpl final : public Module {
public:
  using Module::Module;
  Series forward(const Series& input) const { return input.sigmoid(); }
  Series operator()(const Series& input) const { return forward(input); }
  SigmoidImpl* operator->() noexcept { return this; }
  const SigmoidImpl* operator->() const noexcept { return this; }
};
PLUSS_MODULE(Sigmoid);

class PLUSS_API TanhImpl final : public Module {
public:
  using Module::Module;
  Series forward(const Series& input) const { return input.tanh(); }
  Series operator()(const Series& input) const { return forward(input); }
  TanhImpl* operator->() noexcept { return this; }
  const TanhImpl* operator->() const noexcept { return this; }
};
PLUSS_MODULE(Tanh);

struct SoftplusOptions {
  double beta = 1.0;
  double threshold = 20.0;
};

class PLUSS_API SoftplusImpl final : public Module {
public:
  explicit SoftplusImpl(SoftplusOptions options) : options_(options) {}
  explicit SoftplusImpl(double beta = 1.0, double threshold = 20.0)
      : options_(SoftplusOptions{beta, threshold}) {}

  Series forward(const Series& input) const { return input.softplus(options_.beta, options_.threshold); }
  Series operator()(const Series& input) const { return forward(input); }
  SoftplusImpl* operator->() noexcept { return this; }
  const SoftplusImpl* operator->() const noexcept { return this; }

  SoftplusOptions& options() noexcept { return options_; }
  const SoftplusOptions& options() const noexcept { return options_; }

private:
  SoftplusOptions options_{};
};
PLUSS_MODULE(Softplus);

struct LeakyReLUOptions {
  double negative_slope = 0.01;
};

class PLUSS_API LeakyReLUImpl final : public Module {
public:
  explicit LeakyReLUImpl(LeakyReLUOptions options) : options_(options) {}
  explicit LeakyReLUImpl(double negative_slope = 0.01) : options_(LeakyReLUOptions{negative_slope}) {}

  Series forward(const Series& input) const { return input.leaky_relu(options_.negative_slope); }
  Series operator()(const Series& input) const { return forward(input); }
  LeakyReLUImpl* operator->() noexcept { return this; }
  const LeakyReLUImpl* operator->() const noexcept { return this; }

  LeakyReLUOptions& options() noexcept { return options_; }
  const LeakyReLUOptions& options() const noexcept { return options_; }

private:
  LeakyReLUOptions options_{};
};
PLUSS_MODULE(LeakyReLU);

struct EluOptions {
  double alpha = 1.0;
};

// ELU, applied elementwise: x > 0 ? x : alpha * (exp(x) - 1). Runs through
// Series::elu's single fused reverse node rather than a hand-composed
// relu/clamp_max/exp/sub/mul chain. Promoted out of
// ice::models::TemporalFusionTransformer's originally-private building
// blocks (Ice/models/ts/tft_blocks.h) once a second consumer needed it --
// see ICE_BLUEPRINT.md's centralization note.
class PLUSS_API EluImpl final : public Module {
public:
  explicit EluImpl(EluOptions options) : options_(options) {}
  explicit EluImpl(double alpha = 1.0) : options_(EluOptions{alpha}) {}

  Series forward(const Series& input) const { return input.elu(options_.alpha); }
  Series operator()(const Series& input) const { return forward(input); }
  EluImpl* operator->() noexcept { return this; }
  const EluImpl* operator->() const noexcept { return this; }

  EluOptions& options() noexcept { return options_; }
  const EluOptions& options() const noexcept { return options_; }

private:
  EluOptions options_{};
};
PLUSS_MODULE(Elu);

// Free-function form, for callers (e.g. a GRN's own forward()) that want
// ELU inline without constructing/holding a Module.
[[nodiscard]] inline Series elu(const Series& x, double alpha = 1.0) { return x.elu(alpha); }

class PLUSS_API IdentityImpl final : public Module {
public:
  using Module::Module;
  Series forward(const Series& input) const { return input; }
  Series operator()(const Series& input) const { return forward(input); }
  IdentityImpl* operator->() noexcept { return this; }
  const IdentityImpl* operator->() const noexcept { return this; }
};
PLUSS_MODULE(Identity);

class PLUSS_API SiLUImpl final : public Module {
public:
  using Module::Module;
  Series forward(const Series& input) const { return input.silu(); }
  Series operator()(const Series& input) const { return forward(input); }
  SiLUImpl* operator->() noexcept { return this; }
  const SiLUImpl* operator->() const noexcept { return this; }
};
PLUSS_MODULE(SiLU);

struct GELUOptions {
  bool approximate = false;
};

class PLUSS_API GELUImpl final : public Module {
public:
  explicit GELUImpl(GELUOptions options) : options_(options) {}
  explicit GELUImpl(bool approximate = false) : options_(GELUOptions{approximate}) {}

  Series forward(const Series& input) const { return input.gelu(options_.approximate); }
  Series operator()(const Series& input) const { return forward(input); }
  GELUImpl* operator->() noexcept { return this; }
  const GELUImpl* operator->() const noexcept { return this; }

  GELUOptions& options() noexcept { return options_; }
  const GELUOptions& options() const noexcept { return options_; }

private:
  GELUOptions options_{};
};
PLUSS_MODULE(GELU);

struct HardshrinkOptions {
  double lambd = 0.5;
};

class PLUSS_API HardshrinkImpl final : public Module {
public:
  explicit HardshrinkImpl(HardshrinkOptions options) : options_(options) {}
  explicit HardshrinkImpl(double lambd = 0.5) : options_(HardshrinkOptions{lambd}) {}

  Series forward(const Series& input) const { return input.hardshrink(options_.lambd); }
  Series operator()(const Series& input) const { return forward(input); }
  HardshrinkImpl* operator->() noexcept { return this; }
  const HardshrinkImpl* operator->() const noexcept { return this; }

  HardshrinkOptions& options() noexcept { return options_; }
  const HardshrinkOptions& options() const noexcept { return options_; }

private:
  HardshrinkOptions options_{};
};
PLUSS_MODULE(Hardshrink);

struct SoftmaxOptions {
  std::int64_t dim = -1;
};

class PLUSS_API SoftmaxImpl final : public Module {
public:
  explicit SoftmaxImpl(SoftmaxOptions options) : options_(options) {}
  explicit SoftmaxImpl(std::int64_t dim = -1) : options_(SoftmaxOptions{dim}) {}

  Series forward(const Series& input) const { return input.softmax(options_.dim); }
  Series operator()(const Series& input) const { return forward(input); }
  SoftmaxImpl* operator->() noexcept { return this; }
  const SoftmaxImpl* operator->() const noexcept { return this; }

  SoftmaxOptions& options() noexcept { return options_; }
  const SoftmaxOptions& options() const noexcept { return options_; }

private:
  SoftmaxOptions options_{};
};
PLUSS_MODULE(Softmax);

struct LogSoftmaxOptions {
  std::int64_t dim = -1;
};

class PLUSS_API LogSoftmaxImpl final : public Module {
public:
  explicit LogSoftmaxImpl(LogSoftmaxOptions options) : options_(options) {}
  explicit LogSoftmaxImpl(std::int64_t dim = -1) : options_(LogSoftmaxOptions{dim}) {}

  Series forward(const Series& input) const { return input.log_softmax(options_.dim); }
  Series operator()(const Series& input) const { return forward(input); }
  LogSoftmaxImpl* operator->() noexcept { return this; }
  const LogSoftmaxImpl* operator->() const noexcept { return this; }

  LogSoftmaxOptions& options() noexcept { return options_; }
  const LogSoftmaxOptions& options() const noexcept { return options_; }

private:
  LogSoftmaxOptions options_{};
};
PLUSS_MODULE(LogSoftmax);

struct PReLUOptions {
  std::int64_t num_parameters = 1;
  double init = 0.25;
  Config cfg = ::pluss::detail::nn::default_cfg_();
};

// The one activation in this file with a learnable parameter -- follows the
// same allocate-then-register-then-fill-in-place construction pattern as
// the engine's other parameterized modules (Fusion, Conv, Embedding, ...):
// weight_ is allocated via Series::empty() and registered atomically, then
// reset_parameters() only ever mutates it in place via copy_(), so it can
// never end up registered-but-untracked.
class PLUSS_API PReLUImpl final : public Module {
public:
  explicit PReLUImpl(PReLUOptions options) : Module(true), options_(std::move(options)) {
    register_parameters_();
    reset_parameters();
  }
  explicit PReLUImpl(std::int64_t num_parameters = 1, double init = 0.25, Config cfg = ::pluss::detail::nn::default_cfg_())
      : PReLUImpl(PReLUOptions{num_parameters, init, std::move(cfg)}) {}

  void reset_parameters() {
    weight_.copy_(Series::full({options_.num_parameters}, options_.init, options_.cfg.dtype, options_.cfg.platform));
  }

  Series forward(const Series& input) const { return input.prelu(weight_); }
  Series operator()(const Series& input) const { return forward(input); }
  PReLUImpl* operator->() noexcept { return this; }
  const PReLUImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  PReLUOptions& options() noexcept { return options_; }
  const PReLUOptions& options() const noexcept { return options_; }

private:
  void register_parameters_() {
    weight_ = Series::empty({options_.num_parameters}, options_.cfg.dtype, options_.cfg.platform);
    this->parameter_record("weight", weight_);
  }

  PReLUOptions options_;
  Series weight_ = pluss::series::undefined();
};
PLUSS_MODULE(PReLU);

} // namespace pluss::ai::nn
