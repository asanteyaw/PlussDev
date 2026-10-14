#pragma once

// NASA seven-coefficient ideal-gas thermodynamic polynomials. This is the
// neutral numerical owner used by both physics and chemistry; domain wrappers
// add units and model semantics without rebuilding the polynomial evaluator.

#include "pluss/codem/codem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common::thermo {

struct SpeciesData final {
  std::string name;
  std::vector<std::pair<std::string, double>> composition;
  double t_min{0.0};
  double t_mid{0.0};
  double t_max{0.0};
  std::array<double, 7> low{};
  std::array<double, 7> high{};
};

class Nasa7 final {
public:
  struct Properties final {
    Series cp_r = pluss::series::undefined();
    Series h_rt = pluss::series::undefined();
    Series s_r = pluss::series::undefined();

    [[nodiscard]] Series g_rt() const { return h_rt.sub(s_r); }
  };

  Nasa7(const std::vector<SpeciesData> &data, const Series &like) {
    require(like.defined(), "model exemplar must be defined");
    require(like.dtype().is_floating(),
            "model exemplar must have a real floating dtype");
    require(!data.empty(), "need at least one species");

    dtype_ = like.scalar_type();
    platform_ = like.platform();
    const auto count = static_cast<std::int64_t>(data.size());
    std::vector<double> low;
    std::vector<double> high;
    std::vector<double> mids;
    low.reserve(data.size() * 7);
    high.reserve(data.size() * 7);
    mids.reserve(data.size());
    names_.reserve(data.size());

    std::unordered_set<std::string> unique_names;
    t_lo_ = -std::numeric_limits<double>::infinity();
    t_hi_ = std::numeric_limits<double>::infinity();
    for (const auto &species : data) {
      require(!species.name.empty(), "species names must not be empty");
      require(unique_names.insert(species.name).second,
              "species names must be unique: " + species.name);
      require(std::isfinite(species.t_min) && std::isfinite(species.t_mid) &&
                  std::isfinite(species.t_max) && species.t_min > 0.0 &&
                  species.t_min < species.t_mid &&
                  species.t_mid < species.t_max,
              "temperature ranges must be finite, positive, and increasing "
              "for " +
                  species.name);
      require(all_finite(species.low) && all_finite(species.high),
              "polynomial coefficients must be finite for " + species.name);

      names_.push_back(species.name);
      low.insert(low.end(), species.low.begin(), species.low.end());
      high.insert(high.end(), species.high.begin(), species.high.end());
      mids.push_back(species.t_mid);
      t_lo_ = std::max(t_lo_, species.t_min);
      t_hi_ = std::min(t_hi_, species.t_max);
    }
    require(t_lo_ <= t_hi_,
            "species temperature ranges must have a common interval");

    low_ = on_model(pluss::series::make(low, {count, 7}));
    high_ = on_model(pluss::series::make(high, {count, 7}));
    mids_ = on_model(pluss::series::make(mids, {count}));
    exponents_ = on_model(pluss::series::make({1.0, 2.0, 3.0, 4.0}));
    h_weights_ = on_model(
        pluss::series::make({1.0 / 2.0, 1.0 / 3.0, 1.0 / 4.0, 1.0 / 5.0}));
    s_weights_ =
        on_model(pluss::series::make({1.0, 1.0 / 2.0, 1.0 / 3.0, 1.0 / 4.0}));
  }

  [[nodiscard]] std::int64_t species() const noexcept { return low_.size(0); }
  [[nodiscard]] double t_min() const noexcept { return t_lo_; }
  [[nodiscard]] double t_max() const noexcept { return t_hi_; }
  [[nodiscard]] const std::vector<std::string> &names() const noexcept {
    return names_;
  }

  void check(const Series &temperature) const {
    validate(temperature);
    const Series in_range =
        temperature.ge(t_lo_).logical_and(temperature.le(t_hi_));
    require(in_range.all().item<bool>(),
            "temperature must be inside the common NASA validity range [" +
                std::to_string(t_lo_) + ", " + std::to_string(t_hi_) + "] K");
  }

