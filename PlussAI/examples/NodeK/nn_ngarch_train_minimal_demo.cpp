#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kPi = 3.141592653589793238462643383279502884;

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

Series scalar_like(const Series& like, double v) {
  return pluss::make_series(v, Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
}

struct NGARCHModuleImpl final : pluss::nn::Module {
  Series raw_omega = pluss::series::undefined();
  Series raw_alpha = pluss::series::undefined();
  Series raw_phi = pluss::series::undefined();
  Series raw_lambda = pluss::series::undefined();
  Series raw_gamma = pluss::series::undefined();

  explicit NGARCHModuleImpl(const Config& cfg)
      : Module(true),
        raw_omega(pluss::make_series(-2.7, cfg)),
        raw_alpha(pluss::make_series(0.05, cfg)),
        raw_phi(pluss::make_series(1.3, cfg)),
        raw_lambda(pluss::make_series(0.08, cfg)),
        raw_gamma(pluss::make_series(-0.30, cfg)) {
    parameter_record("raw_omega", raw_omega);
    parameter_record("raw_alpha", raw_alpha);
    parameter_record("raw_phi", raw_phi);
    parameter_record("raw_lambda", raw_lambda);
    parameter_record("raw_gamma", raw_gamma);
  }

  std::tuple<Series, Series, Series, Series, Series> transformed() const {
    const Series eps = scalar_like(raw_omega, 1e-8);
    const Series omega = raw_omega.softplus().add(eps);
    const Series alpha = raw_alpha.sigmoid().mul(0.35);
    const Series phi = raw_phi.sigmoid().mul(0.95);
    const Series lambda = raw_lambda;
    const Series gamma = raw_gamma.tanh().mul(0.8);
    return {omega, alpha, phi, lambda, gamma};
  }

  std::pair<Series, Series> forward(const Series& y) const {
    const Series yy = (y.dim() == 1) ? y : y.flatten(0, -1);

    const auto [omega, alpha, phi, lambda, gamma] = transformed();
    const std::int64_t n = yy.sizes()[0];

    std::vector<Series> z_hist;
    std::vector<Series> h_hist;
    z_hist.reserve(static_cast<std::size_t>(n));
    h_hist.reserve(static_cast<std::size_t>(n));

    Series h_prev = yy.square().mean().clamp_min(1e-8);
    Series z_prev = scalar_like(yy, 0.0);

    for (std::int64_t t = 0; t < n; ++t) {
      Series h_t = omega.add(alpha.mul(h_prev).mul(z_prev.sub(gamma).square())).add(phi.mul(h_prev));
      h_t = h_t.clamp_min(1e-8);
      const Series sqrt_h = h_t.sqrt();
      const Series y_t = yy.select(0, t);
      const Series z_t = y_t.add(h_t.mul(0.5)).sub(lambda.mul(sqrt_h)).div(sqrt_h);
      h_hist.push_back(h_t);
      z_hist.push_back(z_t);
      h_prev = h_t;
      z_prev = z_t;
    }

    return {pluss::series::pile(z_hist, 0).reshape({n}), pluss::series::pile(h_hist, 0).reshape({n})};
  }
};

PLUSS_MODULE(NGARCHModule);

Series ngarch_generate_returns(std::int64_t n, const Config& cfg) {
  const Series omega = pluss::make_series(0.06, cfg);
  const Series alpha = pluss::make_series(0.18, cfg);
  const Series phi = pluss::make_series(0.76, cfg);
  const Series lambda = pluss::make_series(0.12, cfg);
  const Series gamma = pluss::make_series(-0.25, cfg);

  const Series noise = pluss::series::randn({n}, cfg, 123456ULL);
  std::vector<Series> y_hist;
  y_hist.reserve(static_cast<std::size_t>(n));

  Series h_prev = pluss::make_series(0.12, cfg);
  Series z_prev = noise.select(0, 0);
  for (std::int64_t t = 0; t < n; ++t) {
    Series h_t = omega.add(alpha.mul(h_prev).mul(z_prev.sub(gamma).square())).add(phi.mul(h_prev));
    h_t = h_t.clamp_min(1e-8);
    const Series z_t = noise.select(0, t);
    const Series y_t = h_t.sqrt().mul(z_t.add(lambda)).sub(h_t.mul(0.5));
    y_hist.push_back(y_t);
    h_prev = h_t;
    z_prev = z_t;
  }
  return pluss::series::pile(y_hist, 0).reshape({n});
}

Series neg_log_likelihood(const Series& z, const Series& h) {
  const double c = std::log(2.0 * kPi);
  return h.log().add(c).add(z.square()).mul(0.5).sum();
}

} // namespace

int main() {
  const pluss::Platform platform = pluss::pCPU;
  const ScalarType dtype = ScalarType::Float32;
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);

  const Series y = ngarch_generate_returns(450, cfg);
  NGARCHModule model(cfg);
  model->train(true);
  model->to(platform, dtype);
  const auto params = model->parameters();

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 0.03;
  pluss::nn::optim::Adam optimizer(params, opt);

  for (int epoch = 1; epoch <= 350; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      const auto [z, h] = model->forward(y);
      return neg_log_likelihood(z, h);
    });

    if (epoch == 1 || epoch % 70 == 0) {
      std::cout << "epoch=" << epoch << " nll=" << as_f64(loss) << "\n";
    }
  }

  const auto [omega, alpha, phi, lambda, gamma] = model->transformed();
  std::cout << "platform=" << (platform == pluss::pCPU ? "cpu" : platform == pluss::pMPS ? "mps" : "cuda")
            << " dtype="
            << (dtype == ScalarType::Float32 ? "f32" : "other")
            << "\n";
  std::cout << "estimated omega=" << as_f64(omega) << " alpha=" << as_f64(alpha) << " phi=" << as_f64(phi)
            << " lambda=" << as_f64(lambda) << " gamma=" << as_f64(gamma) << "\n";
  std::cout << "target    omega=0.06 alpha=0.18 phi=0.76 lambda=0.12 gamma=-0.25\n";
  return 0;
}
