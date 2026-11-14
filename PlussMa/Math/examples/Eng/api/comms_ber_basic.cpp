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
  const pluss::Series sent = pluss::series::make(std::vector<std::int64_t>{0, 1, 1, 0, 1, 0, 0, 1});
  const pluss::Series received = pluss::series::make(std::vector<std::int64_t>{0, 1, 0, 0, 1, 1, 0, 1});
  out.writeln("two of eight bits differ", cm::ber(sent, received).item<double>());
  out.writeln("identical streams", cm::ber(sent, sent).item<double>());
  out.writeln("float and integer bits compare by value", cm::ber(vec({0, 1, 1, 0}), pluss::series::make(std::vector<std::int64_t>{0, 1, 1, 1})).item<double>());
}
