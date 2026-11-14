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
  out.print_defaults(PrintOptions{}.fixed(5));
  const pluss::Series taps = cm::rrc(0.35, 4, 6);
  out.writeln("taps", taps.numel(), " (span 6 symbols x 4 samples + 1)");
  out.writeln("energy", taps.mul(taps).sum().item<double>());
  out.writeln("centre tap", flat(taps)[12], " first and last", flat(taps)[0], flat(taps)[24]);
  out.writeln("symmetric", taps.sub(taps.flip({0})).abs().amax().item<double>() < 1e-14);
}
