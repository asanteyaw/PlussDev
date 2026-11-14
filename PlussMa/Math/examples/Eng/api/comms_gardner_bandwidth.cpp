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
static pluss::Series chain(const pluss::Series &symbols, const pluss::Series &taps, double delay_samples) {
  return cm::matched(cm::delay(cm::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{delay_samples}).select(0, 0)), taps);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // a wider loop settles faster and tracks a larger timing drift but passes more noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series symbols = cm::modulate(qpsk, random_bits(2 * 1000, 7));
  const pluss::Series noisy = cm::noise(chain(symbols, taps, 1.3), 0.02, 9);
  out.writeln("loop bandwidth  best evm over the stream alignment after settling");
  for (double bandwidth : {0.005, 0.01, 0.03, 0.1}) {
    const cm::TimingResult got = cm::gardner(noisy, 4, bandwidth);
    double best = 1e9;
    for (int lag = 0; lag < 40; ++lag) best = std::min(best, cm::evm(got.symbols.slice(-1, 400 + lag, 900 + lag), symbols.slice(-1, 400, 900)).item<double>());
    out.writeln(bandwidth, best);
  }
}
