#include "pluss/subsp/stats/regression.h"

#include "CodeM/core/Errors.h"
#include "../predict_input.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::regression {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64_cfg_like_(const Series& like, Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(like.platform()).with_pinned(cfg.pinned);
}

Series as_matrix_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": expected defined design matrix");
  const auto s = x.sizes();
  PLUSS_CHECK(!s.empty(), std::string(op) + ": expected non-scalar design matrix");
  if (s.size() == 1) {
    return x.reshape({s[0], 1});
  }
  PLUSS_CHECK(s.size() == 2, std::string(op) + ": expected design matrix to be 1D or 2D");
  if (s[0] == 1 && s[1] > 1) {
    return x.reshape({s[1], 1});
  }
  return x;
}

std::vector<std::string> base_names_(std::int64_t p, const std::vector<std::string>& names, const char* op) {
  if (!names.empty()) {
    PLUSS_CHECK(static_cast<std::int64_t>(names.size()) == p,
                std::string(op) + ": predictor_names size must match input columns");
    return names;
  }
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(p));
  for (std::int64_t j = 0; j < p; ++j) {
    out.push_back("x" + std::to_string(j + 1));
  }
  return out;
}

std::string term_name_(const std::vector<std::int64_t>& exp, const std::vector<std::string>& base) {
  std::ostringstream oss;
  bool first = true;
  for (std::size_t j = 0; j < exp.size(); ++j) {
    const std::int64_t e = exp[j];
    if (e <= 0) {
      continue;
    }
    if (!first) {
      oss << '*';
    }
    first = false;
    oss << base[j];
    if (e > 1) {
      oss << '^' << e;
    }
  }
  return oss.str();
}

void emit_cross_terms_(std::int64_t pos,
                       std::int64_t remaining,
                       std::vector<std::int64_t>& cur,
                       const std::vector<Series>& xcols,
                       const std::vector<std::string>& base_names,
                       std::vector<Series>& out_cols,
                       std::vector<std::string>& out_names) {
  const std::int64_t p = static_cast<std::int64_t>(cur.size());
  if (pos == p - 1) {
    cur[static_cast<std::size_t>(pos)] = remaining;
    std::int64_t active = 0;
    for (std::int64_t e : cur) {
      if (e > 0) {
        ++active;
      }
    }
    if (active < 2) {
      return;
    }
    const std::int64_t n = xcols.front().sizes()[0];
    const Config cfg = f64_cfg_like_(xcols.front(), {});
    Series term = pluss::series::ones({n, 1}, cfg);
    for (std::int64_t j = 0; j < p; ++j) {
      const std::int64_t e = cur[static_cast<std::size_t>(j)];
      if (e > 0) {
        term = term.mul(xcols[static_cast<std::size_t>(j)].pow(static_cast<double>(e)));
      }
    }
    out_cols.push_back(term);
    out_names.push_back(term_name_(cur, base_names));
    return;
  }

  for (std::int64_t e = 0; e <= remaining; ++e) {
    cur[static_cast<std::size_t>(pos)] = e;
    emit_cross_terms_(pos + 1, remaining - e, cur, xcols, base_names, out_cols, out_names);
  }
}

struct PolyBasis {
  Series design = pluss::series::undefined();
  std::vector<std::string> names{};
  std::vector<std::string> base_names{};
};

