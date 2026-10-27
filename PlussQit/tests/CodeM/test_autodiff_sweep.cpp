// Systematic finite-difference gradcheck sweep over differentiable float Series
// ops. Discovery harness for the autodiff gap-closing work: it classifies every
// op it knows about so the confirmed correctness-hole list is produced in one
// run.
//
//   OK            a reverse node forms AND the input sensitivity matches a
//                 central finite-difference of sum(op(x)) along a fixed tangent
//   NO_GRAD_NODE  the tracked input flows in but the output has no source link
//   NO_SENS       reverse() ran but the input never received a sensitivity
//   MISMATCH      a sensitivity was produced but disagrees with finite diff
//   THREW         forward or reverse raised
//   NONDIFF       op is non-differentiable by design; only checked for "no node"
//
// The authoritative per-op check against LibTorch lives in
// test_patch177_cpu_torch_autograd_parity.cpp; entries graduate there as the
// backward for each hole is implemented.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <set>

#include <pluss/codem/codem.h>
#include <pluss/codem/series_factory.h>

#include "CodeM/compat/Invoke.h"  // _compat::set_strict_unrouted

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
namespace tape = pluss::codem::series_tape;

constexpr double kEps = 1e-6;
constexpr double kRelTol = 2e-4;
constexpr double kAbsTol = 2e-4;

enum class Status { OK, NO_GRAD_NODE, NO_SENS, MISMATCH, THREW, NONDIFF, IDENTITY };

const char* status_name(Status s) {
  switch (s) {
    case Status::OK: return "OK";
    case Status::NO_GRAD_NODE: return "NO_GRAD_NODE";
    case Status::NO_SENS: return "NO_SENS";
    case Status::MISMATCH: return "MISMATCH";
    case Status::THREW: return "THREW";
    case Status::NONDIFF: return "NONDIFF";
    case Status::IDENTITY: return "IDENTITY (op returned input unchanged)";
  }
  return "?";
}

struct Result { Status status{Status::OK}; std::string detail; };
std::map<std::string, Result> g_reg;

std::vector<double> tangent(std::size_t n, std::uint64_t seed) {
  std::vector<double> v(n);
  std::uint64_t s = seed * 6364136223846793005ULL + 1442695040888963407ULL;
  for (std::size_t i = 0; i < n; ++i) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    v[i] = (static_cast<double>(s >> 11) / static_cast<double>(1ULL << 53)) * 2.0 - 1.0;
  }
  return v;
}

Series f64(const std::vector<double>& data) {
  return make_series(data, {static_cast<std::int64_t>(data.size())}).to(ScalarType::Float64);
}

double dot(const std::vector<double>& a, const std::vector<double>& b) {
  double s = 0.0;
  const std::size_t n = std::min(a.size(), b.size());
  for (std::size_t i = 0; i < n; ++i) s += a[i] * b[i];
  return s;
}

double sum_of(const Series& y) {
  const auto v = y.to(ScalarType::Float64).host<double>();
  double s = 0.0;
  for (double x : v) s += x;
  return s;
}

double fd_directional(const std::function<Series(const Series&)>& op,
                      const std::vector<double>& x, const std::vector<double>& dir) {
  std::vector<double> xp(x), xm(x);
  for (std::size_t i = 0; i < x.size(); ++i) {
    xp[i] += kEps * dir[i];
    xm[i] -= kEps * dir[i];
  }
  return (sum_of(op(f64(xp))) - sum_of(op(f64(xm)))) / (2.0 * kEps);
}

// True when every element of `y` equals the corresponding element of `input`
// bit-for-bit (NaN treated as matching NaN). A routed differentiable op must
// transform its input somehow; a result that is the input verbatim, with no
// tape node attached, is the signature of the generic unrouted backstop handing
// the receiver straight back -- which then finite-difference-checks as correct
// only because identity's gradient (all ones) is trivially self-consistent.
bool bit_identical(const Series& y, const std::vector<double>& input) {
  if (static_cast<std::size_t>(y.numel()) != input.size()) return false;
  const auto yv = y.to(ScalarType::Float64).host<double>();
  if (yv.size() != input.size()) return false;
  for (std::size_t i = 0; i < yv.size(); ++i) {
    const double a = yv[i], b = input[i];
    if (a == b) continue;
    if (std::isnan(a) && std::isnan(b)) continue;
    return false;
  }
  return true;
}

