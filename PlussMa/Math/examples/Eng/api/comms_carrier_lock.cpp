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
  // QPSK with a carrier frequency offset of 0.003 cycles per symbol and a phase of 0.4 rad, 20 dB signal-to-noise ratio
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 3000, 81));
  const pluss::Series received = cm::noise(cm::derot(data, pluss::series::make(std::vector<double>{-0.003}).select(0, 0), pluss::series::make(std::vector<double>{-0.4}).select(0, 0)), 0.01, 5);
  const cm::CarrierResult locked = cm::pll(received, 4, 0.02);
  out.writeln("symbols", locked.symbols.sizes(), " phase", locked.phase.sizes());
  const std::vector<double> phase = flat(locked.phase);
  out.writeln("tracked phase at symbols 0, 500, 1000, 2000, 2999", phase[0], phase[500], phase[1000], phase[2000], phase[2999]);
  out.writeln("true phase at the same symbols", 0.4, 0.4 + 2 * M_PI * 0.003 * 500, 0.4 + 2 * M_PI * 0.003 * 1000, 0.4 + 2 * M_PI * 0.003 * 2000, 0.4 + 2 * M_PI * 0.003 * 2999);
  // once locked the symbols sit on the constellation up to a multiple of 90 degrees; resolve it against the first symbols
  const pluss::Series tail = locked.symbols.slice(-1, 1500, 3000);
  out.writeln("evm of the locked symbols against the nearest points", cm::evm(tail, cm::symbols(qpsk, cm::detect(qpsk, tail))).item<double>());
}
