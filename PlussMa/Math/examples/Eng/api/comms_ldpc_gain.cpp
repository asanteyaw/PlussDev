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
  // BPSK with the rate-1/2 (3, 6) code against uncoded BPSK at the same Eb/N0
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  const double rate = static_cast<double>(code.dimension) / 1200.0;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(code.dimension * 6, 32).reshape({6, code.dimension});
  const pluss::Series sent = cm::modulate(bpsk, cm::encode(code, bits));
  out.writeln("Eb/N0   uncoded     coded     frames decoded");
  for (double ebn0 : {1.5, 2.0, 2.5, 3.0}) {
    const double n0 = cm::n0(ebn0, bpsk) / rate;
    const cm::LdpcResult decoded = cm::decode(code, cm::llr(bpsk, cm::noise(sent, n0, 33), n0), 60);
    const pluss::Series raw = cm::noise(cm::modulate(bpsk, bits), cm::n0(ebn0, bpsk), 34);
    out.writeln(ebn0, cm::ber(bits, cm::demod(bpsk, raw)).mean().item<double>(), cm::ber(bits, decoded.bits).mean().item<double>(),
                decoded.converged.to(pluss::ScalarType::Float64).sum().item<double>(), "of 6");
  }
}