void U(const std::string& name, const std::vector<double>& input,
       std::function<Series(const Series&)> op, bool nondiff = false) {
  Result res;
  try {
    Series x = f64(input);
    x.tracks_slope_(true);
    Series y = op(x);
    const bool has_link = tape::source_info(y).has_value();
    const bool has_node = has_link || tape::tracks_slope(y);
    if (!has_node) { res.status = nondiff ? Status::NONDIFF : Status::NO_GRAD_NODE; g_reg[name] = res; return; }
    if (nondiff) { res.status = Status::NONDIFF; res.detail = "grad node formed"; g_reg[name] = res; return; }
    if (!has_link && bit_identical(y, input)) {
      res.status = Status::IDENTITY;
      res.detail = "op returned its input verbatim with no tape node (unrouted backstop?)";
      g_reg[name] = res;
      return;
    }
    y.sum().reverse();
    if (!x.has_sensitivity()) { res.status = Status::NO_SENS; g_reg[name] = res; return; }
    const auto g = x.sensitivity().to(ScalarType::Float64).host<double>();
    const auto v = tangent(input.size(), 0x51ED);
    const double analytic = dot(g, v);
    const double numeric = fd_directional(op, input, v);
    if (std::abs(analytic - numeric) > kAbsTol + kRelTol * std::abs(numeric)) {
      res.status = Status::MISMATCH;
      res.detail = "analytic=" + std::to_string(analytic) + " fd=" + std::to_string(numeric);
    }
  } catch (const std::exception& e) { res.status = Status::THREW; res.detail = e.what(); }
  g_reg[name] = res;
}

void B(const std::string& name, const std::vector<double>& la, const std::vector<double>& lb,
       std::function<Series(const Series&, const Series&)> op) {
  for (int side = 0; side < 2; ++side) {
    const std::string tag = name + (side == 0 ? "[lhs]" : "[rhs]");
    Result res;
    try {
      Series a = f64(la), b = f64(lb);
      if (side == 0) a.tracks_slope_(true); else b.tracks_slope_(true);
      Series y = op(a, b);
      const bool has_node = tape::source_info(y).has_value() || tape::tracks_slope(y);
      if (!has_node) { res.status = Status::NO_GRAD_NODE; g_reg[tag] = res; continue; }
      y.sum().reverse();
      Series& tracked = (side == 0) ? a : b;
      if (!tracked.has_sensitivity()) { res.status = Status::NO_SENS; g_reg[tag] = res; continue; }
      const auto g = tracked.sensitivity().to(ScalarType::Float64).host<double>();
      const auto& base = (side == 0) ? la : lb;
      const auto v = tangent(base.size(), 0x51ED + side);
      const double analytic = dot(g, v);
      const auto unary = [&](const Series& p) { return side == 0 ? op(p, f64(lb)) : op(f64(la), p); };
      const double numeric = fd_directional(unary, base, v);
      if (std::abs(analytic - numeric) > kAbsTol + kRelTol * std::abs(numeric)) {
        res.status = Status::MISMATCH;
        res.detail = "analytic=" + std::to_string(analytic) + " fd=" + std::to_string(numeric);
      }
    } catch (const std::exception& e) { res.status = Status::THREW; res.detail = e.what(); }
    g_reg[tag] = res;
  }
}

// Flip the process-global strict-unrouted flag for the duration of a scope and
// restore whatever it was before. Keeps the setting from leaking into the other
// test cases that share this executable.
struct StrictUnroutedScope {
  bool prev;
  StrictUnroutedScope() : prev(pluss::codem::_compat::strict_unrouted()) {
    pluss::codem::_compat::set_strict_unrouted(true);
  }
  ~StrictUnroutedScope() { pluss::codem::_compat::set_strict_unrouted(prev); }
};

const std::vector<double> gen{-0.7, -0.2, 0.15, 0.6, 1.3};
const std::vector<double> pos{0.15, 0.4, 0.9, 1.6, 2.4};
const std::vector<double> unit{-0.85, -0.35, 0.1, 0.5, 0.8};
const std::vector<double> gt1{1.1, 1.4, 2.0, 3.1, 5.0};
const std::vector<double> posb{0.3, 0.7, 1.1, 1.9, 2.7};
const std::int64_t I0 = 0, I2 = 2;

}  // namespace

