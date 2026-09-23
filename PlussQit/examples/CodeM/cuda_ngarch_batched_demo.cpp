// CUDA analog of the batched NGARCH-recursion benchmark from this repo's
// MPS investigation. Same shape of workload as before: T=10000 strictly
// sequential timesteps (h_t depends on h_{t-1}, unavoidable), each step
// operating on a wide [C, M] cross-section x path tensor instead of a
// handful of scalars. On MPS this was the test that surfaced the buffer-
// pool race (see cuda_add_threshold_demo.cpp's header comment). Reports
// CPU-vs-CUDA timing AND checks the two platforms agree numerically
// (byte-identical input data, shared across platforms, so any divergence
// is a genuine computation/synchronization bug, not just different RNG
// streams).
#include <chrono>
#include <iomanip>
#include <iostream>

#include <pluss/pluss.h>

namespace {

double run_one_pass(pluss::Platform platform, std::int64_t C, std::int64_t M, std::int64_t seq_len,
                     const pluss::Series& z3_cpu, std::ostream& out) {
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(platform);

  const pluss::Series omega = pluss::series::full({1}, 1.2376e-04, cfg);
  const pluss::Series alpha = pluss::series::full({1}, 0.015, cfg);
  const pluss::Series beta  = pluss::series::full({1}, 0.90, cfg);
  const pluss::Series gamma = pluss::series::full({1}, -1.2, cfg);

  const pluss::Series z3 = z3_cpu.to(platform, pluss::ScalarType::Float32);
  pluss::Series ht = pluss::series::full({C, M}, 1.5e-4, cfg);

  const auto start = std::chrono::steady_clock::now();

  pluss::Series y = pluss::series::empty({C, M, seq_len}, cfg);
  {
    const pluss::Series z0 = z3.select(2, 0);
    const pluss::Series y0 = -0.5 * ht + ht.sqrt() * z0;
    y.select(2, 0).copy_(y0);
  }
  for (std::int64_t t = 1; t < seq_len; ++t) {
    const pluss::Series z_prev = z3.select(2, t - 1);
    ht = omega + alpha * ht * (z_prev + gamma).pow(2) + beta * ht;
    const pluss::Series z_t = z3.select(2, t);
    const pluss::Series yt = -0.5 * ht + ht.sqrt() * z_t;
    y.select(2, t).copy_(yt);
  }
  const double checksum = y.sum().to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
  const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

  out << "  platform=" << (platform == pluss::pCUDA ? "CUDA" : "CPU")
      << " C=" << C << " M=" << M << " (width=" << (C * M) << ") T=" << seq_len
      << " checksum=" << checksum
      << " seconds=" << elapsed << "\n";
  return elapsed;
}

} // namespace

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const bool cuda_ready = pluss::cuda::is_ready();
  std::cout << (cuda_ready ? "CUDA available.\n" : "CUDA unavailable -- nothing to check.\n");
  if (!cuda_ready) {
    return 0;
  }

  constexpr std::int64_t seq_len = 10000;

  for (const auto [c, m] : {std::pair<std::int64_t, std::int64_t>{8, 1}, {50, 200}}) {
    std::cout << "== width=" << (c * m) << " ==\n";
    const pluss::Series z3_cpu = pluss::series::randn(
        {c, m, seq_len}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU),
        /*seed=*/7ULL);
    const double cpu_seconds = run_one_pass(pluss::pCPU, c, m, seq_len, z3_cpu, std::cout);
    const double cuda_seconds = run_one_pass(pluss::pCUDA, c, m, seq_len, z3_cpu, std::cout);
    std::cout << "  CPU: " << cpu_seconds << "s   CUDA: " << cuda_seconds << "s   ";
    if (cuda_seconds < cpu_seconds) {
      std::cout << "CUDA " << (cpu_seconds / cuda_seconds) << "x FASTER\n";
    } else {
      std::cout << "CUDA " << (cuda_seconds / cpu_seconds) << "x SLOWER\n";
    }
  }

  return 0;
}
