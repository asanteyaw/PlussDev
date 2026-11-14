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
  // hard decisions are +1 for a zero and -1 for a one
  const cm::ConvolutionalCode code;
  const pluss::Series bits = random_bits(2 * 200, 9).reshape({2, 200});
  const pluss::Series decoded = cm::decode(code, soft_of(cm::encode(code, bits)));
  out.writeln("decoded", decoded.sizes(), " dtype Int64", decoded.scalar_type() == pluss::ScalarType::Int64);
  out.writeln("every bit recovered", decoded.sub(bits).abs().amax().item<double>() == 0.0);
}
