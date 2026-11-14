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
  // soft values (log-likelihood ratios) decode better than hard decisions of the same received symbols
  const cm::ConvolutionalCode code;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(20000, 15);
  const double n0 = cm::n0(3.0, bpsk) / 0.5;
  const pluss::Series received = cm::noise(cm::modulate(bpsk, cm::encode(code, bits)), n0, 16);
  const pluss::Series soft = cm::decode(code, cm::llr(bpsk, received, n0));
  const pluss::Series hard_bits = cm::demod(bpsk, received);
  const pluss::Series hard = cm::decode(code, pluss::series::make(std::vector<double>{1.0}).add(hard_bits.to(pluss::ScalarType::Float64).mul(-2.0)));
  out.writeln("bit error rate with soft decisions", cm::ber(bits, soft).item<double>(), " with hard decisions", cm::ber(bits, hard).item<double>());
}