TEST_CASE("autodiff sweep", "[autodiff-sweep]") {
  // Any call that falls through to the generic unrouted-op backstop on CPU must
  // raise, not log a warning and hand back the receiver unchanged. Without this
  // a placeholder overload finite-difference-checks as "correct" because the
  // silent identity it returns has a self-consistent (all-ones) gradient.
  const StrictUnroutedScope strict_scope;

  // ---- unary elementwise ----
  U("neg", gen, [](const Series& x) { return x.neg(); });
  U("abs", gen, [](const Series& x) { return x.abs(); });
  U("square", gen, [](const Series& x) { return x.square(); });
  U("reciprocal", pos, [](const Series& x) { return x.reciprocal(); });
  U("sqrt", pos, [](const Series& x) { return x.sqrt(); });
  U("rsqrt", pos, [](const Series& x) { return x.rsqrt(); });
  U("cbrt", pos, [](const Series& x) { return x.cbrt(); });
  U("exp", gen, [](const Series& x) { return x.exp(); });
  U("exp2", gen, [](const Series& x) { return x.exp2(); });
  U("expm1", gen, [](const Series& x) { return x.expm1(); });
  U("log", pos, [](const Series& x) { return x.log(); });
  U("log2", pos, [](const Series& x) { return x.log2(); });
  U("log10", pos, [](const Series& x) { return x.log10(); });
  U("log1p", unit, [](const Series& x) { return x.log1p(); });
  U("sin", gen, [](const Series& x) { return x.sin(); });
  U("cos", gen, [](const Series& x) { return x.cos(); });
  U("tan", unit, [](const Series& x) { return x.tan(); });
  U("asin", unit, [](const Series& x) { return x.asin(); });
  U("acos", unit, [](const Series& x) { return x.acos(); });
  U("atan", gen, [](const Series& x) { return x.atan(); });
  U("sinh", gen, [](const Series& x) { return x.sinh(); });
  U("cosh", gen, [](const Series& x) { return x.cosh(); });
  U("tanh", gen, [](const Series& x) { return x.tanh(); });
  U("asinh", gen, [](const Series& x) { return x.asinh(); });
  U("acosh", gt1, [](const Series& x) { return x.acosh(); });
  U("atanh", unit, [](const Series& x) { return x.atanh(); });
  U("erf", gen, [](const Series& x) { return x.erf(); });
  U("erfc", gen, [](const Series& x) { return x.erfc(); });
  U("sigmoid", gen, [](const Series& x) { return x.sigmoid(); });
  U("relu", gen, [](const Series& x) { return x.relu(); });
  U("gelu", gen, [](const Series& x) { return x.gelu(); });
  U("silu", gen, [](const Series& x) { return x.silu(); });
  U("softplus", gen, [](const Series& x) { return x.softplus(); });
  // Deliberately non-finite lanes: identity must NOT pass here. The replaced
  // lanes contribute a zero gradient; the finite lanes behave normally.
  U("nan_to_num", {std::nan(""), -0.2, 0.6, 1.3, -0.7},
    [](const Series& x) { return x.nan_to_num(); });
  U("erfinv", unit, [](const Series& x) { return x.erfinv(); });
  U("digamma", pos, [](const Series& x) { return x.digamma(); });
  U("lgamma", pos, [](const Series& x) { return x.lgamma(); });
  U("i0", gen, [](const Series& x) { return x.i0(); });
  U("sinc", gen, [](const Series& x) { return x.sinc(); });
  U("logit", unit, [](const Series& x) { return x.mul(0.25).add(0.5).logit(0.0); });
  U("deg2rad", gen, [](const Series& x) { return x.deg2rad(); });
  U("rad2deg", gen, [](const Series& x) { return x.rad2deg(); });
  U("sign", gen, [](const Series& x) { return x.sign(); }, true);
  U("floor", gen, [](const Series& x) { return x.floor(); }, true);
  U("ceil", gen, [](const Series& x) { return x.ceil(); }, true);
  U("round", gen, [](const Series& x) { return x.round(); }, true);
  U("trunc", gen, [](const Series& x) { return x.trunc(); }, true);

  // ---- binary elementwise ----
  B("add", gen, pos, [](const Series& a, const Series& b) { return a.add(b); });
  B("sub", gen, pos, [](const Series& a, const Series& b) { return a.sub(b); });
  B("mul", gen, pos, [](const Series& a, const Series& b) { return a.mul(b); });
  B("div", gen, pos, [](const Series& a, const Series& b) { return a.div(b); });
  B("pow", pos, unit, [](const Series& a, const Series& b) { return a.pow(b); });
  B("maximum", gen, unit, [](const Series& a, const Series& b) { return a.maximum(b); });
  B("minimum", gen, unit, [](const Series& a, const Series& b) { return a.minimum(b); });
  B("fmax", gen, unit, [](const Series& a, const Series& b) { return a.fmax(b); });
  B("fmin", gen, unit, [](const Series& a, const Series& b) { return a.fmin(b); });
  B("hypot", pos, posb, [](const Series& a, const Series& b) { return a.hypot(b); });
  B("atan2", gen, pos, [](const Series& a, const Series& b) { return a.atan2(b); });
  B("logaddexp", gen, unit, [](const Series& a, const Series& b) { return a.logaddexp(b); });
  B("logaddexp2", gen, unit, [](const Series& a, const Series& b) { return a.logaddexp2(b); });
  B("xlogy", pos, posb, [](const Series& a, const Series& b) { return a.xlogy(b); });
  B("copysign", gen, unit, [](const Series& a, const Series& b) { return a.copysign(b); });
  B("fmod", gen, posb, [](const Series& a, const Series& b) { return a.fmod(b); });
  B("remainder", gen, posb, [](const Series& a, const Series& b) { return a.remainder(b); });
  B("addcmul", gen, pos, [](const Series& a, const Series& b) { return a.addcmul(b, b, 0.5); });
  B("addcdiv", gen, pos, [](const Series& a, const Series& b) { return a.addcdiv(b, b.add(3.0), 0.5); });
  B("lerp", gen, pos, [](const Series& a, const Series& b) { return a.lerp(b, 0.3); });

  // ---- reductions ----
  U("sum", gen, [](const Series& x) { return x.sum(); });
  U("mean", gen, [](const Series& x) { return x.mean(); });
  U("prod", pos, [](const Series& x) { return x.prod(); });
  U("amax", gen, [](const Series& x) { return x.amax(); });
  U("amin", gen, [](const Series& x) { return x.amin(); });
  U("logsumexp", gen, [](const Series& x) { return x.logsumexp(); });
  U("nansum", gen, [](const Series& x) { return x.nansum(); });
  U("norm", gen, [](const Series& x) { return x.norm(); });
  U("median", gen, [](const Series& x) { return x.median(); });
  U("dist", gen, [](const Series& x) { return x.dist(x.mul(0.5)); });

  // ---- cumulative ----
  U("cumsum", gen, [](const Series& x) { return x.cumsum(I0, false); });
  U("cumprod", pos, [](const Series& x) { return x.cumprod(I0, false); });
  U("cummax", gen, [](const Series& x) { return x.cummax(I0); });
  U("cummin", gen, [](const Series& x) { return x.cummin(I0); });
  U("logcumsumexp", gen, [](const Series& x) { return x.logcumsumexp(I0); });

  // ---- structural / view ----
  U("reshape", gen, [](const Series& x) { return x.reshape({5, 1}); });
  U("flatten", gen, [](const Series& x) { return x.flatten(); });
  U("flip", gen, [](const Series& x) { return x.flip(I0); });
  U("roll", gen, [](const Series& x) { return x.roll(I2); });
  U("repeat", gen, [](const Series& x) { return x.repeat(std::vector<std::int64_t>{2}); });
  U("tril_2d", gen, [](const Series& x) { return x.reshape({5, 1}).mul(x.reshape({1, 5})).tril(0); });
  U("triu_2d", gen, [](const Series& x) { return x.reshape({5, 1}).mul(x.reshape({1, 5})).triu(0); });
  U("diagonal_2d", gen, [](const Series& x) { return x.reshape({5, 1}).mul(x.reshape({1, 5})).diagonal(0, 0, 1); });

  // ---- dim-variant reductions (input reshaped to [1,5] so dim 1 has extent) --
  const auto row = [](const Series& x) { return x.reshape({1, 5}); };
  U("sum_dim", gen, [&](const Series& x) { return row(x).sum(1, false); });
  U("mean_dim", gen, [&](const Series& x) { return row(x).mean(1, false); });
  U("prod_dim", pos, [&](const Series& x) { return row(x).prod(1, false); });
  U("amax_dim", gen, [&](const Series& x) { return row(x).amax(1, false); });
  U("amin_dim", gen, [&](const Series& x) { return row(x).amin(1, false); });
  U("norm_dim", gen, [&](const Series& x) { return row(x).norm(1, false); });
  U("logsumexp_dim", gen, [&](const Series& x) { return row(x).logsumexp(1, false); });
  U("nansum_dim", gen, [&](const Series& x) { return row(x).nansum(1, false); });
  U("softmax_dim", gen, [&](const Series& x) { return row(x).softmax(1); });
  U("log_softmax_dim", gen, [&](const Series& x) { return row(x).log_softmax(1); });
  U("cumsum_keepdim", gen, [&](const Series& x) { return row(x).cumsum(1, false); });

  // ---- order selection value paths ----
  U("max_dim_values", gen, [&](const Series& x) { return row(x).max(1); });
  U("min_dim_values", gen, [&](const Series& x) { return row(x).min(1); });
  U("median_dim", gen, [&](const Series& x) { return row(x).median(1); });
  U("sort_values", gen, [&](const Series& x) { return row(x).sort(1, false); });
  U("topk_values", gen, [&](const Series& x) { return row(x).topk(3, 1, true, true); });
  U("kthvalue_values", gen, [&](const Series& x) { return row(x).kthvalue(2, 1, false); });

  // ---- indexing / gather / scatter ----
  U("index_select", gen, [](const Series& x) {
    return x.index_select(0, make_series(std::vector<std::int64_t>{0, 2, 2, 4}, {4}));
  });
  U("masked_fill", gen, [](const Series& x) {
    return x.masked_fill(make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0}, {5}).to(ScalarType::Bool), 2.0);
  });

  // ---- indexing / gather / scatter / select / slice ----
  const auto i64 = [](std::vector<std::int64_t> v) {
    return make_series(v, {static_cast<std::int64_t>(v.size())});
  };
  U("gather", gen, [&](const Series& x) {
    return row(x).gather(1, make_series(std::vector<std::int64_t>{4, 0, 2, 1, 3}, {1, 5}));
  });
  U("scatter_src", gen, [&](const Series& x) {
    Series base = make_series(std::vector<double>{0, 0, 0, 0, 0}, {1, 5}).to(ScalarType::Float64);
    return base.scatter(1, make_series(std::vector<std::int64_t>{0, 1, 2, 3, 4}, {1, 5}), row(x));
  });
  U("scatter_add_src", gen, [&](const Series& x) {
    Series base = make_series(std::vector<double>{0, 0, 0}, {1, 3}).to(ScalarType::Float64);
    return base.scatter_add(1, make_series(std::vector<std::int64_t>{0, 1, 2, 0, 1}, {1, 5}), row(x));
  });
  U("index_add_src", gen, [&](const Series& x) {
    Series base = make_series(std::vector<double>{1, 1, 1}, {3}).to(ScalarType::Float64);
    return base.index_add(0, i64({0, 2, 1, 0, 2}), x, 1.0);
  });
  U("take", gen, [&](const Series& x) { return x.take(i64({3, 1, 1, 0})); });
  U("masked_select", gen, [&](const Series& x) {
    return x.masked_select(i64({1, 0, 1, 1, 0}).to(ScalarType::Bool));
  });
  U("masked_scatter", gen, [&](const Series& x) {
    // mask has 3 True -> src must supply exactly 3 values.
    Series base = make_series(std::vector<double>{9, 9, 9, 9, 9}, {5}).to(ScalarType::Float64);
    return base.masked_scatter(i64({1, 0, 1, 0, 1}).to(ScalarType::Bool), x.slice(0, 0, 3));
  });
  U("select", gen, [&](const Series& x) { return row(x).select(1, 2); });
  U("slice", gen, [](const Series& x) { return x.slice(0, 1, 4); });
  U("transpose", gen, [&](const Series& x) { return row(x).transpose(0, 1); });
  U("unfold", gen, [](const Series& x) { return x.unfold(0, 2, 1); });

  // ---- matmul family ----
  U("matmul_mm", gen, [&](const Series& x) {
    return row(x).matmul(row(x).transpose(0, 1));
  });
  U("mv", gen, [&](const Series& x) {
    return x.reshape({5, 1}).mul(row(x)).mv(x);
  });
  U("dot", gen, [](const Series& x) { return x.dot(x.mul(0.5)); });
  U("addmm", gen, [&](const Series& x) {
    Series bias = make_series(std::vector<double>{0.1}, {1, 1}).to(ScalarType::Float64);
    return bias.addmm(row(x), row(x).transpose(0, 1));
  });
  U("trace", gen, [&](const Series& x) {
    return x.reshape({5, 1}).mul(row(x)).trace();
  });

  // ---- report ----
  std::map<Status, std::vector<std::string>> grouped;
  for (const auto& [name, r] : g_reg) grouped[r.status].push_back(name);
  std::cout << "\n================ AUTOGRAD SWEEP REPORT ================\n";
  for (Status s : {Status::THREW, Status::IDENTITY, Status::NO_GRAD_NODE,
                   Status::NO_SENS, Status::MISMATCH, Status::NONDIFF, Status::OK}) {
    const auto& names = grouped[s];
    std::cout << status_name(s) << " (" << names.size() << "):\n";
    for (const auto& n : names) {
      std::cout << "    " << n;
      const auto& d = g_reg[n].detail;
      if (!d.empty()) std::cout << "   -- " << d.substr(0, 140);
      std::cout << "\n";
    }
  }
  std::cout << "======================================================\n";

  // Regression gate: once a family is swept clean it must stay clean. A new or
  // changed op that loses its reverse node, produces a wrong gradient, or
  // throws is a hard failure here.
  //
  // Known intentional exceptions (kept explicit so they can't hide real ones):
  //   copysign[rhs] -- the sign-source argument is non-differentiable; Pluss
  //                    forms no node rather than a zeros-grad node like torch.
  static const std::set<std::string> kAllowedNoNode{"copysign[rhs]"};

  std::vector<std::string> regressions;
  for (const auto& [name, r] : g_reg) {
    const bool bad = r.status == Status::THREW || r.status == Status::MISMATCH ||
                     r.status == Status::NO_SENS || r.status == Status::IDENTITY ||
                     (r.status == Status::NO_GRAD_NODE && !kAllowedNoNode.count(name));
    if (bad) regressions.push_back(std::string(status_name(r.status)) + "  " + name +
                                   (r.detail.empty() ? "" : "  (" + r.detail + ")"));
  }
  for (const auto& line : regressions) { UNSCOPED_INFO(line); }
  REQUIRE(regressions.empty());
}

