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
static pluss::Series wave_of(const cm::Constellation &c, const pluss::Series &taps, std::uint64_t seed, std::int64_t symbols) {
  return cm::shape(cm::modulate(c, random_bits(c.bits * symbols, seed)), taps, 8);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series wave = wave_of(c, taps, 71, 400);
  // a waveform delayed by d samples gives an estimate of d (modulo a symbol, here 8 samples)
  out.writeln("   true    estimate");
  for (double d : {0.0, 1.37, 2.9, -2.4}) {
    const pluss::Series late = cm::delay(wave, pluss::series::make(std::vector<double>{d}).select(0, 0));
    out.writeln(d, cm::timing(late, 8).item<double>());
  }
}
