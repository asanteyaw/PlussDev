#include <pluss/pluss.h>

#include <cassert>
#include <complex>
#include <cstdint>
#include <type_traits>

static_assert(std::is_same_v<f32_t, float>);
static_assert(std::is_same_v<f64_t, double>);
static_assert(std::is_same_v<i8_t, std::int8_t>);
static_assert(std::is_same_v<i16_t, std::int16_t>);
static_assert(std::is_same_v<i32_t, std::int32_t>);
static_assert(std::is_same_v<i64_t, std::int64_t>);
static_assert(std::is_same_v<u8_t, std::uint8_t>);
static_assert(std::is_same_v<u16_t, std::uint16_t>);
static_assert(std::is_same_v<u32_t, std::uint32_t>);
static_assert(std::is_same_v<u64_t, std::uint64_t>);

int main() {
  const pluss::Config f64_config = as_f64();
  assert(f64_config.dtype == pluss::ScalarType::Float64);
  assert(pluss::as_i64().dtype == pluss::ScalarType::Int64);
  assert(as_c128().dtype == pluss::ScalarType::Complex128);

  const pluss::Series integer = as_i64(std::int64_t{17}, pluss::Config{});
  assert(integer.dtype() == pluss::ScalarType::Int64);
  assert(pluss::as_i64(integer) == 17);
  assert(as_i64(integer) == 17);

  const pluss::Series real = pluss::as_f64(integer, pluss::Config{});
  assert(real.dtype() == pluss::ScalarType::Float64);
  assert(as_f64(real) == 17.0);

  const auto value = pluss::codem::Complex128{2.5, -4.0};
  const pluss::Series complex = as_c128(value, pluss::Config{});
  assert(complex.dtype() == pluss::ScalarType::Complex128);
  assert(as_c128(complex) == value);
}
