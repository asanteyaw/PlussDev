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
  // QPSK has the same bit error probability as BPSK at the same Eb/N0; 8-PSK needs more energy per bit
  const pluss::Series ebn0 = vec({6.0, 10.0});
  out.print_defaults(PrintOptions{}.fixed(8));
  out.writeln("BPSK ", flat(cm::theory(cm::psk(2), ebn0)));
  out.writeln("QPSK ", flat(cm::theory(cm::psk(4), ebn0)));
  out.writeln("8-PSK", flat(cm::theory(cm::psk(8), ebn0)));
  out.writeln("16-PSK", flat(cm::theory(cm::psk(16), ebn0)));
}
