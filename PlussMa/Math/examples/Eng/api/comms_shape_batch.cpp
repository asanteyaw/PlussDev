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
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.25, 4, 8);
  const pluss::Series symbols = cm::modulate(c, random_bits(2 * 4 * 20, 5).reshape({2, 80}));
  const pluss::Series waveform = cm::shape(symbols, taps, 4);
  out.writeln("symbols", symbols.sizes(), " waveform", waveform.sizes(), " (frame, sample)");
  // a real pulse train: BPSK as real symbols
  const pluss::Series real_symbols = vec({1, -1, 1, 1, -1});
  out.writeln("real symbols give a real waveform", cm::shape(real_symbols, taps, 4).dtype().is_complex() ? "complex" : "real", cm::shape(real_symbols, taps, 4).sizes());
}