PolyBasis build_poly_basis_(const Series& x,
                            std::int64_t degree,
                            bool include_cross,
                            const std::vector<std::string>& predictor_names,
                            Config cfg,
                            const char* op,
                            bool prediction = false) {
  PLUSS_CHECK(degree >= 1, std::string(op) + ": degree must be >= 1");
  const auto out_cfg = f64_cfg_like_(x, cfg);
  const auto predictors = static_cast<std::int64_t>(predictor_names.size());
  const Series xm = (prediction ? detail::prediction_matrix(x, predictors, predictors, op) : as_matrix_(x, op)).to(out_cfg.platform, out_cfg.dtype).contiguous();
  const std::int64_t n = xm.sizes()[0];
  const std::int64_t p = xm.sizes()[1];
  PLUSS_CHECK(n > 0 && p > 0, std::string(op) + ": empty design matrix");

  const auto base = base_names_(p, predictor_names, op);
  std::vector<Series> xcols;
  xcols.reserve(static_cast<std::size_t>(p));
  for (std::int64_t j = 0; j < p; ++j) {
    xcols.push_back(xm.select(1, j).reshape({n, 1}));
  }

  std::vector<Series> cols;
  std::vector<std::string> names;
  cols.reserve(static_cast<std::size_t>(p * degree));
  names.reserve(static_cast<std::size_t>(p * degree));

  for (std::int64_t j = 0; j < p; ++j) {
    for (std::int64_t d = 1; d <= degree; ++d) {
      cols.push_back(xcols[static_cast<std::size_t>(j)].pow(static_cast<double>(d)));
      if (d == 1) {
        names.push_back(base[static_cast<std::size_t>(j)]);
      } else {
        names.push_back(base[static_cast<std::size_t>(j)] + "^" + std::to_string(d));
      }
    }
  }

  if (include_cross && degree >= 2) {
    std::vector<std::int64_t> cur(static_cast<std::size_t>(p), 0);
    for (std::int64_t total = 2; total <= degree; ++total) {
      emit_cross_terms_(0, total, cur, xcols, base, cols, names);
    }
  }

  PLUSS_CHECK(!cols.empty(), std::string(op) + ": polynomial basis is empty");
  Series basis = cols.front();
  if (cols.size() > 1) {
    basis = basis.unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
  }
  return PolyBasis{
      .design = basis,
      .names = names,
      .base_names = base,
  };
}

} // namespace

Series poly_expand(const Series& x, std::int64_t degree, bool include_cross, Config cfg) {
  const auto basis = build_poly_basis_(
      x, degree, include_cross, {}, cfg, "subsp::stats::regression::poly_expand");
  return basis.design;
}

PolyFitResult poly_fit(const Series& y, const Series& x, const PolyOptions& options, Config cfg) {
  const char* op = "subsp::stats::regression::poly_fit";
  const auto basis = build_poly_basis_(x, options.degree, options.include_cross, options.predictor_names, cfg, op);

  OLSOptions ols_opt;
  ols_opt.alpha = options.alpha;
  ols_opt.add_intercept = options.add_intercept;
  ols_opt.drop_missing = options.drop_missing;
  ols_opt.rcond = options.rcond;
  ols_opt.weights = options.weights;
  ols_opt.response_name = options.response_name;
  ols_opt.predictor_names = basis.names;

  PolyFitResult out;
  out.ols = ols_fit(y, basis.design, ols_opt, cfg);
  out.degree = options.degree;
  out.include_cross = options.include_cross;
  out.base_predictor_names = basis.base_names;
  return out;
}

Series poly_predict(const PolyFitResult& fit, const Series& x, Config cfg) {
  const char* op = "subsp::stats::regression::poly_predict";
  PLUSS_CHECK(fit.ols.coefficients.defined(), std::string(op) + ": fit.ols.coefficients is undefined");
  PLUSS_CHECK(fit.degree >= 1, std::string(op) + ": fit.degree must be >= 1");
  const std::int64_t fitted_columns = static_cast<std::int64_t>(fit.base_predictor_names.size());
  PLUSS_CHECK(detail::prediction_matrix(x, fitted_columns, fitted_columns, op).sizes()[1] == fitted_columns,
              std::string(op) + ": expected X with " + std::to_string(fitted_columns) + " columns, the predictors the fit used");

  const auto basis = build_poly_basis_(x, fit.degree, fit.include_cross, fit.base_predictor_names, cfg, op, /*prediction=*/true);
  PLUSS_CHECK(basis.design.sizes()[1] == fit.ols.num_predictors,
              std::string(op) + ": expanded predictor count mismatch");
  return ols_predict(fit.ols, basis.design, cfg);
}

std::string poly_summary(const PolyFitResult& fit) {
  std::ostringstream oss;
  print_poly_summary(oss, fit);
  return oss.str();
}

std::ostream& print_poly_summary(std::ostream& os, const PolyFitResult& fit) {
  PLUSS_CHECK(fit.ols.coefficients.defined(), "subsp::stats::regression::print_poly_summary: fit.ols.coefficients is undefined");
  os << "Polynomial Regression (degree=" << fit.degree
     << ", cross=" << (fit.include_cross ? "true" : "false") << ")\n";
  return print_ols_summary(os, fit.ols);
}

} // namespace pluss::subsp::stats::regression

