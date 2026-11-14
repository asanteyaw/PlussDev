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
  // BPSK with the rate-1/3 turbo code against uncoded BPSK at the same Eb/N0
  const cm::TurboCode code;
  const std::int64_t n = 400;
  const pluss::Series pi = cm::shuffle(n, 5);
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(n * 10, 62).reshape({10, n});
  const pluss::Series sent = cm::modulate(bpsk, cm::encode(code, pi, bits));
  const double rate = static_cast<double>(n) / static_cast<double>(3 * n + 6);
  out.writeln("Eb/N0   uncoded     coded (8 iterations)");
  for (double ebn0 : {0.5, 1.0, 1.5, 2.0}) {
    const double n0 = cm::n0(ebn0, bpsk) / rate;
    const pluss::Series decoded = cm::decode(code, pi, cm::llr(bpsk, cm::noise(sent, n0, 63), n0), 8);
    const pluss::Series raw = cm::noise(cm::modulate(bpsk, bits), cm::n0(ebn0, bpsk), 64);
    out.writeln(ebn0, cm::ber(bits, cm::demod(bpsk, raw)).mean().item<double>(), cm::ber(bits, decoded).mean().item<double>());
  }
}
