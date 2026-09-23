// CUDA analog of the MPS investigation in this repo's history: checks
// whether a scalar-broadcast add (shape [1] + shape [1,M]) is correct on
// CUDA across a size sweep crossing common GPU dispatch/threadgroup
// thresholds (4096, 8192, ...). On MPS this exact pattern silently computed
// a+a*b instead of a+b above 4096 elements, due to a buffer-pool race (a
// recycled buffer's CPU-side memcpy staging racing a still-executing GPU
// kernel writing the same physical memory) -- see MPSRuntime.mm's
// RuntimeContext::mode comment for the full story and fix. CUDA's native
// backend has no equivalent buffer pool (every allocation is a direct
// cudaMalloc/cudaFree, every transfer a blocking cudaMemcpy -- see
// CUDARuntime.cpp), so this class of bug should be structurally impossible
// here; this demo exists to confirm that empirically rather than by
// inspection alone, exactly the way the MPS bug was actually found.
#include <iomanip>
#include <iostream>
#include <vector>

#include <pluss/pluss.h>

int main() {
  std::cout << std::fixed << std::setprecision(8);
  const bool cuda_ready = pluss::cuda::is_ready();
  std::cout << (cuda_ready ? "CUDA available.\n" : "CUDA unavailable -- nothing to check.\n");
  if (!cuda_ready) {
    return 0;
  }

  const double a_val = 0.015, b_val = 1.5e-4;
  const double expected = a_val + b_val;
  bool any_mismatch = false;

  for (const std::int64_t m : {std::int64_t{500}, std::int64_t{4000}, std::int64_t{4096}, std::int64_t{4097},
                                std::int64_t{5000}, std::int64_t{8192}, std::int64_t{10000}, std::int64_t{100000},
                                std::int64_t{1000000}}) {
    const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);
    const pluss::Series a = pluss::series::full({1}, a_val, cfg);
    const pluss::Series b = pluss::series::full({1, m}, b_val, cfg);
    const pluss::Series c = (a + b).to(pluss::pCPU, pluss::ScalarType::Float64);
    const std::vector<double> host = c.to_vec<double>();

    std::int64_t first_wrong = -1;
    std::int64_t wrong_count = 0;
    double wrong_value = 0.0;
    for (std::size_t i = 0; i < host.size(); ++i) {
      if (std::abs(host[i] - expected) > 1e-9) {
        if (first_wrong < 0) {
          first_wrong = static_cast<std::int64_t>(i);
          wrong_value = host[i];
        }
        ++wrong_count;
      }
    }
    if (wrong_count > 0) any_mismatch = true;
    std::cout << "M=" << m << " first_wrong_index=" << first_wrong
              << " wrong_count=" << wrong_count << "/" << m
              << " wrong_value=" << wrong_value << " expected=" << expected
              << (wrong_count > 0 ? "  <-- MISMATCH" : "  ok") << "\n";
  }

  std::cout << (any_mismatch ? "RESULT: FAIL -- CUDA broadcast add diverged at some size.\n"
                              : "RESULT: PASS -- CUDA broadcast add correct at every size tested.\n");
  return any_mismatch ? 1 : 0;
}
