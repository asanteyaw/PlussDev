#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

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
  report([&] { (void)th::mix(vec({30.0}), vec({0.012}), vec({0.0}), vec({10.0}), vec({0.006}), vec({1.0})); });   // a stream with no flow
  report([&] { (void)th::mix(vec({30.0}), vec({-0.012}), vec({1.0}), vec({10.0}), vec({0.006}), vec({1.0})); }); // negative humidity ratio
}
