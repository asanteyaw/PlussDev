#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <cmath>
#include <limits>

TEST_CASE("torch-style inplace aliases/shims are accepted on Series", "[invoke][inplace][torch-parity]") {
  auto invoke_inplace = [](std::string_view op, pluss::Series& t, std::initializer_list<pluss::codem::_compat::Arg> xs) -> void {
    std::vector<pluss::codem::_compat::Arg> args(xs.begin(), xs.end());
    pluss::codem::_compat::invoke_(op, t, pluss::msharp::RangeView::Of<pluss::codem::_compat::Arg>(args));
  };

  pluss::Series x = pluss::series::linspace(-2.0, 2.0, 9, pluss::config({{"dtype","float32"}}));
  const std::vector<std::int64_t> shape(x.sizes().begin(), x.sizes().end());

  invoke_inplace("clip_", x, {-1.0, 1.0});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("relu6_", x, {});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("hardtanh_", x, {-0.25, 0.25});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("hardsigmoid_", x, {});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("hardswish_", x, {});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("mish_", x, {});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("selu_", x, {});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("dropout_", x, {0.2, true});
  REQUIRE(x.sizes() == shape);

  invoke_inplace("rrelu_", x, {0.1, 0.3, true});
  REQUIRE(x.sizes() == shape);

  pluss::Series noise = pluss::series::full({9}, 0.2, pluss::config({{"dtype","float32"}}));
  invoke_inplace("rrelu_with_noise_", x, {noise, 0.1, 0.3, true});
  REQUIRE(x.sizes() == shape);

  pluss::Series y = pluss::series::zeros({5}, pluss::config({{"dtype","float32"}}));
  pluss::Series idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
  pluss::Series src = pluss::make_series(std::vector<double>{7.0, 9.0});
  invoke_inplace("_index_put_impl_", y, {idx, src, false, false});
  REQUIRE(y.size(0) == 5);

  pluss::Series t = pluss::series::ones({3}, pluss::config({{"dtype","float32"}}));
  invoke_inplace("requires_grad_", t, {true});
  const auto tracks_i64 = pluss::codem::_compat::invoke("tracks_slope", t)
                              .to(pluss::ScalarType::Int64)
                              .item<std::int64_t>();
  REQUIRE(tracks_i64 == 1);
}

