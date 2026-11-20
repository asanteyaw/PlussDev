#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  refused(out, "dtype Int64", [] { sig::hann(4, {.dtype = pluss::ScalarType::Int64}); });
  refused(out, "dtype Bool", [] { sig::hann(4, {.dtype = pluss::ScalarType::Bool}); });
  refused(out, "dtype Float32", [] { sig::hann(4, {.dtype = pluss::ScalarType::Float32}); });
  refused(out, "float64 on the MPS", [] { sig::hann(4, {.platform = pluss::Platform::MPS}); });
  refused(out, "float32 on the MPS", [] { sig::hann(4, {.dtype = pluss::ScalarType::Float32, .platform = pluss::Platform::MPS}); });
}
