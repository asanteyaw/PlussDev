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
  out.print_defaults(PrintOptions{}.fixed(5));
  const cm::Constellation c = cm::psk(4);
  const pluss::Series sent = cm::modulate(c, random_bits(2000, 2));
  const pluss::Series a = cm::noise(sent, 0.1, 42), b = cm::noise(sent, 0.1, 42), other = cm::noise(sent, 0.1, 43);
  out.writeln("same seed, same noise", a.sub(b).abs().amax().item<double>() == 0.0);
  out.writeln("another seed, another noise", a.sub(other).abs().amax().item<double>() > 0.0);
}
