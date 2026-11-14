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
  // a leading axis is a batch of independent frames
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(3 * 80, 2).reshape({3, 80});
  const pluss::Series symbols = cm::modulate(c, bits);
  out.writeln("bits", bits.sizes(), " symbols", symbols.sizes(), " (frame, symbol)");
  out.writeln("round trip recovers every bit", cm::ber(bits, cm::demod(c, symbols)).amax().item<double>() == 0.0);
}
