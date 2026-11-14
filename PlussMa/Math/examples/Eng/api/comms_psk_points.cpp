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
  // QPSK: four points on the unit circle; the default labelling is Gray, so label 2 is at angle 3 pi / 2 and label 3 at angle pi
  const cm::Constellation qpsk = cm::psk(4);
  out.writeln("order", qpsk.order, " bits per symbol", qpsk.bits);
  out.writeln("real parts by label", flat(qpsk.points.real()));
  out.writeln("imaginary parts by label", flat(qpsk.points.imag()));
  const cm::Constellation eight = cm::psk(8);
  out.writeln("8-PSK point of label 5 (Gray)", flat(eight.points.real())[5], flat(eight.points.imag())[5]);
}
