// Demonstrates training a model on MPS using ONLY the raw Series autodiff
// primitives -- no pluss::nn::Module, no pluss::nn::optim::* optimizer, no
// NodeK model zoo at all. This is the path a user reaches for when they want
// their own custom architecture/training loop rather than one of Pluss's
// built-in models (see PlussAI/test/nn_mps_schedule_family_trainability_test.cpp
// for the built-in-model-zoo counterpart of this proof).
//
// The model is a plain 2-layer MLP (matmul + bias + tanh, matmul + bias)
// hand-written from Series ops. Parameters are ordinary tracked Series;
// the update rule is written out by hand (no Adam/SGD class involved).
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <vector>

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

namespace {

Series mlp_forward(const Series& x, const Series& w1, const Series& b1, const Series& w2, const Series& b2) {
  const Series hidden = x.matmul(w1).add(b1).tanh();
  return hidden.matmul(w2).add(b2);
}

} // namespace

int main() {
  if (!pluss::mps::is_ready()) {
    std::cout << "MPS runtime unavailable on this machine -- nothing to demonstrate.\n";
    return 0;
  }

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);

  // Ground truth: y = sin(2x0) + 0.5*x1^2, a nonlinear function an affine
  // model cannot fit, so a decreasing loss genuinely demonstrates the
  // hidden tanh layer's gradient is flowing correctly through MPS.
  constexpr std::int64_t n = 64;
  std::vector<float> xbuf(static_cast<std::size_t>(n * 2));
  std::vector<float> ybuf(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const float x0 = -1.0f + 2.0f * static_cast<float>(i) / static_cast<float>(n - 1);
    const float x1 = std::sin(0.7f * static_cast<float>(i));
    xbuf[static_cast<std::size_t>(i * 2 + 0)] = x0;
    xbuf[static_cast<std::size_t>(i * 2 + 1)] = x1;
    ybuf[static_cast<std::size_t>(i)] = std::sin(2.0f * x0) + 0.5f * x1 * x1;
  }
  const Series x = pluss::make_series(xbuf, {n, 2}, Config{}.with_dtype(ScalarType::Float32)).to(pluss::pMPS);
  const Series y = pluss::make_series(ybuf, {n, 1}, Config{}.with_dtype(ScalarType::Float32)).to(pluss::pMPS);

  constexpr std::int64_t hidden_size = 16;
  Series w1 = pluss::series::randn({2, hidden_size}, cfg, /*seed=*/1001ULL).mul(0.5f);
  Series b1 = pluss::series::zeros({hidden_size}, cfg);
  Series w2 = pluss::series::randn({hidden_size, 1}, cfg, /*seed=*/1002ULL).mul(0.5f);
  Series b2 = pluss::series::zeros({1}, cfg);
  for (Series* p : {&w1, &b1, &w2, &b2}) {
    p->tracks_slope_(true);
  }

  const float learning_rate = 0.1f;
  double first_loss = 0.0;
  double last_loss = 0.0;

  for (int epoch = 1; epoch <= 200; ++epoch) {
    const Series pred = mlp_forward(x, w1, b1, w2, b2);
    const Series err = pred.sub(y);
    const Series loss = err.mul(err).mean();

    loss.reverse();

    for (Series* p : {&w1, &b1, &w2, &b2}) {
      // Hand-written SGD step: no optimizer class involved. Mirrors the
      // engine's own assign_parameter_() pattern (NodeK/NN/optim/optim_base.h)
      // for reassigning a tracked parameter in place: detach the new value
      // from the update expression's tape, then re-tag it as a fresh leaf.
      Series updated = p->sub(p->sensitivity().mul(learning_rate));
      *p = updated;
      pluss::codem::series_tape::clear_source_link(*p);
      p->tracks_slope_(true);
    }

    const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
    if (epoch == 1) first_loss = loss_value;
    last_loss = loss_value;
    if (epoch == 1 || epoch % 20 == 0) {
      std::cout << "epoch=" << epoch << " loss=" << loss_value << " (platform=MPS)\n";
    }
  }

  std::cout << "first_loss=" << first_loss << " last_loss=" << last_loss << "\n";
  if (!(last_loss < first_loss * 0.1)) {
    std::cerr << "custom MPS autodiff training demo failed to converge\n";
    return 1;
  }
  std::cout << "custom autodiff training on MPS: OK (loss dropped "
            << (first_loss / last_loss) << "x, no pluss::nn:: model or optimizer used)\n";
  return 0;
}
