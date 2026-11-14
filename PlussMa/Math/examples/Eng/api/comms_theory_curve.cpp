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
  out.print_defaults(PrintOptions{}.fixed(8));
  // the whole waterfall in one call: a vector of Eb/N0 values gives a vector of probabilities
  const pluss::Series ebn0 = vec({0, 2, 4, 6, 8, 10, 12});
  out.writeln("Eb/N0 dB", flat(ebn0));
  out.writeln("BPSK  ", flat(cm::theory(cm::psk(2), ebn0)));
  out.writeln("16-QAM", flat(cm::theory(cm::qam(16), ebn0)));
  // a matrix of Eb/N0 values keeps its shape
  out.writeln("shape of a 2 x 2 input", cm::theory(cm::psk(4), vec({0, 4, 8, 12}).reshape({2, 2})).sizes());
}
