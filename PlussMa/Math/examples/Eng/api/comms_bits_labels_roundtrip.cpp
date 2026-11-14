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
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1});
  const pluss::Series labels = cm::to_labels(bits, 3);
  out.writeln("labels of 3 bits", labels_of(labels));
  out.writeln("back to bits   ", labels_of(cm::to_bits(labels, 3)));
  // the first bit of each group is the most significant
  out.writeln("label of 1,0,1", labels_of(cm::to_labels(pluss::series::make(std::vector<std::int64_t>{1, 0, 1}), 3)));
  out.writeln("bits of label 6 in 3 bits", labels_of(cm::to_bits(pluss::series::make(std::vector<std::int64_t>{6}), 3)));
}
