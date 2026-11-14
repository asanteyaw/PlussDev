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
  // a regular (3, 6) Gallager code of length 1200: every column has 3 ones and every row 6
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  out.writeln("length", code.length, " dimension", code.dimension, " rate", static_cast<double>(code.dimension) / static_cast<double>(code.length));
  out.writeln("parity checks", code.parity_check.sizes(), " generator", code.generator.sizes());
  out.writeln("column weights (first five)", flat(code.parity_check.to(pluss::ScalarType::Float64).sum(0).slice(0, 0, 5)), " row weights (first five)", flat(code.parity_check.to(pluss::ScalarType::Float64).sum(1).slice(0, 0, 5)));
  const pluss::Series syndrome = code.generator.to(pluss::ScalarType::Float64).matmul(code.parity_check.to(pluss::ScalarType::Float64).mT());
  out.writeln("every generator row satisfies every check", syndrome.sub(syndrome.div(2.0).floor().mul(2.0)).abs().amax().item<double>() == 0.0);
  out.writeln("same seed, same code", cm::gallager(1200, 3, 6, 1).parity_check.sub(code.parity_check).abs().amax().item<std::int64_t>() == 0,
              " different seed, different code", cm::gallager(1200, 3, 6, 2).parity_check.sub(code.parity_check).abs().amax().item<std::int64_t>() != 0);
}
