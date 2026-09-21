#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
  using namespace pluss;
  using namespace pluss::subsp::math::diff;

  const Series x = pluss::series::linspace(-1.0, 1.0, 256, ScalarType::Float64);
  const Series y = x.mul(2.0).sub(0.5);

  std::vector<Series> params{
      pluss::make_series(0.0).to(ScalarType::Float64),
      pluss::make_series(0.0).to(ScalarType::Float64),
  };

  const double learning_rate = 0.08;

  for (std::int64_t step = 1; step <= 300; ++step) {
    auto [loss, sensitivities] = value_and_grad(
        [&](const std::vector<Trace>& p) -> Trace {
          const Trace w = p[0];
          const Trace b = p[1];
          const Trace vx = trace(x, false);
          const Trace vy = trace(y, false);
          const Trace pred = w * vx + b;
          const Trace err = pred - vy;
          return mean(err * err);
        },
        params);

    for (std::size_t i = 0; i < params.size(); ++i) {
      params[i] = params[i].sub(sensitivities[i].mul(learning_rate));
    }

    if (step % 50 == 0 || step == 1) {
      std::cout << "step=" << step << " loss=" << loss.item<double>() << '\n';
    }
  }

  std::cout << "learned w=" << params[0].item<double>() << " b=" << params[1].item<double>() << '\n';

  // Nonlinear autodiff smoke: exercise recent tape rules in one scalar objective.
  std::vector<Series> probe_params{
      pluss::make_series(0.7).to(ScalarType::Float64),
  };
  auto [probe_loss, probe_sens] = value_and_grad(
      [&](const std::vector<Trace>& p) -> Trace {
        const Trace z = p[0];
        const Trace y0 = square(z);
        const Trace y1 = log(exp(z) + 1.0);
        const Trace y2 = sqrt(y0 + 2.0);
        const Trace y3 = tanh(z) + sin(z) - cos(z);
        const Trace y4 = relu(z - 0.2) + clamp(z, -1.0, 1.0);
        const Trace y5 = pow(z, 3.0) + rsqrt(y0 + 1.0);
        const Trace y6 = softplus(z, 1.5, 18.0) + sigmoid(z);
        return y1 + y2 + y3 + y4 + y5 + y6;
      },
      probe_params);
 
  const double probe_loss_v = probe_loss.item<double>();
  const double probe_sens_v = probe_sens[0].item<double>();
  std::cout << "probe loss=" << probe_loss_v << " sensitivity=" << probe_sens_v << '\n';
  if (!std::isfinite(probe_loss_v) || !std::isfinite(probe_sens_v)) {
    std::cerr << "nonlinear autodiff smoke failed: non-finite value/sensitivity\n";
    return 2;
  }

  {
    Series direct = pluss::make_series(1.25).to(ScalarType::Float64);
    auto tracked_direct = trace(direct, true);
    auto direct_loss = sum(square(tracked_direct));
    direct_loss.reverse();
    const double direct_sens = tracked_direct.sensitivity().item<double>();
    std::cout << "reverse sensitivity=" << direct_sens << '\n';
    if (!std::isfinite(direct_sens) || std::abs(direct_sens - 2.5) > 1e-9) {
      std::cerr << "Series::reverse smoke failed: expected sensitivity 2.5\n";
      return 5;
    }
  }

  {
    const Series primal = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3});
    const Series cotangent = pluss::make_series(std::vector<double>{1.0, -0.5, 0.25}, {3});
    auto [vjp_value, pullback] = vjp(
        [](const Trace& x) -> Trace {
          return sin(x) + square(x);
        },
        primal);
    const Series pulled = pullback(cotangent);
    const double pulled_sum = pulled.sum().item<double>();
    std::cout << "vjp value sum=" << vjp_value.sum().item<double>()
              << " pullback sum=" << pulled_sum << '\n';
    if (!std::isfinite(pulled_sum)) {
      std::cerr << "vjp smoke failed: expected finite pullback\n";
      return 6;
    }
  }

  {
    const Series primal = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3});
    const Series tangent = pluss::make_series(std::vector<double>{0.1, 0.05, -0.2}, {3});
    const auto vector_objective = [](const auto& x) {
      return sin(x) + square(x);
    };
    auto [jvp_value, directional] = jvp(
        vector_objective,
        primal,
        tangent);
    const Series expected = primal.cos().add(primal.mul(2.0)).mul(tangent);
    const double jvp_error = directional.sub(expected).abs().sum().item<double>();
    std::cout << "jvp value sum=" << jvp_value.sum().item<double>()
              << " directional error=" << jvp_error << '\n';
    if (!std::isfinite(jvp_error) || jvp_error > 1e-9) {
      std::cerr << "jvp smoke failed: expected exact vector-output directional derivative\n";
      return 7;
    }
  }

  {
    const Series primal = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3});
    const auto vector_objective = [](const auto& x) {
      return sin(x) + square(x);
    };
    auto [lin_value, pushforward] = linearize(vector_objective, primal);
    const Series tangent_a = pluss::make_series(std::vector<double>{0.1, 0.05, -0.2}, {3});
    const Series tangent_b = pluss::make_series(std::vector<double>{-0.3, 0.2, 0.15}, {3});
    const Series linear_a = pushforward(tangent_a);
    const Series linear_b = pushforward(tangent_b);
    const Series expected_a = primal.cos().add(primal.mul(2.0)).mul(tangent_a);
    const Series expected_b = primal.cos().add(primal.mul(2.0)).mul(tangent_b);
    const double err_a = linear_a.sub(expected_a).abs().sum().item<double>();
    const double err_b = linear_b.sub(expected_b).abs().sum().item<double>();
    std::cout << "linearize value sum=" << lin_value.sum().item<double>()
              << " err_a=" << err_a
              << " err_b=" << err_b << '\n';
    if (!std::isfinite(err_a) || !std::isfinite(err_b) || err_a > 1e-9 || err_b > 1e-9) {
      std::cerr << "linearize smoke failed: pushforward mismatch\n";
      return 8;
    }
  }

  {
    const Series primal = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3});
    const auto vector_objective = [](const auto& x) {
      return sin(x) + square(x);
    };
    auto [jacrev_value, jacrev_matrix] = jacrev(vector_objective, primal);
    auto [jacfwd_value, jacfwd_matrix] = jacfwd(vector_objective, primal);
    const auto slope = primal.cos().add(primal.mul(2.0)).to_vec<double>();
    const Series expected = pluss::make_series(
        std::vector<double>{
            slope[0], 0.0,      0.0,
            0.0,      slope[1], 0.0,
            0.0,      0.0,      slope[2]},
        {3, 3});
    const double jacrev_error = jacrev_matrix.sub(expected).abs().sum().item<double>();
    const double jacfwd_error = jacfwd_matrix.sub(expected).abs().sum().item<double>();
    std::cout << "jacobian value sums=" << jacrev_value.sum().item<double>()
              << "," << jacfwd_value.sum().item<double>()
              << " jacrev error=" << jacrev_error
              << " jacfwd error=" << jacfwd_error << '\n';
    if (!std::isfinite(jacrev_error) || !std::isfinite(jacfwd_error) ||
        jacrev_error > 1e-9 || jacfwd_error > 1e-9) {
      std::cerr << "jacobian smoke failed: Jacobian mismatch\n";
      return 11;
    }
  }

  {
    const Series primal = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3});
    const Series tangent = pluss::make_series(std::vector<double>{0.1, 0.05, -0.2}, {3});
    auto [hvp_value, hv] = hvp(
        [](const auto& x) {
          return sum(sin(x) + square(x));
        },
        primal,
        tangent);
    const Series expected = primal.sin().neg().add(2.0).mul(tangent);
    const double hvp_error = hv.sub(expected).abs().sum().item<double>();
    std::cout << "hvp value=" << hvp_value.item<double>()
              << " hvp error=" << hvp_error << '\n';
    if (!std::isfinite(hvp_error) || hvp_error > 1e-5) {
      std::cerr << "hvp smoke failed: Hessian-vector mismatch\n";
      return 9;
    }
  }

  {
    std::vector<Series> bundle{
        pluss::make_series(0.0).to(ScalarType::Float64),
        pluss::make_series(0.0).to(ScalarType::Float64),
    };
    auto [bundle_value, bundle_grads] = value_and_grad(
        [&](const std::vector<Trace>& p) -> Trace {
          const Trace pred = p[0] * trace(x, false) + p[1];
          const Trace err = pred - trace(y, false);
          return mean(err * err);
        },
        bundle);
    std::cout << "bundle loss=" << bundle_value.item<double>()
              << " dw=" << bundle_grads[0].item<double>()
              << " db=" << bundle_grads[1].item<double>() << '\n';
    if (bundle_grads.size() != 2) {
      std::cerr << "bundle autodiff smoke failed: expected two gradients\n";
      return 10;
    }
  }

  Series version_probe = pluss::make_series(1.0).to(ScalarType::Float64);
  const std::int64_t version_before = version_probe.storage_version();
  version_probe.relu_();
  const std::int64_t version_after = version_probe.storage_version();
  std::cout << "version before=" << version_before << " after=" << version_after << '\n';
  if (version_after <= version_before) {
    std::cerr << "version counter smoke failed: expected in-place op to bump version\n";
    return 3;
  }

  bool blocked_tracked_inplace = false;
  try {
    Series tracked = pluss::make_series(0.25).to(ScalarType::Float64);
    (void)trace(tracked, true);
    tracked.add_(1.0);
  } catch (const std::exception&) {
    blocked_tracked_inplace = true;
  }
  std::cout << "tracked inplace blocked=" << (blocked_tracked_inplace ? "true" : "false") << '\n';
  if (!blocked_tracked_inplace) {
    std::cerr << "tracked in-place safety smoke failed: expected mutation rejection\n";
    return 4;
  }
  return 0;
}
