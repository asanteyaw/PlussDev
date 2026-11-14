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
  out.print_defaults(PrintOptions{}.fixed(6));
  // 5 OFDM blocks of 16 subcarriers carrying 16-QAM, with a cyclic prefix of 4 samples
  const cm::Constellation c = cm::qam(16);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 16 * 5, 2));
  const pluss::Series waveform = cm::to_tones(symbols, 16, 4);
  out.writeln("symbols", symbols.sizes(), " waveform", waveform.sizes(), " = 5 * (16 + 4)");
  const pluss::Series back = cm::from_tones(waveform, 16, 4);
  out.writeln("round trip evm", cm::evm(back, symbols).item<double>());
  const pluss::Series body = waveform.reshape({5, 20}).slice(-1, 4, 20);
  out.writeln("energy per sample", body.abs().mul(body.abs()).mean().item<double>(), " energy per symbol", symbols.abs().mul(symbols.abs()).mean().item<double>());
  out.writeln("the prefix repeats the end of each block", waveform.slice(-1, 0, 4).sub(waveform.slice(-1, 16, 20)).abs().amax().item<double>() < 1e-12);
}
