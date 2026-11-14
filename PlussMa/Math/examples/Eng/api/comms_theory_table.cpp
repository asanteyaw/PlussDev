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
  out.print_defaults(PrintOptions{}.fixed(6));
  struct Row { cm::Constellation c; const char *name; double ebn0; };
  const std::vector<Row> rows = {{cm::psk(2), "BPSK  ", 4.0}, {cm::psk(4), "QPSK  ", 6.0}, {cm::qam(16), "16-QAM", 8.0}, {cm::qam(64), "64-QAM", 12.0}};
  out.writeln("             Eb/N0   simulated    theory");
  std::uint64_t seed = 1;
  for (const Row &row : rows) {
    const pluss::Series bits = random_bits(row.c.bits * 100000, seed++);
    const pluss::Series received = cm::noise(cm::modulate(row.c, bits), cm::n0(row.ebn0, row.c), seed++);
    out.writeln(row.name, row.ebn0, cm::ber(bits, cm::demod(row.c, received)).item<double>(), cm::theory(row.c, vec({row.ebn0})).items<double>()[0]);
  }
}
