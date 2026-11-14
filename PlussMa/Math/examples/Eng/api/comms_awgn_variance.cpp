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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series sent = cm::symbols(c, pluss::series::zeros({50000}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64)));
  const pluss::Series noise = cm::noise(sent, 0.2, 1).sub(sent);
  out.writeln("total noise variance E|n|^2", noise.abs().mul(noise.abs()).mean().item<double>(), " requested 0.2");
  out.writeln("real part", noise.real().mul(noise.real()).mean().item<double>(), " imaginary part", noise.imag().mul(noise.imag()).mean().item<double>(), " (each half)");
  out.writeln("zero variance adds nothing", cm::noise(sent, 0.0, 1).sub(sent).abs().amax().item<double>() == 0.0);
}
