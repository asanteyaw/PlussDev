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
  out.print_defaults(PrintOptions{}.fixed(5));
  // unit-energy taps: white noise of variance N0 per waveform sample is still variance N0 per symbol after matching
  const cm::Constellation c = cm::psk(4);
  const pluss::Series bits = random_bits(2 * 20000, 3);
  const pluss::Series sent = cm::modulate(c, bits);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const double n0 = cm::n0(6.0, c);
  const pluss::Series waveform = cm::noise(cm::shape(sent, taps, 4), n0, 21);
  const pluss::Series received = cm::match(waveform, taps, 4, 20000);
  const pluss::Series noise = received.sub(sent);
  out.writeln("noise variance after matching", noise.abs().mul(noise.abs()).mean().item<double>(), " target", n0);
  out.writeln("measured bit error rate", cm::ber(bits, cm::demod(c, received)).item<double>(),
              " theory", cm::theory(c, vec({6.0})).items<double>()[0]);
}
