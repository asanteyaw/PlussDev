#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <chrono>
#include <cstdint>
#include <iostream>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

struct TinyBinaryClassifierImpl final : pluss::nn::Module {
  pluss::nn::Fusion head;

  explicit TinyBinaryClassifierImpl(const Config& cfg)
      : Module(true), head(2, 1, true, cfg, 31ULL) {
    module_record("head", head);
  }

  Series forward(const Series& x) const {
    return head->forward(x);
  }
};

PLUSS_MODULE(TinyBinaryClassifier);

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

Series synth_features(const Config& cfg, std::int64_t n) {
  return pluss::series::randn({n, 2}, cfg, 1337ULL);
}

Series scalar_like(const Series& like, double v) {
  return pluss::make_series(v, Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
}

Series synth_labels(const Series& x) {
  const std::int64_t n = x.sizes()[0];
  const Series x0 = x.select(1, 0).reshape({n, 1});
  const Series x1 = x.select(1, 1).reshape({n, 1});
  const Series s16 = scalar_like(x, 1.6);
  const Series s09 = scalar_like(x, 0.9);
  const Series s015 = scalar_like(x, 0.15);
  const Series zero = scalar_like(x, 0.0);
  const Series logits = x0.mul(s16).sub(x1.mul(s09)).add(s015);
  return logits.gt(zero).to(x.scalar_type());
}

struct TrainStats {
  double final_loss{0.0};
  double accuracy{0.0};
  double elapsed_seconds{0.0};
};

TrainStats train_once(pluss::Platform platform) {
  const ScalarType dtype = ScalarType::Float32;
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);

  const Series x = synth_features(cfg, 256);
  const Series y = synth_labels(x);

  TinyBinaryClassifier model(cfg);
  model->train(true);
  model->to(platform, dtype);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 0.05;
  pluss::nn::optim::Adam optimizer(model->parameters(), opt);

  const auto t0 = std::chrono::steady_clock::now();
  for (int epoch = 1; epoch <= 180; ++epoch) {
    const Series logits = model->forward(x);
    const Series loss = pluss::nn::losses::BCEWithLogits(logits, y);
    optimizer.clear();
    loss.reverse();
    optimizer.step();
  }
  const auto t1 = std::chrono::steady_clock::now();

  const Series logits = model->forward(x);
  const Series preds = logits.sigmoid().ge(0.5).to(dtype);
  const Series accuracy = preds.eq(y).to(dtype).mean();
  const std::chrono::duration<double> elapsed = t1 - t0;
  return TrainStats{
      .final_loss = as_f64(pluss::nn::losses::BCEWithLogits(logits, y)),
      .accuracy = as_f64(accuracy),
      .elapsed_seconds = elapsed.count(),
  };
}

} // namespace

int main() {
  const TrainStats cpu = train_once(pluss::pCPU);
  std::cout << "cpu dtype=f32 loss=" << cpu.final_loss << " acc=" << cpu.accuracy
            << " sec=" << cpu.elapsed_seconds << "\n";

  if (pluss::mps::is_ready()) {
    const TrainStats mps = train_once(pluss::pMPS);
    std::cout << "mps dtype=f32 loss=" << mps.final_loss << " acc=" << mps.accuracy
              << " sec=" << mps.elapsed_seconds << "\n";
    if (mps.elapsed_seconds > 0.0) {
      std::cout << "speedup_cpu_over_mps=" << (cpu.elapsed_seconds / mps.elapsed_seconds) << "\n";
    }
  } else {
    std::cout << "mps unavailable\n";
  }
  return 0;
}
