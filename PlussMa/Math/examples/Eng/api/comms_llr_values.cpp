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
  out.print_defaults(PrintOptions{}.fixed(3));
  // BPSK: label 0 is +1 and label 1 is -1 for the Gray-labelled constellation; the log-likelihood ratio is 4 r / N0
  const cm::Constellation bpsk = cm::psk(2);
  const std::vector<double> points = flat(bpsk.points.real());
  out.writeln("point of label 0 and label 1", points[0], points[1]);
  const pluss::Series received = pluss::series::complex(vec({0.9, 0.1, -0.1, -0.9}), vec({0.0, 0.0, 0.0, 0.0}), pluss::ScalarType::Complex128);
  out.writeln("llr at N0 = 0.5", flat(cm::llr(bpsk, received, 0.5)));
  out.writeln("4 r / N0       ", 4 * 0.9 / 0.5, 4 * 0.1 / 0.5, -4 * 0.1 / 0.5, -4 * 0.9 / 0.5);
  out.writeln("positive favours bit 0, and a larger N0 shrinks the confidence", flat(cm::llr(bpsk, received, 2.0)));
}
