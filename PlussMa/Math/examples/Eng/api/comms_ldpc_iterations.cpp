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
  // more iterations decode more frames; the decoder stops as soon as every check holds
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  const double rate = static_cast<double>(code.dimension) / 1200.0;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(code.dimension * 6, 35).reshape({6, code.dimension});
  const double n0 = cm::n0(2.5, bpsk) / rate;
  const pluss::Series soft = cm::llr(bpsk, cm::noise(cm::modulate(bpsk, cm::encode(code, bits)), n0, 36), n0);
  out.writeln("iterations allowed  used  bit error rate  frames decoded");
  for (std::int64_t allowed : {1, 3, 10, 30, 100}) {
    const cm::LdpcResult r = cm::decode(code, soft, allowed);
    out.writeln(allowed, r.iterations.items<std::int64_t>()[0], cm::ber(bits, r.bits).mean().item<double>(), r.converged.to(pluss::ScalarType::Float64).sum().item<double>());
  }
  // the scale of the check messages: 0.75 is the usual attenuation of min-sum
  for (double scale : {0.5, 0.75, 1.0}) out.writeln("scale", scale, " bit error rate", cm::ber(bits, cm::decode(code, soft, 50, scale).bits).mean().item<double>());
}
