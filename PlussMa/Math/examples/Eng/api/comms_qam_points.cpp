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
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::qam(16);
  out.writeln("order", c.order, " bits per symbol", c.bits);
  const std::vector<double> re = flat(c.points.real()), im = flat(c.points.imag());
  out.writeln("distinct in-phase levels", std::vector<double>{re[0], re[4], re[8], re[12]});
  double energy = 0.0;
  for (std::size_t k = 0; k < re.size(); ++k) energy += re[k] * re[k] + im[k] * im[k];
  out.writeln("average energy", energy / 16.0);
  out.writeln("label 0b0000 and label 0b1111", re[0], im[0], re[15], im[15]);
}
