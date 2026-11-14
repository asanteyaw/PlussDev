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
  // the (13, 15) turbo code of constraint length 4: systematic bits, two parity streams and the tail of the first encoder
  const cm::TurboCode code;
  const std::int64_t n = 12;
  const pluss::Series pi = cm::shuffle(n, 5);
  const pluss::Series bits = random_bits(n, 3);
  const pluss::Series word = cm::encode(code, pi, bits);
  out.writeln("interleaver", ints_of(pi));
  out.writeln("information bits", ints_of(bits));
  out.writeln("codeword length", word.sizes(), " = 3 x 12 + 2 x 3");
  out.writeln("systematic  ", ints_of(word.slice(-1, 0, n)));
  out.writeln("parity 1    ", ints_of(word.slice(-1, n, 2 * n)));
  out.writeln("parity 2    ", ints_of(word.slice(-1, 2 * n, 3 * n)));
  out.writeln("tail (systematic then parity)", ints_of(word.slice(-1, 3 * n, 3 * n + 6)));
  out.writeln("same seed, same interleaver", cm::shuffle(n, 5).sub(pi).abs().amax().item<std::int64_t>() == 0, " another seed", ints_of(cm::shuffle(n, 6)));
}
