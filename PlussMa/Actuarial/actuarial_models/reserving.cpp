#include "pluss/subsp/actuarial/reserving.h"

#include "pluss/subsp/actuarial/actuarial_models.h"

#include <utility>

namespace pluss::subsp::actuarial {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series factors_(const Series &age_to_age) {
  if (age_to_age.numel() == 0) {
    return Series::ones({1}, ScalarType::Float64).to(age_to_age.platform());
  }
  const Series tail = age_to_age.flip(0).cumprod(0).flip(0);
  const Series one = Series::ones({1}, ScalarType::Float64).to(tail.platform());
  return tail.unite({one}, 0);
}

struct Latest final {
  Series development = pluss::series::undefined();
  Series cumulative = pluss::series::undefined();
  Series has_observation = pluss::series::undefined();
};

Latest latest_(const Triangle &triangle) {
  const std::int64_t origins = triangle.norigins();
  const std::int64_t development = triangle.ndevelopment();
  const Series has_observation = triangle.observed().any(1, false);
  const Series index =
      Series::arange(0, development, 1, ScalarType::Int64)
          .to(triangle.values().platform(), ScalarType::Float64)
          .reshape({1, development});
  const Series latest_development = triangle.observed()
                                        .to(ScalarType::Float64)
                                        .mul(index)
                                        .amax(1, false)
                                        .to(ScalarType::Int64)
                                        .reshape({origins});
  const Series latest_cumulative =
      triangle.values()
          .gather(1, latest_development.reshape({origins, 1}))
          .reshape({origins});
  return {latest_development, latest_cumulative, has_observation};
}

Series clean_vector_(const Series &value, std::string_view name) {
  PLUSS_CHECK(value.defined(), std::string(name) + " must be defined");
  PLUSS_CHECK(value.dim() == 1, std::string(name) + " must be 1D");
  const Series out = value.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(out.isfinite().all().item<bool>(),
              std::string(name) + " must be finite");
  PLUSS_CHECK(out.ge(0.0).all().item<bool>(),
              std::string(name) + " must be >= 0");
  return out;
}

} // namespace

Triangle::Triangle(Series values, Series observed,
                   std::vector<std::string> origins,
                   std::vector<std::string> development)
    : values_(std::move(values)), observed_(std::move(observed)),
      origins_(std::move(origins)), development_(std::move(development)) {}

bool Triangle::defined() const noexcept {
  return values_.defined() && observed_.defined();
}

const Series &Triangle::values() const noexcept { return values_; }
const Series &Triangle::observed() const noexcept { return observed_; }
const std::vector<std::string> &Triangle::origins() const noexcept {
  return origins_;
}
const std::vector<std::string> &Triangle::development() const noexcept {
  return development_;
}
std::int64_t Triangle::norigins() const noexcept {
  return defined() ? values_.sizes()[0] : 0;
}
std::int64_t Triangle::ndevelopment() const noexcept {
  return defined() ? values_.sizes()[1] : 0;
}

Triangle triangle_make_(const Series &values, const Series &observed,
                        std::vector<std::string> origins,
                        std::vector<std::string> development) {
  PLUSS_CHECK(values.defined(), "triangle::cumulative: values must be defined");
  PLUSS_CHECK(values.dim() == 2, "triangle::cumulative: values must be 2D");
  PLUSS_CHECK(values.sizes()[0] > 0 && values.sizes()[1] > 0,
              "triangle::cumulative: values must be non-empty");
  const Series clean = values.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(clean.isfinite().all().item<bool>(),
              "triangle::cumulative: values must be finite");
  PLUSS_CHECK(clean.ge(0.0).all().item<bool>(),
              "triangle::cumulative: values must be >= 0");

  Series mask = observed.defined()
                    ? observed.to(clean.platform(), ScalarType::Bool)
                    : clean.gt(0.0);
  PLUSS_CHECK(mask.sizes() == clean.sizes(),
              "triangle::cumulative: observed must match values shape");
  mask = mask.contiguous();
  PLUSS_CHECK(mask.logical_or(clean.eq(0.0)).all().item<bool>(),
              "triangle::cumulative: unobserved cells must contain zero");

  const Series prefix =
      mask.to(ScalarType::Float64).cumprod(1).to(ScalarType::Bool);
  PLUSS_CHECK(prefix.eq(mask).all().item<bool>(),
              "triangle::cumulative: observed cells must be left-contiguous");

  if (clean.sizes()[1] > 1) {
    const Series current = clean.slice(1, 0, clean.sizes()[1] - 1, 1);
    const Series next = clean.slice(1, 1, clean.sizes()[1], 1);
    const Series pairs = mask.slice(1, 0, mask.sizes()[1] - 1, 1)
                             .logical_and(mask.slice(1, 1, mask.sizes()[1], 1));
    PLUSS_CHECK(
        pairs.logical_not().logical_or(next.ge(current)).all().item<bool>(),
        "triangle::cumulative: observed cumulative values must be "
        "non-decreasing");
  }

  PLUSS_CHECK(origins.empty() ||
                  origins.size() == static_cast<std::size_t>(clean.sizes()[0]),
              "triangle::cumulative: origin labels must match origin count");
  PLUSS_CHECK(
      development.empty() ||
          development.size() == static_cast<std::size_t>(clean.sizes()[1]),
      "triangle::cumulative: development labels must match development count");
  return Triangle(clean, mask, std::move(origins), std::move(development));
}

