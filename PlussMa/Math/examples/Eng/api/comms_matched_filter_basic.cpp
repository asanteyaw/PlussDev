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
  out.print_defaults(PrintOptions{}.fixed(5));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 10);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 50, 4));
  const pluss::Series wave = cm::shape(symbols, taps, 8);
  // the matched filter is a convolution with the reversed taps: its length is the waveform plus the taps minus one
  const pluss::Series filtered = cm::matched(wave, taps);
  out.writeln("waveform", wave.sizes(), " taps", taps.numel(), " filtered", filtered.sizes());
  // sampling it at the combined delay (taps - 1) and every symbol period is what match does
  const std::int64_t delay = taps.numel() - 1;
  out.writeln("sampled filtered output equals match", filtered.select(-1, delay).sub(cm::match(wave, taps, 8, 50).select(-1, 0)).abs().item<double>() < 1e-12);
  out.writeln("peak of the filtered pulse of the first symbol is at the delay", flat(filtered.abs().slice(-1, delay - 2, delay + 3)));
  // the filtered waveform of a batch keeps its frames
  const pluss::Series frames = pluss::series::zeros({2, wave.size(-1)}, pluss::Config{}.with_dtype(pluss::ScalarType::Complex128)).add(wave);
  out.writeln("batched filtered shape", cm::matched(frames, taps).sizes());
}
