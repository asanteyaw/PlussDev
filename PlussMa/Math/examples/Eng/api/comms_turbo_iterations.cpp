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
  // each iteration passes extrinsic information between the two decoders: the error rate falls with the iterations
  const cm::TurboCode code;
  const std::int64_t n = 400;
  const pluss::Series pi = cm::shuffle(n, 5);
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(n * 10, 62).reshape({10, n});
  const double rate = static_cast<double>(n) / static_cast<double>(3 * n + 6);
  const double n0 = cm::n0(1.5, bpsk) / rate;
  const pluss::Series soft = cm::llr(bpsk, cm::noise(cm::modulate(bpsk, cm::encode(code, pi, bits)), n0, 63), n0);
  out.writeln("iterations  bit error rate");
  for (std::int64_t it : {1, 2, 4, 8, 12}) out.writeln(it, cm::ber(bits, cm::decode(code, pi, soft, it)).mean().item<double>());
  // a different code: constraint length 3 with feedforward 5 and feedback 7
  cm::TurboCode small;
  small.constraint_length = 3;
  small.feedforward = 05;
  small.feedback = 07;
  const pluss::Series small_word = cm::encode(small, pi, bits);
  out.writeln("constraint length 3 codeword length", small_word.sizes(), " = 3 x 400 + 2 x 2");
}
