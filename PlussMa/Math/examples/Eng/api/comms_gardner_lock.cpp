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
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series symbols = cm::modulate(qpsk, random_bits(2 * 800, 91));
  // three unknown timing offsets: the loop finds the sampling instants by itself
  out.writeln("true delay  recovered symbols  best evm over the stream alignment");
  for (double delay : {0.0, 1.7, -1.1}) {
    const cm::TimingResult got = cm::gardner(chain(symbols, taps, delay), 4, 0.02);
    double best = 1e9;
    for (int lag = 0; lag < 30; ++lag) best = std::min(best, cm::evm(got.symbols.slice(-1, 200 + lag, 700 + lag), symbols.slice(-1, 200, 700)).item<double>());
    out.writeln(delay, got.symbols.sizes(), best);
  }
  // the sampling instants advance by one symbol (4 samples) per symbol once the loop has settled
  const cm::TimingResult one = cm::gardner(chain(symbols, taps, 1.7), 4, 0.02);
  const std::vector<double> instants = flat(one.offset);
  out.writeln("sampling instants of symbols 600 to 604", instants[600], instants[601], instants[602], instants[603], instants[604]);
  out.writeln("offsets shape", one.offset.sizes(), " symbols shape", one.symbols.sizes());
}
