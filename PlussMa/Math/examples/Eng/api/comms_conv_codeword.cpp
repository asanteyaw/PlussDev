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
  // the textbook rate-1/2 code of constraint length 3 with generators 7 and 5 (octal)
  cm::ConvolutionalCode code;
  code.constraint_length = 3;
  code.generators = {07, 05};
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1});
  const pluss::Series word = cm::encode(code, bits);
  out.writeln("information bits", ints_of(bits));
  out.writeln("codeword (pairs, with the 2-bit zero tail)", ints_of(word), " length", word.sizes());
  out.writeln("the pairs are 11 10 00 01 01 11");
}
