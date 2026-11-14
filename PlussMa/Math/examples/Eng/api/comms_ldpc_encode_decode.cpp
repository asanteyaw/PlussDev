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
static pluss::Series soft_of(const pluss::Series &codeword, double amplitude) {
  return pluss::series::make(std::vector<double>{1.0}).add(codeword.to(pluss::ScalarType::Float64).mul(-2.0)).mul(amplitude);
}

int main() {
  OutputEngine out;
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  const pluss::Series bits = random_bits(2 * code.dimension, 31).reshape({2, code.dimension});
  const pluss::Series word = cm::encode(code, bits);
  out.writeln("bits", bits.sizes(), " codewords", word.sizes(), " systematic: the information bits sit at the info positions", word.index_select(-1, pluss::series::make(std::vector<double>{static_cast<double>(code.info_positions[0]), static_cast<double>(code.info_positions[1])}).to(pluss::ScalarType::Int64)).sub(bits.slice(-1, 0, 2)).abs().amax().item<double>() == 0.0);
  const cm::LdpcResult decoded = cm::decode(code, soft_of(word, 5.0));
  out.writeln("noiseless: converged", decoded.converged.sizes(), "all true", decoded.converged.all().item<bool>(), " iterations", decoded.iterations.items<std::int64_t>()[0]);
  out.writeln("bits recovered", decoded.bits.sub(bits).abs().amax().item<double>() == 0.0, " codeword recovered", decoded.codeword.sub(word).abs().amax().item<double>() == 0.0);
}
