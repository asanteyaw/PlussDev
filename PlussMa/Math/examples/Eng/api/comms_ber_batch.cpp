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
  // one error rate per frame
  const cm::Constellation c = cm::psk(4);
  const pluss::Series bits = random_bits(3 * 2000, 2).reshape({3, 2000});
  const pluss::Series symbols = cm::modulate(c, bits);
  for (double ebn0 : {0.0, 4.0, 8.0}) {
    const pluss::Series decided = cm::demod(c, cm::noise(symbols, cm::n0(ebn0, c), 13));
    out.writeln("Eb/N0", ebn0, "dB  error rate of each frame", flat(cm::ber(bits, decided)));
  }
}
