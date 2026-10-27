#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
namespace reg = pluss::subsp::stats::regression;
namespace hyp = pluss::subsp::stats::inference::hypothesis;

Series vec(const std::vector<double>& v, const pluss::Config& cfg) {
  return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}, cfg);
}

Series mat(const std::vector<double>& v, std::int64_t r, std::int64_t c, const pluss::Config& cfg) {
  return pluss::make_series(v, {r, c}, cfg);
}

void print_vec(const char* key, const Series& x) {
  const auto v = x.to(pluss::pCPU, ScalarType::Float64).host<double>();
  std::cout << key << '=';
  for (std::size_t i = 0; i < v.size(); ++i) {
    if (i) {
      std::cout << ',';
    }
    std::cout << v[i];
  }
  std::cout << '\n';
}

} // namespace

int main() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series x_ols = mat(
      {
          1.0, 2.0,
          2.0, 1.0,
          3.0, 0.0,
          4.0, 2.0,
          5.0, 1.0,
          6.0, 3.0,
          7.0, 4.0,
          8.0, 5.0,
      },
      8,
      2,
      cfg);
  const Series y_ols = vec({0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9}, cfg);
  const auto fit_ols = reg::ols_fit(y_ols, x_ols, {}, cfg);
  print_vec("OLS_COEF", fit_ols.coefficients);

  std::vector<double> xv;
  std::vector<double> yv;
  for (std::int64_t i = -6; i <= 6; ++i) {
    const double x = static_cast<double>(i) / 3.0;
    xv.push_back(x);
    yv.push_back(1.0 / (1.0 + std::exp(-(-0.7 + 1.8 * x))));
  }
  reg::GLMOptions gopt;
  gopt.family = reg::GLMFamily::Binomial;
  gopt.link = reg::GLMLink::Logit;
  const auto fit_glm = reg::glm_fit(vec(yv, cfg), mat(xv, static_cast<std::int64_t>(xv.size()), 1, cfg), gopt, cfg);
  print_vec("GLM_LOGIT_COEF", fit_glm.coefficients);

  const Series y_anova = vec({1.0, 1.2, 0.8, 3.1, 2.9, 3.0, 6.0, 6.2, 5.9}, cfg);
  const Series g_anova = vec({0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, cfg);
  const auto a1 = hyp::anova_1w(y_anova, g_anova, {}, cfg);
  std::cout << "ANOVA_1W_STAT=" << a1.stat << '\n';
  std::cout << "ANOVA_1W_P=" << a1.pvalue << '\n';

  const Series obs = vec({18.0, 21.0, 16.0, 25.0}, cfg);
  const Series exp = vec({20.0, 20.0, 20.0, 20.0}, cfg);
  const auto c2g = hyp::chisq_gof(obs, exp, {}, cfg);
  std::cout << "CHISQ_GOF_STAT=" << c2g.stat << '\n';
  std::cout << "CHISQ_GOF_P=" << c2g.pvalue << '\n';

  const Series table = mat(
      {
          90.0, 60.0, 104.0,
          30.0, 50.0, 51.0,
      },
      2,
      3,
      cfg);
  const auto c2i = hyp::chisq_indep(table, 0.05, cfg);
  std::cout << "CHISQ_INDEP_STAT=" << c2i.stat << '\n';
  std::cout << "CHISQ_INDEP_P=" << c2i.pvalue << '\n';

  const Series fx = vec({-4.0, -1.0, 2.0, 5.0, 8.0, 11.0}, cfg);
  const Series fy = vec({0.9, 1.0, 1.1, 0.95, 1.05, 1.0}, cfg);
  hyp::TestOptions topt;
  topt.tail = hyp::Tail::Right;
  const auto fr = hyp::f_ratio(fx, fy, topt, cfg);
  std::cout << "F_RATIO_STAT=" << fr.stat << '\n';
  std::cout << "F_RATIO_P=" << fr.pvalue << '\n';

  return 0;
}

