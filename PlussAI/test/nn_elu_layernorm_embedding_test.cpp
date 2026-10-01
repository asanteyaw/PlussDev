// Correctness coverage for pluss::ai::nn::Elu/LayerNorm/Embedding --
// promoted out of ice::models::TemporalFusionTransformer's originally
// private building blocks once a second consumer needed them (see
// ICE_BLUEPRINT.md's centralization note). Checks the actual math, not
// just "constructs and runs".
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config float_cfg() { return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

std::vector<double> to_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

bool run_elu_checks(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  const Series x = pluss::make_series(std::vector<float>{-2.0F, -0.5F, 0.0F, 1.0F, 3.0F}, {5}, cfg);
  pluss::nn::Elu elu{};
  const Series y = elu->forward(x);
  const auto values = to_vec(y);

  const std::vector<double> expected = {
      std::exp(-2.0) - 1.0, // x <= 0: exp(x) - 1
      std::exp(-0.5) - 1.0,
      0.0, // exp(0) - 1 == 0, matches x > 0 branch at the boundary too
      1.0, // x > 0: x
      3.0,
  };
  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (std::abs(values[i] - expected[i]) > 1e-4) {
      if (err) *err << "Elu value mismatch at " << i << ": expected " << expected[i] << " got " << values[i] << "\n";
      return false;
    }
  }

  // Gradient check: d/dx elu(x) = 1 for x>0, exp(x) for x<=0.
  Series xg = x.clone();
  xg.tracks_slope_(true);
  const Series yg = elu->forward(xg);
  yg.sum().reverse();
  const auto grad = to_vec(xg.sensitivity());
  const std::vector<double> expected_grad = {std::exp(-2.0), std::exp(-0.5), 1.0, 1.0, 1.0};
  for (std::size_t i = 0; i < expected_grad.size(); ++i) {
    if (std::abs(grad[i] - expected_grad[i]) > 1e-4) {
      if (err) *err << "Elu gradient mismatch at " << i << ": expected " << expected_grad[i] << " got " << grad[i] << "\n";
      return false;
    }
  }
  return true;
}

bool run_layernorm_checks(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  pluss::nn::LayerNorm norm(pluss::nn::LayerNormOptions{.normalized_size = 4, .eps = 1e-5, .cfg = cfg});

  // Freshly constructed: weight=ones, bias=zeros, so forward() output must
  // itself be exactly zero-mean/unit-variance per row (nothing to undo yet).
  const Series x = pluss::make_series(
      std::vector<float>{1.0F, 2.0F, 3.0F, 4.0F, -1.0F, 0.0F, 5.0F, 10.0F}, {2, 4}, cfg);
  const Series y = norm->forward(x);
  const auto row_mean = to_vec(y.mean(-1));
  const auto row_var = to_vec(y.var(-1, false));
  for (std::size_t row = 0; row < row_mean.size(); ++row) {
    if (std::abs(row_mean[row]) > 1e-3) {
      if (err) *err << "LayerNorm row " << row << " mean should be ~0, got " << row_mean[row] << "\n";
      return false;
    }
    if (std::abs(row_var[row] - 1.0) > 1e-2) {
      if (err) *err << "LayerNorm row " << row << " variance should be ~1, got " << row_var[row] << "\n";
      return false;
    }
  }

  // A non-trivial weight/bias must be applied on top of the normalized value.
  norm->weight().copy_(pluss::make_series(std::vector<float>{2.0F, 2.0F, 2.0F, 2.0F}, {4}, cfg));
  norm->bias().copy_(pluss::make_series(std::vector<float>{1.0F, 1.0F, 1.0F, 1.0F}, {4}, cfg));
  const Series y2 = norm->forward(x);
  const auto row_mean2 = to_vec(y2.mean(-1));
  for (const double m : row_mean2) {
    // mean of (2*normalized + 1) == 2*0 + 1 == 1.
    if (std::abs(m - 1.0) > 1e-2) {
      if (err) *err << "LayerNorm scaled/shifted row mean should be ~1, got " << m << "\n";
      return false;
    }
  }

  // Gradient must reach weight, bias, and the input.
  Series xg = x.clone();
  xg.tracks_slope_(true);
  const Series yg = norm->forward(xg);
  yg.sum().reverse();
  if (!xg.sensitivity().defined() || !norm->weight().sensitivity().defined() || !norm->bias().sensitivity().defined()) {
    if (err) *err << "LayerNorm gradients should reach input, weight, and bias\n";
    return false;
  }

  return true;
}

