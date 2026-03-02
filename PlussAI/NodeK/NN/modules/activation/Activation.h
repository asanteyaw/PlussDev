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
  using Module::Module;
  explicit SoftplusImpl(SoftplusOptions options = {}) : options_(options) {}

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
  using Module::Module;
  explicit LeakyReLUImpl(LeakyReLUOptions options = {}) : options_(options) {}

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

class PLUSS_API IdentityImpl final : public Module {
public:
  using Module::Module;
  Series forward(const Series& input) const { return input; }
  Series operator()(const Series& input) const { return forward(input); }
  IdentityImpl* operator->() noexcept { return this; }
  const IdentityImpl* operator->() const noexcept { return this; }
};
PLUSS_MODULE(Identity);

} // namespace pluss::ai::nn
