// Float64 training support: pluss::detail::nn::is_training_float_dtype_
// (include/pluss/nn_detail.h) used to allow only {Float32, Float16,
// BFloat16} -- a leftover from when Series itself only reliably supported
// float32 math. Series has since been verified to Float64 parity across the
// whole engine (see the Series-autodiff-torch-parity program), so the
// optimizers only needed their dtype gate widened; the update math in every
// concrete optimizer (SGD/Adam/AdamW/RMSprop/Nadam/Adagrad) is already
// written in plain, dtype-generic Series arithmetic with hyperparameters
// stored as C++ `double`, not float32-specific in any way.
//
// Rather than re-deriving each optimizer's update formula by hand (real
// risk of a subtle mismatch with an implementation detail -- bias-correction
// order, epsilon placement, ...), this file uses the ALREADY-TRUSTED Float32
// path as the oracle: run one step of each optimizer family at Float32 and
// at Float64 from identical inputs, and require the two results agree
// (within a tolerance sized for float32's ~7-digit precision). If Float64
// support silently used a different code path with a bug, this would catch
// it without needing a hand-derived expected value per family.
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

double scalar_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

Config cfg_for(ScalarType dtype, pluss::Platform platform) {
  return Config{}.with_dtype(dtype).with_platform(platform);
}

// Runs `step_once` (build the optimizer over a fresh 1-element parameter at
// the given dtype, take exactly one step with a fixed gradient, return the
// updated parameter's value) at both Float32 and Float64, and checks they
// agree. Also checks the Float64 result actually stayed Float64-typed (not
// silently downcast somewhere in the lane-dispatch machinery).
bool cross_precision_check(
    const char* name,
    const std::function<Series(ScalarType)>& make_param,
    const std::function<void(Series&, ScalarType)>& take_one_step,
    std::ostream* err) {
  Series p32 = make_param(ScalarType::Float32);
  Series p64 = make_param(ScalarType::Float64);
  take_one_step(p32, ScalarType::Float32);
  take_one_step(p64, ScalarType::Float64);

  if (p64.scalar_type() != ScalarType::Float64) {
    if (err) *err << name << ": Float64 step did not preserve the parameter's dtype\n";
    return false;
  }
  const double v32 = scalar_f64(p32);
  const double v64 = scalar_f64(p64);
  if (std::abs(v32 - v64) > 1e-4) {
    if (err) *err << name << ": Float32 (" << v32 << ") vs Float64 (" << v64
                  << ") step mismatch\n";
    return false;
  }
  return true;
}

