#include <pluss/ai/ice.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f32_cpu() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

bool run_nn_grad_mode_checks(std::ostream* err = nullptr) {
  Series x = pluss::make_series(std::vector<float>{1.0F, 2.0F, 3.0F}, {3}, f32_cpu());
  x.tracks_slope_(true);

  if (!pluss::nn::GradMode::is_enabled()) {
    if (err) *err << "Grad mode should start enabled on the test thread\n";
    return false;
  }

  {
    const pluss::nn::NoGradGuard guard;
    if (pluss::nn::GradMode::is_enabled()) {
      if (err) *err << "NoGradGuard should disable grad mode inside the scope\n";
      return false;
    }
    const Series y = x.mul(x);
    if (y.tracks_slope()) {
      if (err) *err << "Series created under NoGradGuard should not capture a reverse graph\n";
      return false;
    }
  }

  if (!pluss::nn::GradMode::is_enabled()) {
    if (err) *err << "NoGradGuard should restore the previous grad mode on exit\n";
    return false;
  }

  {
    const pluss::nn::AutoGradMode disable(false);
    if (pluss::nn::GradMode::is_enabled()) {
      if (err) *err << "AutoGradMode(false) should disable grad mode\n";
      return false;
    }
    {
      const pluss::nn::AutoGradMode enable(true);
      if (!pluss::nn::GradMode::is_enabled()) {
        if (err) *err << "Nested AutoGradMode(true) should temporarily re-enable grad mode\n";
        return false;
      }
      const Series y = x.add(1.0F);
      if (!y.tracks_slope()) {
        if (err) *err << "Series created with grad mode re-enabled should track slope again\n";
        return false;
      }
    }
    if (pluss::nn::GradMode::is_enabled()) {
      if (err) *err << "Nested AutoGradMode should restore the outer disabled state\n";
      return false;
    }
  }

  if (!pluss::nn::GradMode::is_enabled()) {
    if (err) *err << "Grad mode should be restored after nested AutoGradMode scopes\n";
    return false;
  }

  {
    const std::vector<Series*> params{&x};
    const pluss::ai::ice::NoGradGuard guard(params);
    if (pluss::nn::GradMode::is_enabled()) {
      if (err) *err << "ice::NoGradGuard should delegate to the shared lower-level no-grad state\n";
      return false;
    }
    if (!x.tracks_slope()) {
      if (err) *err << "ice::NoGradGuard should not mutate parameter tracked-state directly anymore\n";
      return false;
    }
    const Series y = x.sub(2.0F);
    if (y.tracks_slope()) {
      if (err) *err << "ice::NoGradGuard should suppress graph capture through the shared grad mode\n";
      return false;
    }
  }

  if (!pluss::nn::GradMode::is_enabled()) {
    if (err) *err << "ice::NoGradGuard should restore shared grad mode on exit\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("pluss nn grad mode guards are scoped and ice delegates to them", "[ai][nn][autodiff]") {
  REQUIRE(run_nn_grad_mode_checks());
}
#else
int main() {
  if (!run_nn_grad_mode_checks(&std::cerr)) return 1;
  std::cout << "nn_grad_mode_test passed\n";
  return 0;
}
#endif
