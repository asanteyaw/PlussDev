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
  const cm::Constellation c = cm::psk(8);
  const pluss::Series labels = pluss::series::randn({4, 100}, pluss::ScalarType::Float64, 7).abs().mul(1.0e3).floor().to(pluss::ScalarType::Int64);
  const pluss::Series sent = labels.sub(labels.div(8).floor().mul(8)).to(pluss::ScalarType::Int64);
  const pluss::Series detected = cm::detect(c, cm::symbols(c, sent));
  out.writeln("sent", sent.sizes(), " detected", detected.sizes(), " dtype is Int64", detected.scalar_type() == pluss::ScalarType::Int64);
  out.writeln("every label recovered", cm::ser(sent, detected).amax().item<double>() == 0.0);
}
