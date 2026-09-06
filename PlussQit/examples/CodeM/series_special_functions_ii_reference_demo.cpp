#include <pluss/pluss.h>

#include <iostream>
#include <cmath>
#include <limits>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void show(const pluss::Series& value) {
  pluss::series::print(std::cout, value) << "\n\n";
}

void show_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

} // namespace

int main() {
  heading("digamma_landmarks");
  show(pluss::make_series({0.5, 1.0, 2.0, 3.0}).digamma());

  heading("digamma_signed_zero_and_poles");
  show(pluss::make_series({0.0, -0.0, -1.0, -0.5}).digamma());

  heading("digamma_reverse");
  auto dx = pluss::make_series({1.5, 2.5, 3.5});
  dx.tracks_slope_(true);
  dx.digamma().sum().reverse();
  show(dx.sensitivity());

  heading("polygamma_orders");
  const auto px = pluss::make_series({1.0, 2.0, 3.0});
  show(px.polygamma(0));
  show(px.polygamma(1));
  show(px.polygamma(2));

  heading("polygamma_negative_noninteger");
  show(pluss::make_series({-0.5, 0.5, 1.5}).polygamma(1));

  heading("polygamma_reverse");
  auto pr = pluss::make_series({1.5, 2.5});
  pr.tracks_slope_(true);
  pr.polygamma(1).sum().reverse();
  show(pr.sensitivity());

  heading("mvlgamma_dimension");
  const auto mx = pluss::make_series({3.0, 4.0});
  show(mx.mvlgamma(1));
  show(mx.mvlgamma(2));
  show(mx.mvlgamma(3));

  heading("mvlgamma_domain");
  show(pluss::make_series({0.5, 0.6, 1.0}).mvlgamma(2));

  heading("mvlgamma_reverse");
  auto mr = pluss::make_series({3.0, 4.0});
  mr.tracks_slope_(true);
  mr.mvlgamma(2).sum().reverse();
  show(mr.sensitivity());

  heading("erfinv_landmarks");
  show(pluss::make_series({-0.9, -0.5, 0.0, 0.5, 0.9}).erfinv());

  heading("erfinv_composition");
  show(pluss::make_series({-0.8, -0.2, 0.3, 0.95}).erfinv().erf());

  heading("erfinv_domain");
  show(pluss::make_series({-1.1, -1.0, 1.0, 1.1,
                           std::numeric_limits<double>::quiet_NaN()}).erfinv());

  heading("erfinv_reverse");
  auto er = pluss::make_series({0.0, 0.3, 0.6});
  er.tracks_slope_(true);
  er.erfinv().sum().reverse();
  show(er.sensitivity());

  heading("erfinv_normal_quantiles");
  show(pluss::make_series({0.025, 0.5, 0.975})
           .mul(2.0)
           .sub(1.0)
           .erfinv()
           .mul(std::sqrt(2.0)));

  heading("i0_even_and_growth");
  show(pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0, 8.0}).i0());

  heading("i0_reverse_is_odd");
  auto ir = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0});
  ir.tracks_slope_(true);
  ir.i0().sum().reverse();
  show(ir.sensitivity());

  heading("sinc_landmarks");
  show(pluss::make_series({-1.5, -1.0, -0.5, 0.0, 0.5, 1.0, 1.5}).sinc());

  heading("sinc_matrix_shape");
  show(pluss::make_series({{0.0, 0.25}, {0.5, 1.0}}).sinc());

  heading("sinc_integral_promotion");
  show(pluss::make_series({-1, 0, 1}).sinc());

  heading("sinc_reverse");
  auto sr = pluss::make_series({0.0, 0.5, 1.0, 1.5});
  sr.tracks_slope_(true);
  sr.sinc().sum().reverse();
  show(sr.sensitivity());

  heading("igamma_progression");
  show(pluss::make_series({2.0, 2.0, 2.0, 2.0})
           .igamma(pluss::make_series({0.0, 1.0, 3.0, 20.0})));

  heading("igamma_broadcast");
  show(pluss::make_series({1.0, 2.0}).reshape({2, 1})
           .igamma(pluss::make_series({0.5, 1.0, 2.0})));

  heading("igamma_complement_and_infinity");
  const auto ia = pluss::make_series({2.0, 2.0, 2.0});
  const auto ix = pluss::make_series({0.0, 3.0, std::numeric_limits<double>::infinity()});
  show(ia.igamma(ix));
  show(ia.igammac(ix));
  show(ia.igamma(ix).add(ia.igammac(ix)));

  heading("igamma_reverse_limit");
  auto igr = pluss::make_series({1.0, 3.0});
  igr.tracks_slope_(true);
  pluss::make_series({2.0, 2.0}).igamma(igr).sum().reverse();
  show(igr.sensitivity());

  heading("zeta_landmarks");
  show(pluss::make_series({2.0, 4.0, 6.0}).zeta(1.0));

  heading("zeta_shift");
  show(pluss::make_series({2.0, 3.0}).zeta(1.0)
           .sub(pluss::make_series({2.0, 3.0}).zeta(2.0)));

  heading("zeta_nonpositive_offset");
  show(pluss::make_series({2.0, 3.0, 2.5}).zeta(-0.5));
  show(pluss::make_series({2.0}).zeta(0.0));

  heading("beta_values_and_log");
  const auto ba = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  show(ba.beta(3.0));
  show(ba.betaln(3.0));
  show(ba.betaln(3.0).exp());

  heading("beta_reverse");
  auto br = pluss::make_series({2.0, 4.0});
  br.tracks_slope_(true);
  br.beta(3.0).sum().reverse();
  show(br.sensitivity());

  heading("inplace_integral_error");
  auto integers = pluss::make_series({0, 1});
  try {
    integers.erfinv_();
  } catch (const std::exception& error) {
    show_error(error);
  }

  return 0;
}
