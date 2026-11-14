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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series bits = random_bits(3 * 400, 6).reshape({3, 400});
  const pluss::Series received = cm::noise(cm::modulate(c, bits), 0.05, 8);
  const pluss::Series decided = cm::demod(c, received);
  out.writeln("bits", bits.sizes(), " decisions", decided.sizes(), " dtype is Int64", decided.scalar_type() == pluss::ScalarType::Int64);
  out.writeln("errors in each frame below 5", cm::ber(bits, decided).mul(400.0).items<double>());
}
