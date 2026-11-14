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
  out.print_defaults(PrintOptions{}.fixed(4));
  // QPSK: two bits per symbol, the first bit is the high bit of the label
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{0, 0, 0, 1, 1, 1, 1, 0});
  const pluss::Series symbols = cm::modulate(qpsk, bits);
  out.writeln("bits", labels_of(bits));
  out.writeln("symbols", symbols.sizes(), " real", flat(symbols.real()), " imag", flat(symbols.imag()));
  // 16-QAM: four bits per symbol
  out.writeln("16-QAM symbols from 8 bits", cm::modulate(cm::qam(16), pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1, 0, 0, 1, 0})).sizes());
}
