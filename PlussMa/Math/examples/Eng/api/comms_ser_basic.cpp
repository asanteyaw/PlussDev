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
  const cm::Constellation c = cm::qam(16);
  const pluss::Series labels = pluss::series::randn({20000}, pluss::ScalarType::Float64, 4).abs().mul(1.0e3).floor();
  const pluss::Series sent = labels.sub(labels.div(16).floor().mul(16)).to(pluss::ScalarType::Int64);
  const pluss::Series received = cm::noise(cm::symbols(c, sent), 0.05, 5);
  const pluss::Series bits_sent = cm::to_bits(sent, 4), bits_decided = cm::demod(c, received);
  const double ser = cm::ser(sent, cm::detect(c, received)).item<double>();
  out.writeln("symbol error rate", ser, " bit error rate", cm::ber(bits_sent, bits_decided).item<double>());
  out.writeln("with Gray labels a symbol error is mostly one bit: ber is about ser / 4", ser / 4.0);
}
