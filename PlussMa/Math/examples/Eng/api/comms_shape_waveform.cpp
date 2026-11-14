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
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::psk(4);
  const pluss::Series symbols = cm::modulate(c, random_bits(2 * 16, 4));
  const pluss::Series taps = cm::rrc(0.35, 8, 10);
  const pluss::Series waveform = cm::shape(symbols, taps, 8);
  out.writeln("symbols", symbols.sizes(), " taps", taps.numel(), " waveform", waveform.sizes(), " = (16 - 1) * 8 + 81");
  out.writeln("waveform is complex", waveform.dtype().is_complex());
  const double peak = waveform.abs().amax().item<double>();
  out.writeln("peak amplitude", peak);
}
