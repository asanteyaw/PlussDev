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
  // the default code is the rate-1/2 constraint-length-7 code with generators 171 and 133 (octal)
  const cm::ConvolutionalCode code;
  out.writeln("constraint length", code.constraint_length, " generators 0171 and 0133 octal, stored as", code.generators);
  const pluss::Series bits = random_bits(2 * 100, 3).reshape({2, 100});
  const pluss::Series word = cm::encode(code, bits);
  out.writeln("bits", bits.sizes(), " codeword", word.sizes(), " = (100 + 6) x 2 per frame");
  // a rate-1/3 code from three generators
  cm::ConvolutionalCode third;
  third.constraint_length = 3;
  third.generators = {07, 07, 05};
  out.writeln("rate 1/3 codeword length for 100 bits", cm::encode(third, random_bits(100, 1)).sizes());
}
