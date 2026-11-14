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
  // a four-tap multipath channel shorter than the prefix becomes one complex gain per subcarrier
  const std::int64_t k = 16, cp = 4, frames = 5;
  const cm::Constellation c = cm::qam(16);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * k * frames, 3));
  const pluss::Series tx = cm::to_tones(symbols, k, cp);
  const std::vector<double> h_re{1.0, 0.0, 0.4, 0.0}, h_im{0.0, 0.3, 0.0, -0.2};
  const pluss::Series taps = pluss::series::complex(vec(h_re), vec(h_im), pluss::ScalarType::Complex128);
  const pluss::Series rx = pluss::subsp::eng::signal::conv(tx, taps).slice(-1, 0, tx.size(-1));
  const pluss::Series received = cm::from_tones(rx, k, cp).reshape({frames, k});
  // the channel's frequency response on the subcarriers
  std::vector<double> pad_re = h_re, pad_im = h_im;
  pad_re.resize(k, 0.0);
  pad_im.resize(k, 0.0);
  const pluss::Series response = pluss::series::complex(vec(pad_re), vec(pad_im), pluss::ScalarType::Complex128).fft(k, -1, "backward");
  out.writeln("evm before equalisation", cm::evm(received, symbols.reshape({frames, k})).amax().item<double>(), "(a different gain on every subcarrier)");
  const pluss::Series equalised = received.div(response);
  out.writeln("evm after dividing by the channel response", cm::evm(equalised, symbols.reshape({frames, k})).amax().item<double>());
  // a prefix shorter than the channel memory leaves intersymbol interference
  const pluss::Series short_rx = cm::from_tones(pluss::subsp::eng::signal::conv(cm::to_tones(symbols, k, 1), taps).slice(-1, 0, frames * (k + 1)), k, 1).reshape({frames, k});
  out.writeln("with a one-sample prefix, evm after dividing", cm::evm(short_rx.div(response), symbols.reshape({frames, k})).amax().item<double>());
}
