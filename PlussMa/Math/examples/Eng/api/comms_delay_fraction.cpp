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
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series wave = wave_of(cm::qam(16), taps, 4, 200);
  // a delay is undone by the opposite delay; the shift is circular, so the waveform is exactly recovered
  const pluss::Series there = cm::delay(wave, pluss::series::make(std::vector<double>{3.4}).select(0, 0));
  out.writeln("delay by 3.4 and back, largest difference", cm::delay(there, pluss::series::make(std::vector<double>{-3.4}).select(0, 0)).sub(wave).abs().amax().item<double>());
  // a whole-sample delay is an exact circular shift
  const pluss::Series shifted = cm::delay(wave, pluss::series::make(std::vector<double>{5.0}).select(0, 0));
  out.writeln("delay by 5 samples equals a circular shift", shifted.slice(-1, 5, 20).sub(wave.slice(-1, 0, 15)).abs().amax().item<double>() < 1e-10);
  out.writeln("delay of a real signal stays real", !cm::delay(vec({0, 1, 0, 0, 0, 0, 0, 0}), pluss::series::make(std::vector<double>{2.0}).select(0, 0)).dtype().is_complex());
}
