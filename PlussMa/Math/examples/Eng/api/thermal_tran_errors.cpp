#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  const pluss::Series time = vec(grid(10.0, 5));
  th::Net bare = net;
  bare.heats = {};
  th::Drive power;
  power.power = mat({1, 1, 1, 1, 1}, {5, 1});
  report([&] { (void)th::tran(bare, time, power); });          // a power drive with no heats
  th::Drive wrong;
  wrong.temperature = mat({20, 20, 20}, {3, 1});
  report([&] { (void)th::tran(net, time, wrong); });           // a drive with the wrong number of time samples
  th::Net open = net;
  open.stores = {};
  th::Start start;
  start.temperature = vec({1.0});
  report([&] { (void)th::tran(open, time, {}, start); });      // a start temperature with no stores
  th::Net heavy = net;
  heavy.stores.capacity = vec({-100.0});
  report([&] { (void)th::tran(heavy, time); });                // a capacity that is not positive
}