bool run_optim_float64_checks(pluss::Platform platform, std::ostream* err = nullptr) {
  const auto make_scalar = [&](double v, ScalarType dtype) {
    return pluss::make_series(v, cfg_for(dtype, platform));
  };

  // SGD
  if (!cross_precision_check(
          "SGD",
          [&](ScalarType dt) { return make_scalar(1.0, dt); },
          [&](Series& p, ScalarType dt) {
            pluss::nn::optim::SGDOptions opt{};
            opt.learning_rate = 0.1;
            pluss::nn::optim::SGD sgd(std::vector<Series*>{&p}, opt);
            sgd.step(std::vector<Series>{make_scalar(0.5, dt)});
          },
          err)) {
    return false;
  }

  // Adam
  if (!cross_precision_check(
          "Adam",
          [&](ScalarType dt) { return make_scalar(1.0, dt); },
          [&](Series& p, ScalarType dt) {
            pluss::nn::optim::AdamOptions opt{};
            opt.learning_rate = 0.1;
            pluss::nn::optim::Adam adam(std::vector<Series*>{&p}, opt);
            adam.step(std::vector<Series>{make_scalar(0.5, dt)});
          },
          err)) {
    return false;
  }

  // AdamW (default weight_decay != 0, so this also exercises the decoupled
  // decay path, not just the Adam-equivalent case)
  if (!cross_precision_check(
          "AdamW",
          [&](ScalarType dt) { return make_scalar(1.0, dt); },
          [&](Series& p, ScalarType dt) {
            pluss::nn::optim::AdamWOptions opt{};
            opt.learning_rate = 0.1;
            pluss::nn::optim::AdamW adamw(std::vector<Series*>{&p}, opt);
            adamw.step(std::vector<Series>{make_scalar(0.5, dt)});
          },
          err)) {
    return false;
  }

  // RMSprop
  if (!cross_precision_check(
          "RMSprop",
          [&](ScalarType dt) { return make_scalar(1.0, dt); },
          [&](Series& p, ScalarType dt) {
            pluss::nn::optim::RMSpropOptions opt{};
            opt.learning_rate = 0.1;
            pluss::nn::optim::RMSprop rms(std::vector<Series*>{&p}, opt);
            rms.step(std::vector<Series>{make_scalar(0.5, dt)});
          },
          err)) {
    return false;
  }

  // NAdam
  if (!cross_precision_check(
          "NAdam",
          [&](ScalarType dt) { return make_scalar(1.0, dt); },
          [&](Series& p, ScalarType dt) {
            pluss::nn::optim::NAdamOptions opt{};
            opt.learning_rate = 0.1;
            pluss::nn::optim::NAdam nadam(std::vector<Series*>{&p}, opt);
            nadam.step(std::vector<Series>{make_scalar(0.5, dt)});
          },
          err)) {
    return false;
  }

  // Adagrad
  if (!cross_precision_check(
          "Adagrad",
          [&](ScalarType dt) { return make_scalar(1.0, dt); },
          [&](Series& p, ScalarType dt) {
            pluss::nn::optim::AdagradOptions opt{};
            opt.learning_rate = 0.1;
            pluss::nn::optim::Adagrad adagrad(std::vector<Series*>{&p}, opt);
            adagrad.step(std::vector<Series>{make_scalar(0.5, dt)});
          },
          err)) {
    return false;
  }

  // Multi-step: catches state (moment buffers etc.) silently drifting to a
  // different dtype across iterations, not just the first step.
  {
    Series p32 = make_scalar(1.0, ScalarType::Float32);
    Series p64 = make_scalar(1.0, ScalarType::Float64);
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 0.05;
    pluss::nn::optim::Adam adam32(std::vector<Series*>{&p32}, opt);
    pluss::nn::optim::Adam adam64(std::vector<Series*>{&p64}, opt);
    for (int i = 0; i < 5; ++i) {
      const double g = 0.3 + 0.1 * static_cast<double>(i);
      adam32.step(std::vector<Series>{make_scalar(g, ScalarType::Float32)});
      adam64.step(std::vector<Series>{make_scalar(g, ScalarType::Float64)});
    }
    if (p64.scalar_type() != ScalarType::Float64) {
      if (err) *err << "Adam multi-step: parameter dtype drifted away from Float64\n";
      return false;
    }
    if (std::abs(scalar_f64(p32) - scalar_f64(p64)) > 1e-4) {
      if (err) *err << "Adam multi-step: Float32 (" << scalar_f64(p32) << ") vs Float64 ("
                    << scalar_f64(p64) << ") diverged\n";
      return false;
    }
  }

  return true;
}

}  // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn optimizers train Float64 parameters matching the Float32 path (CPU)", "[nn][optim][float64]") {
  REQUIRE(run_optim_float64_checks(pluss::pCPU, &std::cerr));
}

// Checked live rather than assumed safe by symmetry with the CPU result
// above: turns out CodeM's own `Series::platform(Platform)` (OpsInternal.cpp)
// already carries a pre-existing, deliberate guard --
// `PLUSS_CHECK(scalar_type() != ScalarType::Float64, "... float64 is
// unsupported on MPS ...")` -- rejecting any Float64 Series moved onto MPS,
// unrelated to and unaffected by this optimizer-layer dtype-gate change.
// Apple GPUs genuinely have limited/no native double-precision hardware
// support, so this is correct, existing, defensive engineering: a loud,
// clear, early failure instead of silent wrong-precision computation. The
// optimizer's own fused/batched-lane dtype checks (supports_fused_lane_
// platform_/supports_batched_lane_platform_ in optim_base.h) never get a
// chance to matter in practice, since a Float64 parameter simply can't be
// materialized on MPS in the first place. This pins that guard down as a
// contract, not a bug to chase.
TEST_CASE("Float64 parameters on MPS are rejected loudly, not silently miscomputed", "[nn][optim][float64][mps]") {
  if (!pluss::mps::is_ready()) {
    WARN("MPS not ready on this machine; skipping MPS Float64 guard check");
    return;
  }
  bool threw_expected = false;
  try {
    (void)run_optim_float64_checks(pluss::pMPS, &std::cerr);
  } catch (const std::exception& e) {
    const std::string what = e.what();
    threw_expected = what.find("float64 is unsupported on MPS") != std::string::npos;
    if (!threw_expected) {
      UNSCOPED_INFO("unexpected exception: " << what);
    }
  }
  REQUIRE(threw_expected);
}
#else
int main() {
  bool ok = run_optim_float64_checks(pluss::pCPU, &std::cerr);
  if (pluss::mps::is_ready()) {
    // See the Catch2 TEST_CASE above: Float64-on-MPS is expected to throw
    // (a pre-existing CodeM guard, not an optimizer-layer bug).
    bool threw_expected = false;
    try {
      (void)run_optim_float64_checks(pluss::pMPS, &std::cerr);
    } catch (const std::exception& e) {
      threw_expected = std::string(e.what()).find("float64 is unsupported on MPS") != std::string::npos;
    }
    ok = threw_expected && ok;
  }
  return ok ? 0 : 1;
}
#endif
