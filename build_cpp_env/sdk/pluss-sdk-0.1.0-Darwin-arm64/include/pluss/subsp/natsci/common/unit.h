#pragma once

#include "pluss/subsp/natsci/common/dimension.h"

#include <limits>
#include <stdexcept>
#include <string_view>

namespace pluss::subsp::natsci::common {

template <DimensionType D> class Unit final {
public:
  constexpr Unit(double si_scale, std::string_view symbol,
                 std::string_view name)
      : si_scale_(si_scale), symbol_(symbol), name_(name) {
    if (!(si_scale > 0.0) ||
        !(si_scale <= std::numeric_limits<double>::max())) {
      throw std::invalid_argument(
          "natsci::common::Unit: SI scale must be finite and positive");
    }
  }

  [[nodiscard]] constexpr double si_scale() const noexcept { return si_scale_; }
  [[nodiscard]] constexpr std::string_view symbol() const noexcept {
    return symbol_;
  }
  [[nodiscard]] constexpr std::string_view name() const noexcept {
    return name_;
  }

private:
  double si_scale_;
  std::string_view symbol_;
  std::string_view name_;
};

template <DimensionType D>
[[nodiscard]] constexpr Unit<D> scaled_unit(const Unit<D> &base, double factor,
                                            std::string_view symbol,
                                            std::string_view name) {
  return Unit<D>(base.si_scale() * factor, symbol, name);
}

namespace units {

inline constexpr Unit<Dimensionless> one{1.0, "1", "one"};

inline constexpr Unit<Length> metre{1.0, "m", "metre"};
inline constexpr Unit<Length> kilometre =
    scaled_unit(metre, 1000.0, "km", "kilometre");
inline constexpr Unit<Length> centimetre =
    scaled_unit(metre, 0.01, "cm", "centimetre");
inline constexpr Unit<Length> millimetre =
    scaled_unit(metre, 0.001, "mm", "millimetre");

inline constexpr Unit<Mass> kilogram{1.0, "kg", "kilogram"};
inline constexpr Unit<Mass> gram = scaled_unit(kilogram, 0.001, "g", "gram");

inline constexpr Unit<Time> second{1.0, "s", "second"};
inline constexpr Unit<Time> minute = scaled_unit(second, 60.0, "min", "minute");
inline constexpr Unit<Time> hour = scaled_unit(second, 3600.0, "h", "hour");

inline constexpr Unit<ElectricCurrent> ampere{1.0, "A", "ampere"};
inline constexpr Unit<Temperature> kelvin{1.0, "K", "kelvin"};
inline constexpr Unit<Amount> mole{1.0, "mol", "mole"};
inline constexpr Unit<LuminousIntensity> candela{1.0, "cd", "candela"};

inline constexpr Unit<Area> square_metre{1.0, "m^2", "square metre"};
inline constexpr Unit<Volume> cubic_metre{1.0, "m^3", "cubic metre"};
inline constexpr Unit<Frequency> hertz{1.0, "Hz", "hertz"};
inline constexpr Unit<Speed> metre_per_second{1.0, "m/s", "metre per second"};
inline constexpr Unit<Acceleration> metre_per_second_sq{
    1.0, "m/s^2", "metre per second squared"};
inline constexpr Unit<Force> newton{1.0, "N", "newton"};
// Apple's system headers expose `pascal` as a compatibility macro.
inline constexpr Unit<Pressure> pascal_unit{1.0, "Pa", "pascal"};
inline constexpr Unit<Energy> joule{1.0, "J", "joule"};
inline constexpr Unit<Power> watt{1.0, "W", "watt"};
inline constexpr Unit<ElectricCharge> coulomb{1.0, "C", "coulomb"};
inline constexpr Unit<ElectricPotential> volt{1.0, "V", "volt"};
inline constexpr Unit<Action> joule_second{1.0, "J s", "joule second"};
inline constexpr Unit<Entropy> joule_per_kelvin{1.0, "J/K", "joule per kelvin"};
inline constexpr Unit<MolarInverse> per_mole{1.0, "mol^-1", "per mole"};
inline constexpr Unit<GravitationalConstant> gravitational_si{
    1.0, "m^3 kg^-1 s^-2", "cubic metre per kilogram per second squared"};

} // namespace units
} // namespace pluss::subsp::natsci::common
