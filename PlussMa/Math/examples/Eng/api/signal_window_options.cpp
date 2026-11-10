#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series symmetric = sig::hann(6);
  const pluss::Series periodic = sig::hann(
      6, {.mode = sig::WindowMode::Periodic,
          .dtype = pluss::ScalarType::Float32});

  out.writeln("symmetric =", symmetric.items<double>());
  out.writeln("periodic =", periodic.items<float>());
  out.writeln("periodic dtype =", periodic.scalar_type());
  out.writeln("empty size =", sig::hann(0).numel());
  out.writeln("singleton =", sig::hann(1).items<double>());
}