// Higher-order reverse: d/dx [ sum( d/dx sum(f(x)) ) ] must equal the analytic
// second derivative. Uses create_graph so the first backward is itself taped.
TEST_CASE("autodiff sweep: second order", "[autodiff-sweep]") {
  auto second_deriv_check = [](const std::string& name, const std::vector<double>& in,
                               std::function<Series(const Series&)> f,
                               std::function<double(double)> d2) {
    const StrictUnroutedScope strict_scope;
    Series x = f64(in);
    x.tracks_slope_(true);
    Series y = f(x).sum();
    y.reverse(/*keep_graph=*/true, /*accumulate=*/false, /*track_higher_order=*/true);
    REQUIRE(x.has_sensitivity());
    Series g = x.sensitivity();
    INFO(name << ": first backward grad must itself be taped for higher order");
    REQUIRE(tape::tracks_slope(g));
    x.clear();
    g.sum().reverse();
    REQUIRE(x.has_sensitivity());
    const auto got = x.sensitivity().to(ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < got.size(); ++i) {
      INFO(name << " element " << i << " x=" << in[i]);
      REQUIRE(got[i] == Catch::Approx(d2(in[i])).margin(1e-8).epsilon(1e-9));
    }
  };
  second_deriv_check("exp", gen, [](const Series& x) { return x.exp(); },
                     [](double v) { return std::exp(v); });
  second_deriv_check("cube", gen, [](const Series& x) { return x.mul(x).mul(x); },
                     [](double v) { return 6.0 * v; });
  second_deriv_check("sin", gen, [](const Series& x) { return x.sin(); },
                     [](double v) { return -std::sin(v); });
  second_deriv_check("reciprocal", pos, [](const Series& x) { return x.reciprocal(); },
                     [](double v) { return 2.0 / (v * v * v); });
  second_deriv_check("sqrt", pos, [](const Series& x) { return x.sqrt(); },
                     [](double v) { return -0.25 / (v * std::sqrt(v)); });
  second_deriv_check("log", pos, [](const Series& x) { return x.log(); },
                     [](double v) { return -1.0 / (v * v); });
  second_deriv_check("tanh", gen, [](const Series& x) { return x.tanh(); },
                     [](double v) { const double t = std::tanh(v); return -2.0 * t * (1.0 - t * t); });
  second_deriv_check("sigmoid", gen, [](const Series& x) { return x.sigmoid(); },
                     [](double v) { const double s = 1.0 / (1.0 + std::exp(-v));
                                    return s * (1.0 - s) * (1.0 - 2.0 * s); });
  second_deriv_check("softplus", gen, [](const Series& x) { return x.softplus(); },
                     [](double v) { const double s = 1.0 / (1.0 + std::exp(-v)); return s * (1.0 - s); });
  // Exact-erf gelu (x.gelu() defaults to the tanh approximation, so ask for
  // exact): f(x) = x*Phi(x); f''(x) = phi(x)*(2 - x^2), phi = N(0,1) pdf.
  second_deriv_check("gelu_exact", gen, [](const Series& x) { return x.gelu(false); },
                     [](double v) {
                       const double phi = std::exp(-0.5 * v * v) / std::sqrt(2.0 * M_PI);
                       return phi * (2.0 - v * v);
                     });
}

