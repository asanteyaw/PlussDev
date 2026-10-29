#pragma once

#include <concepts>

namespace pluss::subsp::natsci::common {

// SI base-dimension exponents in the order L, M, T, I, Theta, N, J.
template <int Length, int Mass, int Time, int Current, int Temperature,
          int Amount, int LuminousIntensity>
struct Dimension {
  static constexpr int length = Length;
  static constexpr int mass = Mass;
  static constexpr int time = Time;
  static constexpr int current = Current;
  static constexpr int temperature = Temperature;
  static constexpr int amount = Amount;
  static constexpr int luminous_intensity = LuminousIntensity;
};

template <class T>
concept DimensionType = requires {
  { T::length } -> std::convertible_to<int>;
  { T::mass } -> std::convertible_to<int>;
  { T::time } -> std::convertible_to<int>;
  { T::current } -> std::convertible_to<int>;
  { T::temperature } -> std::convertible_to<int>;
  { T::amount } -> std::convertible_to<int>;
  { T::luminous_intensity } -> std::convertible_to<int>;
};

template <DimensionType Lhs, DimensionType Rhs>
using DimensionProduct =
    Dimension<Lhs::length + Rhs::length, Lhs::mass + Rhs::mass,
              Lhs::time + Rhs::time, Lhs::current + Rhs::current,
              Lhs::temperature + Rhs::temperature, Lhs::amount + Rhs::amount,
              Lhs::luminous_intensity + Rhs::luminous_intensity>;

template <DimensionType Lhs, DimensionType Rhs>
using DimensionQuotient =
    Dimension<Lhs::length - Rhs::length, Lhs::mass - Rhs::mass,
              Lhs::time - Rhs::time, Lhs::current - Rhs::current,
              Lhs::temperature - Rhs::temperature, Lhs::amount - Rhs::amount,
              Lhs::luminous_intensity - Rhs::luminous_intensity>;

template <DimensionType D, int Power>
using DimensionPower =
    Dimension<D::length * Power, D::mass * Power, D::time * Power,
              D::current * Power, D::temperature * Power, D::amount * Power,
              D::luminous_intensity * Power>;

using Dimensionless = Dimension<0, 0, 0, 0, 0, 0, 0>;
using Length = Dimension<1, 0, 0, 0, 0, 0, 0>;
using Mass = Dimension<0, 1, 0, 0, 0, 0, 0>;
using Time = Dimension<0, 0, 1, 0, 0, 0, 0>;
using ElectricCurrent = Dimension<0, 0, 0, 1, 0, 0, 0>;
using Temperature = Dimension<0, 0, 0, 0, 1, 0, 0>;
using Amount = Dimension<0, 0, 0, 0, 0, 1, 0>;
using LuminousIntensity = Dimension<0, 0, 0, 0, 0, 0, 1>;

using Area = DimensionPower<Length, 2>;
using Volume = DimensionPower<Length, 3>;
using Frequency = DimensionQuotient<Dimensionless, Time>;
using Speed = DimensionQuotient<Length, Time>;
using Acceleration = DimensionQuotient<Speed, Time>;
using Force = DimensionProduct<Mass, Acceleration>;
using Pressure = DimensionQuotient<Force, Area>;
using Energy = DimensionProduct<Force, Length>;
using Power = DimensionQuotient<Energy, Time>;
using Momentum = DimensionProduct<Mass, Speed>;
using AngularMomentum = DimensionProduct<Length, Momentum>;
using ElectricCharge = DimensionProduct<ElectricCurrent, Time>;
using ElectricPotential = DimensionQuotient<Energy, ElectricCharge>;
using Action = DimensionProduct<Energy, Time>;
using Entropy = DimensionQuotient<Energy, Temperature>;
using MolarInverse = DimensionPower<Amount, -1>;
using MolarGasConstant = DimensionQuotient<Entropy, Amount>;
using GravitationalConstant = Dimension<3, -1, -2, 0, 0, 0, 0>;

} // namespace pluss::subsp::natsci::common
