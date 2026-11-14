#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // one offset per frame
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(3 * 2 * 500, 5).reshape({3, 1000}));
  const pluss::Series offsets = vec({0.001, 0.002, -0.003});
  const pluss::Series rotated = cm::derot(data, offsets.neg(), vec({0.0, 0.0, 0.0}));
  out.writeln("true offsets     ", flat(offsets));
  out.writeln("estimated offsets", flat(cm::cfo(rotated, 4)));
  out.writeln("restored by derotating with the estimates", cm::derot(rotated, cm::cfo(rotated, 4), vec({0.0, 0.0, 0.0})).sub(data).abs().amax().item<double>() < 0.02);
}
