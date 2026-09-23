// Pure libtorch (no pluss at all) CPU-vs-CUDA timing comparison for the
// same NGARCH-recursion workload as cuda_ngarch_batched_demo.cpp: a long,
// strictly sequential per-timestep elementwise recursion with autograd
// through the whole chain. Exists to answer the same question the MPS
// investigation in this repo's history answered for Apple's backend: is
// "long sequential recursion is slow on GPU" a pluss-specific inefficiency,
// or an inherent property of this workload shape? On MPS, torch showed the
// identical pattern (even more pronounced than pluss's own number), which
// is what actually proved it wasn't pluss's fault. Run this and
// cuda_ngarch_batched_demo.cpp side by side on the same hardware.
#include <chrono>
#include <iomanip>
#include <iostream>

#include <torch/torch.h>

namespace {

double run_one_pass(torch::Device device, int64_t batch, int64_t seq_len, std::ostream& out) {
  auto opts = torch::TensorOptions().dtype(torch::kFloat32).device(device);

  torch::Tensor y;
  {
    torch::Tensor tt = torch::arange(seq_len, opts).remainder(17.0) - 8.0;
    torch::Tensor bb = torch::arange(batch, opts).remainder(5.0);
    y = 0.01 * tt.unsqueeze(0) + 0.002 * bb.unsqueeze(1);
  }

  torch::Tensor omega = torch::full({batch}, 1.2376e-04, opts).set_requires_grad(true);
  torch::Tensor alpha = torch::full({batch}, 0.015, opts).set_requires_grad(true);
  torch::Tensor beta  = torch::full({batch}, 0.90, opts).set_requires_grad(true);
  torch::Tensor lamda = torch::full({batch}, 0.05459, opts).set_requires_grad(true);
  torch::Tensor gamma = torch::full({batch}, -1.2, opts).set_requires_grad(true);

  const auto start = std::chrono::steady_clock::now();

  const double r = 0.019, d = 0.012;
  torch::Tensor phi = beta + alpha * (1 + gamma.square());
  torch::Tensor h_t = omega / (1.0 - phi);
  torch::Tensor z_t = (y.select(1, 0) - r + d - lamda * h_t.sqrt() + 0.5 * h_t) / h_t.sqrt();

  std::vector<torch::Tensor> hs{h_t}, zs{z_t};
  for (int64_t t = 1; t < seq_len; ++t) {
    h_t = omega + alpha * h_t * (z_t + gamma).pow(2) + beta * h_t;
    z_t = (y.select(1, t) - r + d - lamda * h_t.sqrt() + 0.5 * h_t) / h_t.sqrt();
    hs.push_back(h_t);
    zs.push_back(z_t);
  }
  torch::Tensor h = torch::stack(hs, 1);
  torch::Tensor z = torch::stack(zs, 1);

  torch::Tensor x = z * h.sqrt();
  torch::Tensor stddev = h.sqrt();
  torch::Tensor logpdf = -0.5 * (x / stddev).square() - stddev.log() - 0.5 * std::log(2.0 * M_PI);
  torch::Tensor loss = -logpdf.sum() / static_cast<double>(batch);

  loss.backward();
  const double loss_value = loss.to(torch::kCPU).item<double>();
  const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

  out << "  device=" << device
      << " batch=" << batch << " T=" << seq_len
      << " nll=" << loss_value
      << " forward+backward_seconds=" << elapsed << "\n";
  return elapsed;
}

} // namespace

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const bool cuda_ready = torch::cuda::is_available();
  std::cout << (cuda_ready ? "torch CUDA available.\n" : "torch CUDA unavailable -- nothing to check.\n");
  if (!cuda_ready) {
    return 0;
  }

  constexpr int64_t batch = 8;
  constexpr int64_t seq_len = 10000;

  std::cout << "-- CPU --\n";
  const double cpu_seconds = run_one_pass(torch::kCPU, batch, seq_len, std::cout);

  std::cout << "-- CUDA --\n";
  const double cuda_seconds = run_one_pass(torch::Device(torch::kCUDA), batch, seq_len, std::cout);

  std::cout << "\nCPU: " << cpu_seconds << "s   CUDA: " << cuda_seconds << "s\n";
  if (cuda_seconds < cpu_seconds) {
    std::cout << "torch CUDA is " << (cpu_seconds / cuda_seconds) << "x FASTER than CPU for this workload shape.\n";
  } else {
    std::cout << "torch CUDA is " << (cuda_seconds / cpu_seconds) << "x SLOWER than CPU for this workload shape.\n";
  }

  return 0;
}
