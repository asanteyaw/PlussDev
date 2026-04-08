#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <array>
#include <span>
#include <vector>

struct DemoCase {
  std::string name;
  std::function<pluss::Series()> run;
};

int main(int argc, char** argv) {
  using pluss::Series;
  using pluss::Platform;
  using pluss::ScalarType;

  const bool want_mps = (argc > 1 && std::string(argv[1]) == "mps");
  const bool mps_ready = pluss::mps::is_ready();
  if (want_mps && !mps_ready) {
    std::cerr << "Requested MPS, but MPS is not ready on this runtime. Aborting (no CPU fallback).\n";
    return 2;
  }
  const Platform run_platform = want_mps ? pluss::pMPS : pluss::pCPU;
  const ScalarType run_float_dt = (run_platform == pluss::pMPS) ? ScalarType::Float32 : ScalarType::Float64;

  auto make_f = [&](std::vector<double> vals, std::vector<std::int64_t> shape) {
    return pluss::make_series(std::move(vals), std::move(shape)).to(run_platform, run_float_dt);
  };

  let x = make_f({-3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, {7});
  let y = make_f({3.0, 2.0, 1.0, 0.0, -1.0, -2.0, -3.0}, {7});
  let pos = make_f({0.2, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0}, {7});
  let unit = make_f({-0.9, -0.5, -0.1, 0.0, 0.1, 0.5, 0.9}, {7});
  
  let m = make_f({1, 2, 3, 4, 5, 6}, {2, 3});
  let n = make_f({6, 5, 4, 3, 2, 1}, {2, 3});
  let mm = make_f({1,2,3,4,5,6,7,8,9}, {3,3});

  let i = pluss::make_series(std::vector<std::int64_t>{1,2,3,4,5,6,7}, {7}).to(run_platform);
  let bi = pluss::make_series(std::vector<std::int64_t>{1,2,3,4,5,6}, {2,3}).to(run_platform);

  let gather_src = pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}, {2, 3}).to(run_platform);
  let gather_idx = pluss::make_series(std::vector<std::int64_t>{2, 1, 0, 0, 1, 2}, {2, 3}).to(run_platform);
  let scatter_base = pluss::Series::zeros({2, 3}, ScalarType::Int64).to(run_platform);
  let scatter_val = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}, {2, 3}).to(run_platform);
  let flat_idx = pluss::make_series(std::vector<std::int64_t>{0, 3, 5}, {3}).to(run_platform);
  let flat_src = pluss::make_series(std::vector<std::int64_t>{99, 88, 77}, {3}).to(run_platform);

  let cond = pluss::Series::ones({7}, ScalarType::Bool).to(run_platform);
  let mask = pluss::Series::ones({2, 3}, ScalarType::Bool).to(run_platform);
  const std::array<std::int64_t, 2> shape_3x2{3, 2};
  const std::array<std::int64_t, 2> perm_10{1, 0};

  std::vector<DemoCase> cases;
  cases.reserve(160);

  auto add_case = [&](const std::string& name, std::function<Series()> fn) {
    cases.push_back(DemoCase{name, std::move(fn)});
  };

  // Shape / indexing / layout
  add_case("view", [&]{ return m.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size())); });
  add_case("reshape", [&]{ return m.reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size())); });
  add_case("transpose", [&]{ return m.transpose(0, 1); });
  add_case("permute", [&]{ return mm.permute(std::span<const std::int64_t>(perm_10.data(), perm_10.size())); });
  add_case("unsqueeze", [&]{ return x.unsqueeze(0); });
  add_case("squeeze", [&]{ return x.unsqueeze(0).squeeze(); });
  add_case("squeeze(dim)", [&]{ return x.unsqueeze(0).squeeze(0); });
  add_case("flatten", [&]{ return mm.flatten(0, 1); });
  add_case("select", [&]{ return mm.select(0, 1); });
  add_case("block", [&]{ return mm.block(1, 2, 0, 2); });
  add_case("take", [&]{ return gather_src.take(flat_idx); });
  add_case("nonzero", [&]{ return cond.nonzero(); });

  // Arithmetic / compare
  add_case("add(scalar)", [&]{ return x.add(1.5); });
  add_case("sub(scalar)", [&]{ return x.sub(1.5); });
  add_case("mul(scalar)", [&]{ return x.mul(2.0); });
  add_case("div(scalar)", [&]{ return x.div(2.0); });
  add_case("pow(scalar)", [&]{ return pos.pow(2.0); });
  add_case("maximum(scalar)", [&]{ return x.maximum(0.0); });
  add_case("minimum(scalar)", [&]{ return x.minimum(0.0); });
  add_case("eq(scalar)", [&]{ return x.eq(0.0); });
  add_case("ne(scalar)", [&]{ return x.ne(0.0); });
  add_case("lt(scalar)", [&]{ return x.lt(0.0); });
  add_case("le(scalar)", [&]{ return x.le(0.0); });
  add_case("gt(scalar)", [&]{ return x.gt(0.0); });
  add_case("ge(scalar)", [&]{ return x.ge(0.0); });
  add_case("add(series)", [&]{ return x.add(y); });
  add_case("sub(series)", [&]{ return x.sub(y); });
  add_case("mul(series)", [&]{ return x.mul(y); });
  add_case("div(series)", [&]{ return pos.div(pos.add(1.0)); });
  add_case("pow(series)", [&]{ return pos.pow(pos.mul(0.0).add(2.0)); });

  // Bitwise / logical
  add_case("bitwise_and", [&]{ return bi.bitwise_and(bi.add(1)); });
  add_case("bitwise_or", [&]{ return bi.bitwise_or(bi.add(1)); });
  add_case("bitwise_xor", [&]{ return bi.bitwise_xor(bi.add(1)); });
  add_case("bitwise_not", [&]{ return bi.bitwise_not(); });
  add_case("logical_and", [&]{ return x.gt(0.0).logical_and(x.lt(3.0)); });
  add_case("logical_or", [&]{ return x.gt(0.0).logical_or(x.lt(-2.0)); });
  add_case("logical_xor", [&]{ return x.gt(0.0).logical_xor(x.lt(0.0)); });
  add_case("logical_not", [&]{ return x.gt(0.0).logical_not(); });

  // Unary math
  add_case("exp", [&]{ return pos.exp(); });
  add_case("log", [&]{ return pos.log(); });
  add_case("sqrt", [&]{ return pos.sqrt(); });
  add_case("sin", [&]{ return x.sin(); });
  add_case("cos", [&]{ return x.cos(); });
  add_case("tan", [&]{ return unit.tan(); });
  add_case("asin", [&]{ return unit.asin(); });
  add_case("acos", [&]{ return unit.acos(); });
  add_case("atan", [&]{ return x.atan(); });
  add_case("sinh", [&]{ return x.sinh(); });
  add_case("cosh", [&]{ return x.cosh(); });
  add_case("tanh", [&]{ return x.tanh(); });
  add_case("floor", [&]{ return x.div(2.0).floor(); });
  add_case("ceil", [&]{ return x.div(2.0).ceil(); });
  add_case("round", [&]{ return x.div(2.0).round(); });
  add_case("trunc", [&]{ return x.div(2.0).trunc(); });
  add_case("reciprocal", [&]{ return pos.reciprocal(); });
  add_case("sigmoid", [&]{ return x.sigmoid(); });
  add_case("relu", [&]{ return x.relu(); });
  add_case("log1p", [&]{ return pos.log1p(); });
  add_case("expm1", [&]{ return unit.expm1(); });
  add_case("erf", [&]{ return unit.erf(); });
  add_case("erfc", [&]{ return unit.erfc(); });
  add_case("lgamma", [&]{ return pos.lgamma(); });
  add_case("asinh", [&]{ return x.asinh(); });
  add_case("acosh", [&]{ return pos.add(1.0).acosh(); });
  add_case("atanh", [&]{ return unit.atanh(); });
  add_case("isnan", [&]{ return x.isnan(); });
  add_case("isinf", [&]{ return x.isinf(); });
  add_case("isfinite", [&]{ return x.isfinite(); });

  // Reductions / scans
  add_case("sum(dim)", [&]{ return mm.sum(1, false); });
  add_case("mean(dim)", [&]{ return mm.mean(1, true); });
  add_case("amax(dim)", [&]{ return mm.amax(1, false); });
  add_case("amin(dim)", [&]{ return mm.amin(1, false); });
  add_case("all(dim)", [&]{ return mm.gt(0.0).all(1, false); });
  add_case("any(dim)", [&]{ return mm.gt(7.0).any(1, false); });
  add_case("mean()", [&]{ return mm.mean(); });
  add_case("norm", [&]{ return x.norm(2.0); });
  add_case("var", [&]{ return x.var(false); });
  add_case("std", [&]{ return x.std(false); });
  add_case("cumsum", [&]{ return mm.cumsum(1, false); });
  add_case("cumprod", [&]{ return mm.cumprod(1, false); });

  // Masking / clamping
  add_case("where(series)", [&]{ return x.where(cond, y); });
  add_case("where(scalar)", [&]{ return x.where(cond, 42.0); });
  add_case("clamp_min(scalar)", [&]{ return x.clamp_min(0.0); });
  add_case("clamp_max(scalar)", [&]{ return x.clamp_max(0.0); });
  add_case("clamp_min(series)", [&]{ return x.clamp_min(y); });
  add_case("clamp_max(series)", [&]{ return x.clamp_max(y); });
  add_case("nan_to_num", [&]{ return x.nan_to_num(0.0, 1e9, -1e9); });
  add_case("masked_select", [&]{ return m.masked_select(mask); });
  add_case("masked_scatter", [&]{ return m.masked_scatter(mask, n); });

  // Gather / scatter family
  add_case("gather", [&]{ return gather_src.gather(1, gather_idx); });
  add_case("scatter", [&]{ return scatter_base.scatter(1, gather_idx, scatter_val); });
  add_case("scatter_add", [&]{ return scatter_base.scatter_add(1, gather_idx, scatter_val); });
  add_case("scatter_reduce_sum", [&]{ return scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true); });
  add_case("scatter_reduce_amax", [&]{ return scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true); });
  add_case("scatter_reduce_amin", [&]{ return scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true); });

  // Integer analytics
  add_case("unique", [&]{ return i.unique(true); });
  add_case("bincount", [&]{ return i.sub(1.0).to(ScalarType::Int64).bincount(0); });
  add_case("histc", [&]{ return pos.histc(5, 0.0, 5.0); });

  // In-place variants (clone first so each case is isolated)
  add_case("exp_", [&]{ auto t = pos.clone(); t.exp_(); return t; });
  add_case("log_", [&]{ auto t = pos.clone(); t.log_(); return t; });
  add_case("sqrt_", [&]{ auto t = pos.clone(); t.sqrt_(); return t; });
  add_case("sin_", [&]{ auto t = x.clone(); t.sin_(); return t; });
  add_case("cos_", [&]{ auto t = x.clone(); t.cos_(); return t; });
  add_case("tanh_", [&]{ auto t = x.clone(); t.tanh_(); return t; });
  add_case("tan_", [&]{ auto t = unit.clone(); t.tan_(); return t; });
  add_case("asin_", [&]{ auto t = unit.clone(); t.asin_(); return t; });
  add_case("acos_", [&]{ auto t = unit.clone(); t.acos_(); return t; });
  add_case("atan_", [&]{ auto t = x.clone(); t.atan_(); return t; });
  add_case("sinh_", [&]{ auto t = x.clone(); t.sinh_(); return t; });
  add_case("cosh_", [&]{ auto t = x.clone(); t.cosh_(); return t; });
  add_case("floor_", [&]{ auto t = x.div(2.0); t.floor_(); return t; });
  add_case("ceil_", [&]{ auto t = x.div(2.0); t.ceil_(); return t; });
  add_case("round_", [&]{ auto t = x.div(2.0); t.round_(); return t; });
  add_case("trunc_", [&]{ auto t = x.div(2.0); t.trunc_(); return t; });
  add_case("reciprocal_", [&]{ auto t = pos.clone(); t.reciprocal_(); return t; });
  add_case("sigmoid_", [&]{ auto t = x.clone(); t.sigmoid_(); return t; });
  add_case("relu_", [&]{ auto t = x.clone(); t.relu_(); return t; });
  add_case("logical_not_", [&]{ auto t = x.gt(0.0); t.logical_not_(); return t; });
  add_case("log1p_", [&]{ auto t = pos.clone(); t.log1p_(); return t; });
  add_case("expm1_", [&]{ auto t = unit.clone(); t.expm1_(); return t; });
  add_case("erf_", [&]{ auto t = unit.clone(); t.erf_(); return t; });
  add_case("erfc_", [&]{ auto t = unit.clone(); t.erfc_(); return t; });
  add_case("lgamma_", [&]{ auto t = pos.clone(); t.lgamma_(); return t; });
  add_case("asinh_", [&]{ auto t = x.clone(); t.asinh_(); return t; });
  add_case("acosh_", [&]{ auto t = pos.add(1.0); t.acosh_(); return t; });
  add_case("atanh_", [&]{ auto t = unit.clone(); t.atanh_(); return t; });
  add_case("isnan_", [&]{ auto t = x.clone(); t.isnan_(); return t; });
  add_case("isinf_", [&]{ auto t = x.clone(); t.isinf_(); return t; });
  add_case("isfinite_", [&]{ auto t = x.clone(); t.isfinite_(); return t; });
  add_case("add_", [&]{ auto t = x.clone(); t.add_(1.0); return t; });
  add_case("sub_", [&]{ auto t = x.clone(); t.sub_(1.0); return t; });
  add_case("mul_", [&]{ auto t = x.clone(); t.mul_(2.0); return t; });
  add_case("div_", [&]{ auto t = x.clone(); t.div_(2.0); return t; });
  add_case("pow_", [&]{ auto t = pos.clone(); t.pow_(2.0); return t; });
  add_case("minimum_", [&]{ auto t = x.clone(); t.minimum_(0.0); return t; });
  add_case("maximum_", [&]{ auto t = x.clone(); t.maximum_(0.0); return t; });
  add_case("cumsum_", [&]{ auto t = mm.clone(); pluss::codem::_compat::invoke_("cumsum_", t, {1LL, false}); return t; });
  add_case("cumprod_", [&]{ auto t = mm.clone(); pluss::codem::_compat::invoke_("cumprod_", t, {1LL, false}); return t; });
  add_case("clamp_min_", [&]{ auto t = x.clone(); t.clamp_min_(0.0); return t; });
  add_case("clamp_max_", [&]{ auto t = x.clone(); t.clamp_max_(0.0); return t; });
  add_case("nan_to_num_", [&]{ auto t = x.clone(); t.nan_to_num_(0.0, 1e9, -1e9); return t; });
  add_case("where_", [&]{ auto t = x.clone(); t.where_(cond, y); return t; });
  add_case("scatter_", [&]{ auto t = scatter_base.clone(); pluss::codem::_compat::invoke_("scatter_", t, {1LL, gather_idx, scatter_val}); return t; });
  add_case("scatter_add_", [&]{ auto t = scatter_base.clone(); pluss::codem::_compat::invoke_("scatter_add_", t, {1LL, gather_idx, scatter_val}); return t; });
  add_case("masked_scatter_", [&]{ auto t = m.clone(); pluss::codem::_compat::invoke_("masked_scatter_", t, {mask, n}); return t; });
  add_case("put_", [&]{ auto t = gather_src.clone(); pluss::codem::_compat::invoke_("put_", t, {flat_idx, flat_src, false}); return t; });

  // Torch+MATLAB-hybrid routed methods (from SeriesMethodsGen wrappers)
  add_case("stat.pdf()", [&]{ return pos.stat.pdf(); });
  add_case("stat.logpdf()", [&]{ return pos.stat.logpdf(); });
  add_case("stat.logcdf()", [&]{ return pos.stat.logcdf(); });
  add_case("stat.cdf()", [&]{ return pos.stat.cdf(); });
  add_case("dct", [&]{ return x.dct(); });
  add_case("idct", [&]{ return x.dct().idct(); });
  add_case("dst", [&]{ return x.dst(); });
  add_case("idst", [&]{ return x.dst().idst(); });
  add_case("hilbert", [&]{ return x.hilbert(); });
  add_case("histcounts", [&]{ return pos.histcounts(5, 0.0, 5.0); });
  add_case("skewness", [&]{ return x.stat.skewness(); });
  add_case("kurtosis", [&]{ return x.stat.kurtosis(); });
  add_case("nanvar", [&]{ return x.nanvar(); });
  add_case("nanstd", [&]{ return x.nanstd(); });
  add_case("ediff1d", [&]{ return x.ediff1d(); });
  add_case("movsum", [&]{ return x.stat.movsum(3); });
  add_case("movmean", [&]{ return x.stat.movmean(3); });
  add_case("movmax", [&]{ return x.stat.movmax(3); });
  add_case("movmin", [&]{ return x.stat.movmin(3); });
  add_case("movstd", [&]{ return x.stat.movstd(3); });
  add_case("movvar", [&]{ return x.stat.movvar(3); });
  add_case("pdist2", [&]{ return m.pdist2(n); });
  add_case("cdist", [&]{ return m.cdist(n); });
  add_case("normr", [&]{ return m.normr(); });
  add_case("normc", [&]{ return m.normc(); });
  add_case("standardize", [&]{ return x.standardize(); });
  add_case("center", [&]{ return x.center(); });
  add_case("besselj", [&]{ return pos.besselj(); });
  add_case("bessely", [&]{ return pos.bessely(); });
  add_case("besseli", [&]{ return pos.besseli(); });
  add_case("besselk", [&]{ return pos.besselk(); });
  add_case("beta", [&]{ return pos.beta(2.0); });
  add_case("betaln", [&]{ return pos.betaln(2.0); });
  add_case("erfcinv", [&]{ return unit.abs().erfcinv(); });
  add_case("zeta", [&]{ return pos.zeta(1.0); });
  add_case("airy", [&]{ return pos.airy(); });
  add_case("unwrap", [&]{ return x.unwrap(); });
  add_case("cplxpair", [&]{ return x.hilbert().cplxpair(); });
  add_case("vdot", [&]{ return x.vdot(y); });
  add_case("real", [&]{ return x.hilbert().real(); });
  add_case("imag", [&]{ return x.hilbert().imag(); });
  add_case("angle", [&]{ return x.hilbert().angle(); });
  add_case("fftshift", [&]{ return mm.fftshift(); });
  add_case("ifftshift", [&]{ return mm.fftshift().ifftshift(); });
  add_case("abs2", [&]{ return x.hilbert().abs2(); });

  std::cout << "Running " << cases.size() << " Series method demos on "
            << ((run_platform == pluss::pMPS) ? "MPS" : "CPU") << "\n\n";
  pluss::codem::PrintOptions po;
  po.edge_items = 2;
  po.max_elements = 12;

  std::size_t ok = 0;
  std::size_t fail = 0;
  for (std::size_t i_case = 0; i_case < cases.size(); ++i_case) {
    const auto& c = cases[i_case];
    try {
      auto out = c.run();
      ++ok;
      std::cout << "[" << (i_case + 1) << "/" << cases.size() << "] " << c.name << " : OK\n";
      std::cout << pluss::codem::to_string(out, po) << "\n\n";
    } catch (const std::exception& e) {
      ++fail;
      std::cout << "[" << (i_case + 1) << "/" << cases.size() << "] " << c.name << " : FAIL -> " << e.what() << "\n\n";
    } catch (...) {
      ++fail;
      std::cout << "[" << (i_case + 1) << "/" << cases.size() << "] " << c.name << " : FAIL -> unknown exception\n\n";
    }
  }

  std::cout << "Summary: OK=" << ok << " FAIL=" << fail << " TOTAL=" << cases.size() << "\n";
  return 0;
}