TEST_CASE("torch-style amp/fused inplace ops mutate real state", "[invoke][inplace][torch-parity][fused]") {
  auto invoke_inplace = [](std::string_view op, pluss::Series& t, std::initializer_list<pluss::codem::_compat::Arg> xs) -> void {
    std::vector<pluss::codem::_compat::Arg> args(xs.begin(), xs.end());
    pluss::codem::_compat::invoke_(op, t, pluss::msharp::RangeView::Of<pluss::codem::_compat::Arg>(args));
  };

  {
    pluss::Series g = pluss::make_series(std::vector<double>{1.0, -2.0, std::numeric_limits<double>::infinity()})
                          .to(pluss::ScalarType::Float32);
    pluss::Series found_inf = pluss::series::zeros({}, pluss::config({{"dtype", "float32"}}));
    pluss::Series inv_scale = pluss::series::full({}, 0.5, pluss::config({{"dtype", "float32"}}));
    invoke_inplace("_amp_foreach_non_finite_check_and_unscale_", g, {found_inf, inv_scale});
    const auto found = found_inf.to(pluss::ScalarType::Float64).item<double>();
    REQUIRE(found == 1.0);
    const auto gv = g.to(pluss::ScalarType::Float64).host<double>();
    REQUIRE(std::abs(gv[0] - 0.5) < 1e-12);
    REQUIRE(std::abs(gv[1] + 1.0) < 1e-12);
    REQUIRE(!std::isfinite(gv[2]));
  }

  {
    pluss::Series scale = pluss::series::full({}, 16.0, pluss::config({{"dtype", "float32"}}));
    pluss::Series tracker = pluss::series::zeros({}, pluss::config({{"dtype", "int32"}}));
    pluss::Series found0 = pluss::series::zeros({}, pluss::config({{"dtype", "float32"}}));
    invoke_inplace("_amp_update_scale_", scale, {tracker, found0, 2.0, 0.5, 2});
    REQUIRE(tracker.to(pluss::ScalarType::Int64).item<std::int64_t>() == 1);
    invoke_inplace("_amp_update_scale_", scale, {tracker, found0, 2.0, 0.5, 2});
    REQUIRE(std::abs(scale.to(pluss::ScalarType::Float64).item<double>() - 32.0) < 1e-12);
    REQUIRE(tracker.to(pluss::ScalarType::Int64).item<std::int64_t>() == 0);
    pluss::Series found1 = pluss::series::full({}, 1.0, pluss::config({{"dtype", "float32"}}));
    invoke_inplace("_amp_update_scale_", scale, {tracker, found1, 2.0, 0.5, 2});
    REQUIRE(std::abs(scale.to(pluss::ScalarType::Float64).item<double>() - 16.0) < 1e-12);
    REQUIRE(tracker.to(pluss::ScalarType::Int64).item<std::int64_t>() == 0);
  }

  {
    pluss::Series p = pluss::make_series(std::vector<double>{1.0, 2.0}).to(pluss::ScalarType::Float32);
    pluss::Series g = pluss::make_series(std::vector<double>{0.1, 0.2}).to(pluss::ScalarType::Float32);
    pluss::Series m = pluss::series::zeros({2}, pluss::config({{"dtype", "float32"}}));
    invoke_inplace("_fused_sgd_", p, {g, m, 0.0, 0.9, 0.1, 0.0, false, false, true});
    auto pv = p.to(pluss::ScalarType::Float64).host<double>();
    auto mv = m.to(pluss::ScalarType::Float64).host<double>();
    REQUIRE(std::abs(pv[0] - 0.99) < 1e-6);
    REQUIRE(std::abs(pv[1] - 1.98) < 1e-6);
    REQUIRE(std::abs(mv[0] - 0.1) < 1e-6);
    REQUIRE(std::abs(mv[1] - 0.2) < 1e-6);
    invoke_inplace("_fused_sgd_", p, {g, m, 0.0, 0.9, 0.1, 0.0, false, false, false});
    pv = p.to(pluss::ScalarType::Float64).host<double>();
    REQUIRE(std::abs(pv[0] - 0.971) < 1e-6);
    REQUIRE(std::abs(pv[1] - 1.942) < 1e-6);
  }

  {
    pluss::Series p = pluss::make_series(std::vector<double>{1.0, 2.0}).to(pluss::ScalarType::Float32);
    pluss::Series g = pluss::make_series(std::vector<double>{1.0, 2.0}).to(pluss::ScalarType::Float32);
    pluss::Series state_sum = pluss::series::zeros({2}, pluss::config({{"dtype", "float32"}}));
    pluss::Series step = pluss::series::full({}, 1.0, pluss::config({{"dtype", "float32"}}));
    invoke_inplace("_fused_adagrad_", p, {g, state_sum, step, 0.1, 0.0, 0.0, 1e-10, false});
    const auto pv = p.to(pluss::ScalarType::Float64).host<double>();
    REQUIRE(std::abs(pv[0] - 0.9) < 1e-6);
    REQUIRE(std::abs(pv[1] - 1.9) < 1e-6);
  }

  {
    pluss::Series p = pluss::make_series(std::vector<double>{1.0, 2.0}).to(pluss::ScalarType::Float32);
    pluss::Series g = pluss::make_series(std::vector<double>{0.1, 0.2}).to(pluss::ScalarType::Float32);
    pluss::Series exp_avg = pluss::series::zeros({2}, pluss::config({{"dtype", "float32"}}));
    pluss::Series exp_avg_sq = pluss::series::zeros({2}, pluss::config({{"dtype", "float32"}}));
    pluss::Series max_exp_avg_sq = pluss::series::zeros({2}, pluss::config({{"dtype", "float32"}}));
    pluss::Series step = pluss::series::full({}, 1.0, pluss::config({{"dtype", "float32"}}));
    invoke_inplace("_fused_adam_", p, {g, exp_avg, exp_avg_sq, max_exp_avg_sq, step, 0.1, 0.9, 0.999, 0.0, 1e-8, false, false});
    const auto pv = p.to(pluss::ScalarType::Float64).host<double>();
    REQUIRE(std::abs(pv[0] - 0.9) < 2e-5);
    REQUIRE(std::abs(pv[1] - 1.9) < 2e-5);
    invoke_inplace("_fused_adamw_", p, {g, exp_avg, exp_avg_sq, max_exp_avg_sq, step, 0.1, 0.9, 0.999, 0.01, 1e-8, false, false});
    REQUIRE(p.sizes().size() == 1);
    REQUIRE(p.size(0) == 2);
  }
}
