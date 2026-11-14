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
  // derotation multiplies symbol n by exp(-j (2 pi f n + phi))
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series symbols = cm::symbols(qpsk, pluss::series::make(std::vector<std::int64_t>{0, 0, 0, 0}));
  const pluss::Series f = pluss::series::make(std::vector<double>{0.05}).select(0, 0), phi = pluss::series::make(std::vector<double>{0.3}).select(0, 0);
  const pluss::Series turned = cm::derot(symbols, f, phi);
  out.writeln("angles of four identical symbols after derotating", flat(turned.imag().atan2(turned.real())));
  out.writeln("expected", -0.3, -0.3 - 2.0 * M_PI * 0.05, -0.3 - 4.0 * M_PI * 0.05, -0.3 - 6.0 * M_PI * 0.05);
  // derotating by the opposite sign restores them: derotate by (-f, -phi) rotates forward
  const pluss::Series back = cm::derot(turned, f.neg(), phi.neg());
  out.writeln("round trip difference", back.sub(symbols).abs().amax().item<double>());
}
