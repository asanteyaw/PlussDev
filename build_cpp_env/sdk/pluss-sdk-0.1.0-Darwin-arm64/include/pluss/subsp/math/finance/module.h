#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/objects.h"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::subsp::math::finance::module {

enum class Engine : std::uint8_t {
  Fft = 0,
  Cos = 1,
  MonteCarlo = 2,
  Analytic = 3,
};

enum class Risk : std::uint8_t {
  PriceOnly = 0,
  All = 1,
};

enum class Loss : std::uint8_t {
  MSE = 0,
  RelativeMSE = 1,
  MAE = 2,
  Huber = 3,
};

struct Controls {
  Engine engine = Engine::Cos;
  Risk risk = Risk::All;
  Loss loss = Loss::MSE;
  double huber_delta = 0.05;
  pricer::Fft fft{};
  pricer::COS cos{};
  pricer::MonteCarlo mc{};
  pricer::Analytic analytic{};
};

struct Paths {
  pluss::codem::Series asset = pluss::series::undefined();
  pluss::codem::Series vol = pluss::series::undefined();
  pluss::codem::Series variance = pluss::series::undefined();
  pluss::codem::Series asset_shocks = pluss::series::undefined();
  pluss::codem::Series vol_shocks = pluss::series::undefined();
};

using ModelVariant = std::variant<model::Heston,
                                  model::Bates,
                                  model::BlackScholes,
                                  model::Sabr,
                                  model::RoughHeston,
                                  model::RoughBergomi>;

template <typename T>
struct is_supported_model : std::false_type {};
template <>
struct is_supported_model<model::Heston> : std::true_type {};
template <>
struct is_supported_model<model::Bates> : std::true_type {};
template <>
struct is_supported_model<model::BlackScholes> : std::true_type {};
template <>
struct is_supported_model<model::Sabr> : std::true_type {};
template <>
struct is_supported_model<model::RoughHeston> : std::true_type {};
template <>
struct is_supported_model<model::RoughBergomi> : std::true_type {};

template <typename T>
inline constexpr bool is_supported_model_v = is_supported_model<std::remove_cvref_t<T>>::value;

template <typename T>
concept SupportedModel = is_supported_model_v<T>;

struct Model {
  ModelVariant value = model::Heston{};

  Model() = default;
  Model(const ModelVariant& v) : value(v) {}
  Model(ModelVariant&& v) : value(std::move(v)) {}

  template <SupportedModel M>
  Model(const M& m) : value(m) {}

  template <SupportedModel M>
  Model(M&& m) : value(std::forward<M>(m)) {}

  Model& operator=(const ModelVariant& v) {
    value = v;
    return *this;
  }

  Model& operator=(ModelVariant&& v) {
    value = std::move(v);
    return *this;
  }

  template <SupportedModel M>
  Model& operator=(const M& m) {
    value = m;
    return *this;
  }

  template <SupportedModel M>
  Model& operator=(M&& m) {
    value = std::forward<M>(m);
    return *this;
  }

  CODEM_API pluss::codem::Series parameters(pluss::codem::Config cfg = {}) const;
  CODEM_API void parameters(const pluss::codem::Series& params, pluss::codem::Config cfg = {});
  CODEM_API void parameters(std::span<const double> params, pluss::codem::Config cfg = {});

  template <std::size_t N>
  void parameters(const std::array<double, N>& params, pluss::codem::Config cfg = {}) {
    parameters(std::span<const double>(params.data(), params.size()), cfg);
  }

  void parameters(const std::vector<double>& params, pluss::codem::Config cfg = {}) {
    parameters(std::span<const double>(params.data(), params.size()), cfg);
  }

  template <SupportedModel M>
  M& as() {
    return std::get<M>(value);
  }

  template <SupportedModel M>
  const M& as() const {
    return std::get<M>(value);
  }

  template <typename F>
  decltype(auto) visit(F&& f) {
    return std::visit(std::forward<F>(f), value);
  }

  template <typename F>
  decltype(auto) visit(F&& f) const {
    return std::visit(std::forward<F>(f), value);
  }
};

struct Field {
  instrument::Vanilla instrument{};
  market::State market{};
  Model model{};
  Controls controls{};
  Paths paths{};

  CODEM_API Paths simulate_paths(pluss::codem::Config cfg = {}) const;
  CODEM_API Paths simulate_paths(const Model& model_candidate, pluss::codem::Config cfg = {}) const;
  CODEM_API void simulate(pluss::codem::Config cfg = {});
  CODEM_API void simulate(const Model& model_candidate, pluss::codem::Config cfg = {});
  CODEM_API void clear_paths();

  CODEM_API std::string model_name() const;
  CODEM_API std::string engine_name() const;
  CODEM_API void validate() const;
  CODEM_API pricer::Result price(pluss::codem::Config cfg = {}) const;
  CODEM_API pricer::Result price(const Model& model_candidate, pluss::codem::Config cfg = {}) const;
  CODEM_API pluss::codem::Series residuals(const pluss::codem::Series& target_prices,
                                           pluss::codem::Config cfg = {}) const;
  CODEM_API pluss::codem::Series loss(const pluss::codem::Series& target_prices,
                                      pluss::codem::Config cfg = {}) const;
  CODEM_API double loss_value(const pluss::codem::Series& target_prices,
                              pluss::codem::Config cfg = {}) const;
  CODEM_API double objective(const pluss::codem::Series& target_prices,
                             pluss::codem::Config cfg = {}) const;
  CODEM_API std::function<double(const Model&)> bind_objective(const pluss::codem::Series& target_prices,
                                                               pluss::codem::Config cfg = {}) const;

  template <SupportedModel M>
  pricer::Result price(const M& m, pluss::codem::Config cfg = {}) const {
    return price(Model{m}, cfg);
  }

  template <SupportedModel M>
  std::function<double(const M&)> bind_objective_typed(const pluss::codem::Series& target_prices,
                                                       pluss::codem::Config cfg = {}) const {
    Field base = *this;
    return [base, target_prices, cfg](const M& candidate) mutable -> double {
      base.model = candidate;
      return base.loss_value(target_prices, cfg);
    };
  }
};

} // namespace pluss::subsp::math::finance::module
