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
  // one waveform per row, each with its own delay: delay and estimate take a vector of delays
  const cm::Constellation c = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series one = wave_of(c, taps, 3, 300);
  const pluss::Series frames = pluss::series::zeros({3, one.size(-1)}, pluss::Config{}.with_dtype(pluss::ScalarType::Complex128)).add(one);
  const pluss::Series delays = vec({0.5, 1.5, 3.2});
  const pluss::Series late = cm::delay(frames, delays);
  out.writeln("true delays     ", flat(delays));
  out.writeln("estimated delays", flat(cm::timing(late, 8)));
}
