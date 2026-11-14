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
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(4 * 5000, 12);
  const double n0 = cm::n0(8.0, c);
  const pluss::Series received = cm::noise(cm::modulate(c, bits), n0, 17);
  const pluss::Series soft = cm::llr(c, received, n0);
  const pluss::Series hard = cm::demod(c, received);
  out.writeln("llr shape", soft.sizes());
  out.writeln("sign of the llr agrees with the hard decision for all but", cm::ber(hard, soft.lt(0.0).to(pluss::ScalarType::Int64)).item<double>(), "of the bits");
  // the least confident bits are the ones most likely to be wrong
  const pluss::Series wrong = hard.ne(bits).to(pluss::ScalarType::Float64);
  const pluss::Series confident = soft.abs().gt(8.0).to(pluss::ScalarType::Float64);
  out.writeln("error rate overall", wrong.mean().item<double>(), " among bits with |llr| above 8", wrong.mul(confident).sum().item<double>() / confident.sum().item<double>());
}
