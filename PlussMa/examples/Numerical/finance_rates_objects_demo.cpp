#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace cal = pluss::subsp::math::finance::calibration;
  namespace mdl = pluss::subsp::math::finance::models;
  namespace rate = pluss::subsp::math::finance::rates;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  rate::VasicekParams vp;
  vp.kappa = 0.75;
  vp.theta = 0.03;
  vp.sigma = 0.01;
  vp.r0 = 0.028;

  rate::SimOptions ropt;
  ropt.scheme = rate::Scheme::Transition;
  ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  ropt.randomized_quasi = true;
  ropt.sobol_scrambling = 3;
  ropt.seed = 20260651ULL;

  const auto v_paths = rate::vasicek_paths(60, 6, 1.0 / 252.0, vp, ropt, cfg, ropt.seed);
  const double v_zcb = rate::vasicek_zcb(2.0, vp.r0, vp);

  mdl::VasicekRate vmodel{vp};
  const auto v_obj_paths = vmodel.paths(60, 6, 1.0 / 252.0, ropt, cfg, ropt.seed);
  const double v_obj_zcb = vmodel.zcb(2.0, vp.r0);

  std::vector<cal::RateQuote> v_curve;
  for (double t : {0.5, 1.0, 2.0, 3.0}) {
    cal::RateQuote q;
    q.maturity = t;
    q.zcb_price = rate::vasicek_zcb(t, vp.r0, vp);
    q.weight = 1.0;
    v_curve.push_back(q);
  }
  cal::VasicekCalibOptions vcal;
  vcal.random_starts = 3;
  vcal.max_iterations = 40;
  vcal.seed = 20260652ULL;
  const auto v_fit = vmodel.calibrate_inplace(v_curve, vcal);

  rate::CirParams cp;
  cp.kappa = 1.1;
  cp.theta = 0.035;
  cp.sigma = 0.2;
  cp.r0 = 0.03;
  cp.rate_floor = 1e-12;

  ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Halton;
  ropt.seed = 20260653ULL;
  const auto c_paths = rate::cir_paths(60, 6, 1.0 / 252.0, cp, ropt, cfg, ropt.seed);
  const double c_zcb = rate::cir_zcb(2.0, cp.r0, cp);

  mdl::CirRate cmodel{cp};
  const auto c_obj_paths = cmodel.paths(60, 6, 1.0 / 252.0, ropt, cfg, ropt.seed);
  const double c_obj_zcb = cmodel.zcb(2.0, cp.r0);

  std::vector<cal::RateQuote> c_curve;
  for (double t : {0.5, 1.0, 2.0, 3.0}) {
    cal::RateQuote q;
    q.maturity = t;
    q.zcb_price = rate::cir_zcb(t, cp.r0, cp);
    q.weight = 1.0;
    c_curve.push_back(q);
  }
  cal::CirCalibOptions ccal;
  ccal.random_starts = 3;
  ccal.max_iterations = 40;
  ccal.seed = 20260654ULL;
  const auto c_fit = cmodel.calibrate_inplace(c_curve, ccal);

  std::cout << "Vasicek functional\n";
  std::cout << "zcb(2y)=" << v_zcb << "\n";
  std::cout << "paths.rate=" << v_paths.rate << "\n";

  std::cout << "\nVasicek object\n";
  std::cout << "zcb(2y)=" << v_obj_zcb << "\n";
  std::cout << "paths.rate=" << v_obj_paths.rate << "\n";
  std::cout << "calibrate.objective=" << v_fit.objective << " rmse=" << v_fit.rmse << "\n";
  std::cout << "params=" << vmodel.params().kappa << ", " << vmodel.params().theta << ", " << vmodel.params().sigma
            << ", " << vmodel.params().r0 << "\n";

  std::cout << "\nCIR functional\n";
  std::cout << "zcb(2y)=" << c_zcb << "\n";
  std::cout << "paths.rate=" << c_paths.rate << "\n";

  std::cout << "\nCIR object\n";
  std::cout << "zcb(2y)=" << c_obj_zcb << "\n";
  std::cout << "paths.rate=" << c_obj_paths.rate << "\n";
  std::cout << "calibrate.objective=" << c_fit.objective << " rmse=" << c_fit.rmse << "\n";
  std::cout << "params=" << cmodel.params().kappa << ", " << cmodel.params().theta << ", " << cmodel.params().sigma
            << ", " << cmodel.params().r0 << "\n";

  rate::HullWhiteParams hp;
  hp.kappa = 0.55;
  hp.sigma = 0.012;
  hp.r0 = 0.028;
  hp.theta0 = 0.03;
  hp.theta_slope = 0.001;

  ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  ropt.randomized_quasi = true;
  ropt.sobol_scrambling = 3;
  ropt.seed = 20260655ULL;
  const auto h_paths = rate::hull_white_paths(60, 6, 1.0 / 252.0, hp, ropt, cfg, ropt.seed);
  const double h_zcb = rate::hull_white_zcb(2.0, hp.r0, hp);

  mdl::HullWhiteRate hmodel{hp};
  const auto h_obj_paths = hmodel.paths(60, 6, 1.0 / 252.0, ropt, cfg, ropt.seed);
  const double h_obj_zcb = hmodel.zcb(2.0, hp.r0);

  std::vector<cal::RateQuote> h_curve;
  for (double t : {0.5, 1.0, 2.0, 3.0}) {
    cal::RateQuote q;
    q.maturity = t;
    q.zcb_price = rate::hull_white_zcb(t, hp.r0, hp);
    q.weight = 1.0;
    h_curve.push_back(q);
  }
  cal::HullWhiteCalibOptions hcal;
  hcal.random_starts = 2;
  hcal.max_iterations = 25;
  hcal.seed = 20260656ULL;
  const auto h_fit = hmodel.calibrate_inplace(h_curve, hcal);

  rate::HjmParams jp;
  jp.r0 = 0.029;
  jp.tenors = {0.25, 0.5, 1.0, 2.0, 3.0};
  jp.vol = {0.012, 0.011, 0.010, 0.009, 0.008};

  ropt.paths_first = true;
  ropt.seed = 20260657ULL;
  const auto j_paths = rate::hjm_paths(60, 6, 1.0 / 252.0, jp, ropt, cfg, ropt.seed);
  const double j_zcb = rate::hjm_zcb_mc(2.0, jp, 3000, 64, ropt, cfg);

  mdl::HjmRate jmodel{jp};
  const auto j_obj_paths = jmodel.paths(60, 6, 1.0 / 252.0, ropt, cfg, ropt.seed);
  const double j_obj_zcb = jmodel.zcb_mc(2.0, 3000, 64, ropt, cfg);

  std::vector<cal::RateQuote> j_curve;
  for (double t : {0.5, 1.0, 2.0}) {
    cal::RateQuote q;
    q.maturity = t;
    q.zcb_price = rate::hjm_zcb_mc(t, jp, 3000, 64, ropt, cfg);
    q.weight = 1.0;
    j_curve.push_back(q);
  }
  cal::HjmCalibOptions jcal;
  jcal.random_starts = 2;
  jcal.max_iterations = 16;
  jcal.mc_paths = 1800;
  jcal.mc_steps = 48;
  jcal.sim_options = ropt;
  jcal.seed = 20260658ULL;
  const auto j_fit = jmodel.calibrate_inplace(j_curve, jcal);

  std::cout << "\nHull-White functional\n";
  std::cout << "zcb(2y)=" << h_zcb << "\n";
  std::cout << "paths.rate=" << h_paths.rate << "\n";

  std::cout << "\nHull-White object\n";
  std::cout << "zcb(2y)=" << h_obj_zcb << "\n";
  std::cout << "paths.rate=" << h_obj_paths.rate << "\n";
  std::cout << "calibrate.objective=" << h_fit.objective << " rmse=" << h_fit.rmse << "\n";
  std::cout << "params=" << hmodel.params().kappa << ", " << hmodel.params().sigma << ", " << hmodel.params().r0
            << ", " << hmodel.params().theta0 << ", " << hmodel.params().theta_slope << "\n";

  std::cout << "\nHJM functional\n";
  std::cout << "zcb_mc(2y)=" << j_zcb << "\n";
  std::cout << "paths.rate=" << j_paths.rate << "\n";
  std::cout << "paths.forward=" << j_paths.forward << "\n";

  std::cout << "\nHJM object\n";
  std::cout << "zcb_mc(2y)=" << j_obj_zcb << "\n";
  std::cout << "paths.rate=" << j_obj_paths.rate << "\n";
  std::cout << "paths.forward=" << j_obj_paths.forward << "\n";
  std::cout << "calibrate.objective=" << j_fit.objective << " rmse=" << j_fit.rmse << "\n";
  std::cout << "params.r0=" << jmodel.params().r0 << " vol=" << pluss::make_series(jmodel.params().vol, cfg) << "\n";

  return 0;
}