// Regression coverage for an engine-level copy_() backward bug this test
// tranche caught: Module::parameter_record() marks a freshly Series::empty()
// leaf tracks_slope_(true) *before* reset_parameters() fills it in via
// copy_(untracked_init_values) -- exactly the pattern EmbeddingImpl (and
// Conv/MultiheadAttention/Transformer*) use. copy_()'s backward used to
// unconditionally zero out the region it overwrote and try to redirect that
// gradient to the copy_ source or to prior graph history: correct when
// either exists, but when neither does (a one-time untracked initialization
// of a brand new leaf) it just discarded the real gradient accumulated
// later from normal downstream use, leaving every such parameter's
// gradient silently zero after its very first reverse(). Fixed in
// PlussQit/src/CodeM/compat/Invoke.cpp's copy_dispatch_inplace_.
bool run_copy_init_then_use_gradient_check(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  Series w = Series::empty({5, 3}, cfg.dtype, cfg.platform);
  w.tracks_slope_(true); // mimic parameter_record(): mark leaf before filling values
  w.copy_(pluss::make_series( // mimic reset_parameters(): untracked one-time init
      std::vector<float>{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5}, {5, 3}, cfg));
  w.clear();

  const Series idx = pluss::make_series(std::vector<std::int64_t>{0, 2, 4}, {3},
                                        Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const Series out = w.index_select(0, idx);
  out.sum().reverse();

  if (!w.sensitivity().defined()) {
    if (err) *err << "copy_-then-use check: w.sensitivity() undefined\n";
    return false;
  }
  const auto grad = to_vec(w.sensitivity());
  const std::vector<double> expected = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (std::abs(grad[i] - expected[i]) > 1e-9) {
      if (err) *err << "copy_-then-use check: gradient mismatch at " << i
                    << ": expected " << expected[i] << " got " << grad[i] << "\n";
      return false;
    }
  }
  return true;
}

bool run_embedding_checks(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  constexpr std::int64_t kCardinality = 5;
  constexpr std::int64_t kHiddenSize = 3;
  pluss::nn::Embedding embedding(
      pluss::nn::EmbeddingOptions{.cardinality = kCardinality, .hidden_size = kHiddenSize, .cfg = cfg, .seed = 7ULL});

  // Looking up index i must return exactly weight row i.
  const Series index = pluss::make_series(std::vector<std::int64_t>{0, 2, 4}, {3},
                                          Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const Series looked_up = embedding->forward(index);
  if (looked_up.sizes().size() != 2 || looked_up.sizes()[0] != 3 || looked_up.sizes()[1] != kHiddenSize) {
    if (err) *err << "Embedding forward() should return [numel(index), hidden_size]\n";
    return false;
  }
  const auto looked_up_flat = to_vec(looked_up);
  const auto weight_flat = to_vec(embedding->weight());
  for (std::int64_t row = 0; row < 3; ++row) {
    const std::int64_t src_row = (row == 0) ? 0 : (row == 1 ? 2 : 4);
    for (std::int64_t col = 0; col < kHiddenSize; ++col) {
      const double expected = weight_flat[static_cast<std::size_t>(src_row * kHiddenSize + col)];
      const double actual = looked_up_flat[static_cast<std::size_t>(row * kHiddenSize + col)];
      if (std::abs(expected - actual) > 1e-9) {
        if (err) *err << "Embedding lookup row " << row << " should exactly match weight row " << src_row << "\n";
        return false;
      }
    }
  }

  // Gradient must be sparse: only looked-up rows (0, 2, 4) get a nonzero
  // gradient; untouched rows (1, 3) must stay exactly zero.
  embedding->weight().tracks_slope_(true);
  embedding->weight().clear();
  const Series out = embedding->forward(index);
  out.sum().reverse();
  const auto grad = to_vec(embedding->weight().sensitivity());
  for (std::int64_t row = 0; row < kCardinality; ++row) {
    const bool touched = (row == 0 || row == 2 || row == 4);
    double row_grad_sum = 0.0;
    for (std::int64_t col = 0; col < kHiddenSize; ++col) {
      row_grad_sum += std::abs(grad[static_cast<std::size_t>(row * kHiddenSize + col)]);
    }
    if (touched && row_grad_sum < 1e-6) {
      if (err) *err << "Embedding row " << row << " was looked up but got a zero gradient\n";
      return false;
    }
    if (!touched && row_grad_sum > 1e-9) {
      if (err) *err << "Embedding row " << row << " was never looked up but got a nonzero gradient\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn elu layernorm embedding tranche", "[nn][elu][layernorm][embedding]") {
  if (!run_elu_checks(&std::cerr)) FAIL("Elu checks failed");
  if (!run_layernorm_checks(&std::cerr)) FAIL("LayerNorm checks failed");
  if (!run_copy_init_then_use_gradient_check(&std::cerr)) FAIL("copy_-then-use gradient check failed");
  if (!run_embedding_checks(&std::cerr)) FAIL("Embedding checks failed");
}
#else
int main() {
  bool ok = true;
  ok = run_elu_checks(&std::cerr) && ok;
  ok = run_layernorm_checks(&std::cerr) && ok;
  ok = run_copy_init_then_use_gradient_check(&std::cerr) && ok;
  ok = run_embedding_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "nn elu/layernorm/embedding checks passed\n";
  return 0;
}
#endif
