#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7));
  // a Complex64 constellation keeps modulated symbols and noisy symbols in single precision
  cm::ConstellationOptions single;
  single.dtype = pluss::ScalarType::Complex64;
  const cm::Constellation c = cm::qam(16, single);
  const pluss::Series symbols = cm::modulate(c, random_bits(40, 1));
  out.writeln("symbol dtype is Complex64", symbols.scalar_type() == pluss::ScalarType::Complex64);
  out.writeln("noisy symbol dtype is Complex64", cm::noise(symbols, 0.01, 5).scalar_type() == pluss::ScalarType::Complex64);
  out.writeln("first symbol", flat(symbols.real())[0], flat(symbols.imag())[0]);
}