  // Evaluation permits polynomial extrapolation, matching Cantera's NASA
  // update path. Call check() first when declared validity must be enforced.
  [[nodiscard]] Properties eval(const Series &temperature) const {
    const auto data = prep(temperature);
    return {cp(data), enthalpy(data), entropy(data)};
  }

  [[nodiscard]] Series cp_r(const Series &temperature) const {
    return cp(prep(temperature));
  }
  [[nodiscard]] Series h_rt(const Series &temperature) const {
    return enthalpy(prep(temperature));
  }
  [[nodiscard]] Series s_r(const Series &temperature) const {
    return entropy(prep(temperature));
  }
  [[nodiscard]] Series g_rt(const Series &temperature) const {
    const auto data = prep(temperature);
    return enthalpy(data).sub(entropy(data));
  }

private:
  struct EvalData final {
    Series temperature;
    Series temp;
    Series coeff;
    Series terms;
    Series a0;
  };

  [[nodiscard]] EvalData prep(const Series &temperature) const {
    validate(temperature);
    const Series temp = temperature.unsqueeze(-1);
    const Series coeff = select(temperature);
    const Series powers = temp.pow(exponents_);
    const Series terms = coeff.slice(-1, 1, 5, 1).mul(powers.unsqueeze(-2));
    return {temperature, temp, coeff, terms, coeff.select(-1, 0)};
  }

  [[nodiscard]] Series cp(const EvalData &data) const {
    return data.a0.add(data.terms.sum(-1, false));
  }

  [[nodiscard]] Series enthalpy(const EvalData &data) const {
    return data.a0.add(data.terms.mul(h_weights_).sum(-1, false))
        .add(data.coeff.select(-1, 5).div(data.temp));
  }

  [[nodiscard]] Series entropy(const EvalData &data) const {
    return data.a0.mul(data.temperature.log().unsqueeze(-1))
        .add(data.coeff.select(-1, 6))
        .add(data.terms.mul(s_weights_).sum(-1, false));
  }

  void validate(const Series &temperature) const {
    require(temperature.defined(), "temperature must be defined");
    require(temperature.dtype().is_floating(),
            "temperature must have a real floating dtype");
    require(temperature.scalar_type() == dtype_,
            "temperature dtype must match the model dtype");
    require(temperature.platform() == platform_,
            "temperature platform must match the model platform");
    require(temperature.numel() > 0, "temperature must not be empty");

    const Series valid =
        temperature.isfinite().logical_and(temperature.gt(0.0));
    require(valid.all().item<bool>(),
            "temperature must be finite and strictly positive");
  }
  static void require(bool condition, const std::string &message) {
    if (!condition)
      throw std::invalid_argument("natsci::thermo: " + message);
  }

  static bool all_finite(const std::array<double, 7> &values) {
    return std::all_of(values.begin(), values.end(),
                       [](double value) { return std::isfinite(value); });
  }

  [[nodiscard]] Series on_model(Series value) const {
    return value.to(dtype_).to(platform_);
  }

  [[nodiscard]] Series select(const Series &temperature) const {
    // Cantera's NasaPoly2 uses the low interval at exactly the midpoint.
    const Series low = temperature.unsqueeze(-1).le(mids_).unsqueeze(-1);
    return low.where(low_, high_);
  }

  Series low_ = pluss::series::undefined();
  Series high_ = pluss::series::undefined();
  Series mids_ = pluss::series::undefined();
  Series exponents_ = pluss::series::undefined();
  Series h_weights_ = pluss::series::undefined();
  Series s_weights_ = pluss::series::undefined();
  std::vector<std::string> names_;
  ScalarType dtype_{ScalarType::Float32};
  Platform platform_{Platform::CPU};
  double t_lo_{0.0};
  double t_hi_{0.0};
};

} // namespace pluss::subsp::natsci::common::thermo