// Zero-size inputs must not crash the forward or the reverse: the op yields an
// empty result, and the tracked input receives an empty (correctly shaped)
// sensitivity. A NaN/uninitialised read or a divide-by-zero in a length-0 loop
// shows up here.
TEST_CASE("autodiff sweep: empty shapes", "[autodiff-sweep]") {
  const StrictUnroutedScope strict_scope;
  const std::vector<double> none{};

  auto empty_ok = [&](const std::string& name, std::function<Series(const Series&)> f) {
    INFO(name);
    Series x = make_series(none, {0}).to(ScalarType::Float64);
    x.tracks_slope_(true);
    Series y = f(x);
    y.sum().reverse();  // sum() folds an empty reduction result to a 0 scalar
    REQUIRE(x.has_sensitivity());
    REQUIRE(x.sensitivity().numel() == 0);
    const auto g = x.sensitivity().host<double>();  // must be readable, not garbage
    REQUIRE(g.empty());
  };

  empty_ok("neg", [](const Series& x) { return x.neg(); });
  empty_ok("exp", [](const Series& x) { return x.exp(); });
  empty_ok("log", [](const Series& x) { return x.log(); });
  empty_ok("sqrt", [](const Series& x) { return x.sqrt(); });
  empty_ok("sigmoid", [](const Series& x) { return x.sigmoid(); });
  empty_ok("relu", [](const Series& x) { return x.relu(); });
  empty_ok("gelu", [](const Series& x) { return x.gelu(); });
  empty_ok("tanh", [](const Series& x) { return x.tanh(); });
  empty_ok("square", [](const Series& x) { return x.square(); });
  empty_ok("add_self", [](const Series& x) { return x.add(x); });
  empty_ok("mul_self", [](const Series& x) { return x.mul(x); });
  empty_ok("sum", [](const Series& x) { return x.sum(); });
  empty_ok("cumsum", [](const Series& x) { return x.cumsum(I0, false); });
  empty_ok("reshape", [](const Series& x) { return x.reshape({0, 1}); });
  empty_ok("flip", [](const Series& x) { return x.flip(I0); });
}
