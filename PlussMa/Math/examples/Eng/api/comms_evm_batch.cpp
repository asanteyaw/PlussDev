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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series sent = cm::modulate(c, random_bits(3 * 2 * 2000, 5).reshape({3, 4000}));
  const pluss::Series received = cm::noise(sent, 0.09, 8);
  out.writeln("evm of each frame", flat(cm::evm(received, sent)), " expected about 0.3");
  out.writeln("a gain error of 10 percent shows as an evm of", cm::evm(sent.mul(1.1), sent).amax().item<double>());
  out.writeln("noise-free", cm::evm(sent, sent).amax().item<double>());
}
