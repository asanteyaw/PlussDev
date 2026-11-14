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

static pluss::Series soft_of(const pluss::Series &bits) { return pluss::series::make(std::vector<double>{1.0}).add(bits.to(pluss::ScalarType::Float64).mul(-2.0)); }

int main() {
  OutputEngine out;
  // the code has free distance 10: isolated bit errors in the codeword are corrected, a burst is not
  const cm::ConvolutionalCode code;
  const pluss::Series bits = random_bits(300, 12);
  const pluss::Series coded = cm::encode(code, bits).to(pluss::ScalarType::Float64);
  for (int errors : {4, 8, 12}) {
    std::vector<double> mask(static_cast<std::size_t>(coded.numel()), 0.0);
    for (int e = 0; e < errors; ++e) mask[static_cast<std::size_t>(40 + 53 * e)] = 1.0;
    const pluss::Series flipped = coded.sub(pluss::series::make(mask).mul(2.0).mul(coded.sub(0.5)));
    const pluss::Series decoded = cm::decode(code, soft_of(flipped));
    out.writeln(errors, "scattered coded-bit errors: information bit errors", cm::ber(bits, decoded).item<double>() * 300.0);
  }
  std::vector<double> burst(static_cast<std::size_t>(coded.numel()), 0.0);
  for (int e = 100; e < 112; ++e) burst[static_cast<std::size_t>(e)] = 1.0;
  const pluss::Series flipped = coded.sub(pluss::series::make(burst).mul(2.0).mul(coded.sub(0.5)));
  out.writeln("a burst of 12 adjacent coded bits: information bit errors", cm::ber(bits, cm::decode(code, soft_of(flipped))).item<double>() * 300.0);
}
