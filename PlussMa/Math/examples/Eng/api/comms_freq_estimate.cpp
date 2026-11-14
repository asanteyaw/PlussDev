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
  out.print_defaults(PrintOptions{}.fixed(6));
  // QPSK symbols with a carrier offset of 0.0031 cycles per symbol and a phase of 0.4 rad, with a little noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 3000, 81));
  const pluss::Series zero = pluss::series::make(std::vector<double>{0.0}).select(0, 0);
  const pluss::Series rotated = cm::derot(data, pluss::series::make(std::vector<double>{-0.0031}).select(0, 0), pluss::series::make(std::vector<double>{-0.4}).select(0, 0));
  for (double n0 : {0.0, 0.05, 0.2}) {
    const pluss::Series received = n0 > 0.0 ? cm::noise(rotated, n0, 3) : rotated;
    out.writeln("noise variance", n0, " estimated offset", cm::cfo(received, 4).item<double>(), " true 0.003100");
  }
  // BPSK and 8-PSK use the matching power
  out.writeln("BPSK estimate", cm::cfo(cm::derot(cm::modulate(cm::psk(2), random_bits(3000, 4)), pluss::series::make(std::vector<double>{-0.01}).select(0, 0), zero), 2).item<double>());
}