namespace triangle {
Triangle cumulative(const Series &values, const Series &observed,
                    std::vector<std::string> origins,
                    std::vector<std::string> development) {
  return triangle_make_(values, observed, std::move(origins),
                        std::move(development));
}
} // namespace triangle

namespace operators {

Reserving chain_ladder(Series supplied_cdf) {
  return Reserving("chain_ladder", [supplied_cdf = std::move(supplied_cdf)](
                                       const Triangle &triangle) {
    PLUSS_CHECK(triangle.defined(), "chain_ladder: triangle must be defined");
    const std::int64_t development = triangle.ndevelopment();
    PLUSS_CHECK(development >= 2,
                "chain_ladder: at least two development periods are required");
    const Series current = triangle.values().slice(1, 0, development - 1, 1);
    const Series next = triangle.values().slice(1, 1, development, 1);
    const Series pairs =
        triangle.observed()
            .slice(1, 0, development - 1, 1)
            .logical_and(triangle.observed().slice(1, 1, development, 1))
            .to(ScalarType::Float64);
    const Series denominator = current.mul(pairs).sum(0, false);
    PLUSS_CHECK(denominator.gt(0.0).all().item<bool>(),
                "chain_ladder: each development pair needs observed exposure");

    ReserveResult result;
    result.age_to_age = next.mul(pairs).sum(0, false).div(denominator);
    result.cdf = supplied_cdf.defined()
                     ? clean_vector_(supplied_cdf, "chain_ladder: cdf")
                           .to(triangle.values().platform())
                     : factors_(result.age_to_age);
    PLUSS_CHECK(result.cdf.numel() == development,
                "chain_ladder: cdf length must match development count");
    PLUSS_CHECK(result.cdf.ge(1.0).all().item<bool>(),
                "chain_ladder: cdf values must be >= 1");
    const Series cdf_head = result.cdf.slice(0, 0, development - 1, 1);
    const Series cdf_tail = result.cdf.slice(0, 1, development, 1);
    PLUSS_CHECK(cdf_head.ge(cdf_tail).all().item<bool>(),
                "chain_ladder: cdf must be non-increasing");
    PLUSS_CHECK(result.cdf.select(0, development - 1)
                    .sub(1.0)
                    .abs()
                    .le(1e-10)
                    .item<bool>(),
                "chain_ladder: final cdf value must equal 1");

    if (supplied_cdf.defined()) {
      // Selected factors imply the adjacent factors: f_j = cdf_j / cdf_{j+1}.
      // Reporting the factors estimated from the triangle beside a different
      // imposed pattern would describe a pattern that was not used.
      result.age_to_age = cdf_head.div(cdf_tail);
    }

    const Latest latest = latest_(triangle);
    const Series selected = result.cdf.index_select(0, latest.development);
    result.ultimate = latest.has_observation.to(ScalarType::Float64)
                          .mul(latest.cumulative.mul(selected));
    result.reserve = result.ultimate.sub(latest.cumulative).clamp_min(0.0);
    return result;
  });
}

Reserving bornhuetter_ferguson(Series expected_ultimate,
                               Series percent_reported) {
  const Series expected = clean_vector_(
      expected_ultimate, "bornhuetter_ferguson: expected_ultimate");
  const Series percent =
      clean_vector_(percent_reported, "bornhuetter_ferguson: percent_reported");
  PLUSS_CHECK(expected.numel() == percent.numel(),
              "bornhuetter_ferguson: assumptions must have the same length");
  PLUSS_CHECK(percent.le(1.0).all().item<bool>(),
              "bornhuetter_ferguson: percent_reported must be <= 1");
  return Reserving("bornhuetter_ferguson", [expected,
                                            percent](const Triangle &triangle) {
    PLUSS_CHECK(triangle.defined(),
                "bornhuetter_ferguson: triangle must be defined");
    PLUSS_CHECK(expected.numel() == triangle.norigins(),
                "bornhuetter_ferguson: assumptions must match origin count");
    const Latest latest = latest_(triangle);
    ReserveResult result;
    result.reserve =
        expected.to(triangle.values().platform())
            .mul(percent.to(triangle.values().platform()).mul(-1.0).add(1.0));
    result.ultimate = latest.cumulative.add(result.reserve);
    return result;
  });
}

} // namespace operators
} // namespace pluss::subsp::actuarial
