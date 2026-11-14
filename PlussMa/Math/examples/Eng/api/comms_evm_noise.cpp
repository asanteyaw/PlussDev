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
  // for unit-energy symbols in noise of variance N0 the error vector magnitude is sqrt(N0)
  const cm::Constellation c = cm::qam(16);
  const pluss::Series sent = cm::modulate(c, random_bits(4 * 20000, 7));
  for (double n0 : {0.01, 0.04, 0.16}) {
    const pluss::Series received = cm::noise(sent, n0, 3);
    out.writeln("N0", n0, " evm", cm::evm(received, sent).item<double>(), " sqrt(N0)", std::sqrt(n0));
  }
}
