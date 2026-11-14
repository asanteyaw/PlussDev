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
  // shape then match: the symbols come back, up to the small interference of a truncated pulse
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(4 * 100, 8);
  const pluss::Series sent = cm::modulate(c, bits);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series received = cm::match(cm::shape(sent, taps, 8), taps, 8, 100);
  out.writeln("received symbols", received.sizes());
  out.writeln("error vector magnitude", cm::evm(received, sent).item<double>());
  out.writeln("every bit recovered", cm::ber(bits, cm::demod(c, received)).item<double>() == 0.0);
}
