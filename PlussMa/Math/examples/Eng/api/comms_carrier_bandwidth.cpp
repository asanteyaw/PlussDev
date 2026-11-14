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
  out.print_defaults(PrintOptions{}.fixed(4));
  // a wider loop locks faster and tracks a larger offset but passes more noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 4000, 21));
  const pluss::Series received = cm::noise(cm::derot(data, pluss::series::make(std::vector<double>{-0.004}).select(0, 0), pluss::series::make(std::vector<double>{-1.0}).select(0, 0)), 0.04, 6);
  for (double bandwidth : {0.005, 0.02, 0.08}) {
    const cm::CarrierResult locked = cm::pll(received, 4, bandwidth);
    const pluss::Series settled = locked.symbols.slice(-1, 2000, 4000);
    out.writeln("loop bandwidth", bandwidth, " evm after settling", cm::evm(settled, cm::symbols(qpsk, cm::detect(qpsk, settled))).item<double>());
  }
}
