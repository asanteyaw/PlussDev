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
  out.writeln("N0 at Eb/N0 = 0 dB for BPSK, QPSK, 16-QAM, 64-QAM");
  for (const cm::Constellation &c : {cm::psk(2), cm::psk(4), cm::qam(16), cm::qam(64)}) out.writeln("order", c.order, cm::n0(0.0, c));
  // the same N0 is a larger Eb/N0 for a constellation with more bits per symbol: each bit carries less energy
  out.writeln("N0 at Eb/N0 = 10 dB for QPSK", cm::n0(10.0, cm::psk(4)), " and 10 log10 of 1 / (2 N0)", 10.0 * std::log10(1.0 / (2.0 * cm::n0(10.0, cm::psk(4)))));
}
