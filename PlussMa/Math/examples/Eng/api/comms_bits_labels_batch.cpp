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
  const pluss::Series bits = random_bits(2 * 12, 4).reshape({2, 12});
  const pluss::Series labels = cm::to_labels(bits, 4);
  out.writeln("bits", bits.sizes(), " labels", labels.sizes(), " bits again", cm::to_bits(labels, 4).sizes());
  out.writeln("round trip exact", cm::to_bits(labels, 4).sub(bits).abs().amax().item<double>() == 0.0);
  // floating zeros and ones are accepted as well
  out.writeln("float bits", labels_of(cm::to_labels(vec({1, 1, 0, 1}), 2)));
}
