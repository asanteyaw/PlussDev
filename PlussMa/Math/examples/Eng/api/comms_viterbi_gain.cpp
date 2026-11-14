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
  // BPSK with the rate-1/2 code against uncoded BPSK at the same Eb/N0 (each coded bit carries half the energy per bit)
  const cm::ConvolutionalCode code;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(20000, 5);
  const pluss::Series sent = cm::encode(code, bits);
  out.writeln("Eb/N0   uncoded     coded");
  for (double ebn0 : {2.0, 3.0, 4.0}) {
    const double n0_coded = cm::n0(ebn0, bpsk) / 0.5;
    const pluss::Series received = cm::noise(cm::modulate(bpsk, sent), n0_coded, 7);
    const pluss::Series decoded = cm::decode(code, cm::llr(bpsk, received, n0_coded));
    const pluss::Series raw = cm::noise(cm::modulate(bpsk, bits), cm::n0(ebn0, bpsk), 8);
    out.writeln(ebn0, cm::ber(bits, cm::demod(bpsk, raw)).item<double>(), cm::ber(bits, decoded).item<double>());
  }
}
