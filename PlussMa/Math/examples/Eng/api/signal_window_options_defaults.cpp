#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const sig::WindowOptions defaults;
  out.writeln("default mode is symmetric:", defaults.mode == sig::WindowMode::Symmetric);
  out.writeln("default dtype =", defaults.dtype);
  out.writeln("default platform is the CPU:", defaults.platform == pluss::Platform::CPU);
  const pluss::Series w = sig::blackman(5);
  out.writeln("a window made with no options has dtype", w.scalar_type(), "and", w.numel(), "coefficients");
}
