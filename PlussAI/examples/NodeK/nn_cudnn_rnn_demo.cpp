#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>
#include <vector>

namespace {

std::vector<pluss::ai::rnn::NetWeight> make_weights(std::int64_t gates,
                                                    std::int64_t input_size,
                                                    std::int64_t hidden_size,
                                                    std::int64_t num_layers,
                                                    bool bidirectional,
                                                    const pluss::Config& cfg,
                                                    std::uint64_t seed_base,
                                                    std::int64_t proj_size = 0) {
  const std::int64_t dirs = bidirectional ? 2 : 1;
  const bool projected_lstm = gates == 4 && proj_size > 0;
  const std::int64_t hidden_out = projected_lstm ? proj_size : hidden_size;
  std::vector<pluss::ai::rnn::NetWeight> out;
  out.reserve(static_cast<std::size_t>(num_layers * dirs));
  for (std::int64_t layer = 0; layer < num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input_size : hidden_out * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::uint64_t seed = seed_base + static_cast<std::uint64_t>(23 * (layer * dirs + dir));
      out.push_back(pluss::ai::rnn::NetWeight{
          .weight_ih = pluss::series::rand({gates * hidden_size, layer_input}, cfg, seed + 1ULL),
          .weight_hh = pluss::series::rand({gates * hidden_size, hidden_out}, cfg, seed + 2ULL),
          .bias = pluss::series::undefined(),
          .bias_ih = pluss::series::rand({gates * hidden_size}, cfg, seed + 3ULL),
          .bias_hh = pluss::series::rand({gates * hidden_size}, cfg, seed + 4ULL),
          .weight_hr = projected_lstm ? pluss::series::rand({hidden_out, hidden_size}, cfg, seed + 5ULL) : pluss::series::undefined(),
      });
    }
  }
  return out;
}

} // namespace

int main() {
  using namespace pluss;
  using namespace pluss::nn;

  std::cout << "cuDNN RNN route status\n";
  std::cout << "cuda_ready: " << (pluss::cuda::is_ready() ? "true" : "false") << '\n';
  std::cout << "cudnn_ready: " << (pluss::cuda::cudnn_ready() ? "true" : "false") << '\n';
  std::cout << "cudnn_rnn_enabled: " << (pluss::nn::cudnn_rnn_enabled() ? "true" : "false") << '\n';
  std::cout << "cudnn_rnn_strict: " << (pluss::nn::cudnn_rnn_strict() ? "true" : "false") << '\n';
  std::cout << "cudnn_rnn_route_ready: " << (pluss::nn::cudnn_rnn_ready() ? "true" : "false") << "\n\n";

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series x = series::rand({8, 4, 3}, cfg, 1101ULL);
  const std::int64_t num_layers = 2;
  const bool bidirectional = true;
  const std::int64_t dirs = bidirectional ? 2 : 1;
  const Series h0 = series::zeros({num_layers * dirs, 4, 5}, cfg);
  const Series c0 = series::zeros({num_layers * dirs, 4, 5}, cfg);

  ai::rnn::NetConfig run_cfg{};
  run_cfg.training = false;
  run_cfg.batch_first = false;
  run_cfg.bidirectional = bidirectional;
  run_cfg.dropout = 0.2;
  run_cfg.num_layers = num_layers;
  run_cfg.use_bias = true;
  run_cfg.split_bias = true;
  run_cfg.nonlinearity = "tanh";
  run_cfg.cfg = cfg;
  run_cfg.seed = 501ULL;
  ai::rnn::NetConfig train_cfg = run_cfg;
  train_cfg.training = true;
  ai::rnn::NetConfig proj_cfg = run_cfg;
  proj_cfg.proj_size = 3;
  const Series h0_proj = series::zeros({num_layers * dirs, 4, 3}, cfg);
  const Series c0_proj = series::zeros({num_layers * dirs, 4, 5}, cfg);

  const auto rnn_w = make_weights(1, 3, 5, num_layers, bidirectional, cfg, 2001ULL);
  const auto gru_w = make_weights(3, 3, 5, num_layers, bidirectional, cfg, 3001ULL);
  const auto lstm_w = make_weights(4, 3, 5, num_layers, bidirectional, cfg, 4001ULL);
  const auto lstm_proj_w = make_weights(4, 3, 5, num_layers, bidirectional, cfg, 4501ULL, 3);

  const auto [rnn_y_cpu, rnn_h_cpu] = rnn(x, h0, rnn_w, run_cfg);
  const auto [gru_y_cpu, gru_h_cpu] = gru(x, h0, gru_w, run_cfg);
  const auto [lstm_y_cpu, lstm_state_cpu] = lstm(x, std::make_tuple(h0, c0), lstm_w, run_cfg);
  const auto [lstm_proj_y_cpu, lstm_proj_state_cpu] =
      lstm(x, std::make_tuple(h0_proj, c0_proj), lstm_proj_w, proj_cfg);

  std::cout << "CPU run\n";
  std::cout << "rnn output: " << rnn_y_cpu << '\n';
  std::cout << "rnn h_n: " << rnn_h_cpu << '\n';
  std::cout << "gru output: " << gru_y_cpu << '\n';
  std::cout << "gru h_n: " << gru_h_cpu << '\n';
  std::cout << "lstm output: " << lstm_y_cpu << '\n';
  std::cout << "lstm h_n: " << std::get<0>(lstm_state_cpu) << '\n';
  std::cout << "lstm c_n: " << std::get<1>(lstm_state_cpu) << "\n\n";
  std::cout << "lstm(proj) output: " << lstm_proj_y_cpu << '\n';
  std::cout << "lstm(proj) h_n: " << std::get<0>(lstm_proj_state_cpu) << '\n';
  std::cout << "lstm(proj) c_n: " << std::get<1>(lstm_proj_state_cpu) << "\n\n";

  if (pluss::cuda::is_ready() && pluss::cuda::cudnn_ready()) {
    const auto to_cuda_weight = [](const ai::rnn::NetWeight& w) {
      return ai::rnn::NetWeight{
          .weight_ih = w.weight_ih.to(pluss::pCUDA),
          .weight_hh = w.weight_hh.to(pluss::pCUDA),
          .bias = w.bias.defined() ? w.bias.to(pluss::pCUDA) : pluss::series::undefined(),
          .bias_ih = w.bias_ih.defined() ? w.bias_ih.to(pluss::pCUDA) : pluss::series::undefined(),
          .bias_hh = w.bias_hh.defined() ? w.bias_hh.to(pluss::pCUDA) : pluss::series::undefined(),
          .weight_hr = w.weight_hr.defined() ? w.weight_hr.to(pluss::pCUDA) : pluss::series::undefined(),
      };
    };
    auto to_cuda_weights = [&](const std::vector<ai::rnn::NetWeight>& ws) {
      std::vector<ai::rnn::NetWeight> out;
      out.reserve(ws.size());
      for (const auto& w : ws) out.push_back(to_cuda_weight(w));
      return out;
    };
    const auto [rnn_y_gpu, rnn_h_gpu] = rnn(
        x.to(pluss::pCUDA),
        h0.to(pluss::pCUDA),
        to_cuda_weights(rnn_w),
        run_cfg);
    const auto [gru_y_gpu, gru_h_gpu] = gru(
        x.to(pluss::pCUDA),
        h0.to(pluss::pCUDA),
        to_cuda_weights(gru_w),
        run_cfg);
    const auto [lstm_y_gpu, lstm_state_gpu] = lstm(x.to(pluss::pCUDA),
                                                    std::make_tuple(h0.to(pluss::pCUDA), c0.to(pluss::pCUDA)),
                                                    to_cuda_weights(lstm_w),
                                                    run_cfg);
    const auto [lstm_proj_y_gpu, lstm_proj_state_gpu] = lstm(
        x.to(pluss::pCUDA),
        std::make_tuple(h0_proj.to(pluss::pCUDA), c0_proj.to(pluss::pCUDA)),
        to_cuda_weights(lstm_proj_w),
        proj_cfg);
    const auto [rnn_y_gpu_train, rnn_h_gpu_train] = rnn(
        x.to(pluss::pCUDA),
        h0.to(pluss::pCUDA),
        to_cuda_weights(rnn_w),
        train_cfg);
    const auto [gru_y_gpu_train, gru_h_gpu_train] = gru(
        x.to(pluss::pCUDA),
        h0.to(pluss::pCUDA),
        to_cuda_weights(gru_w),
        train_cfg);
    const auto [lstm_y_gpu_train, lstm_state_gpu_train] = lstm(
        x.to(pluss::pCUDA),
        std::make_tuple(h0.to(pluss::pCUDA), c0.to(pluss::pCUDA)),
        to_cuda_weights(lstm_w),
        train_cfg);

    std::cout << "CUDA run\n";
    std::cout << "rnn output: " << rnn_y_gpu.to(pluss::pCPU) << '\n';
    std::cout << "rnn h_n: " << rnn_h_gpu.to(pluss::pCPU) << '\n';
    std::cout << "gru output: " << gru_y_gpu.to(pluss::pCPU) << '\n';
    std::cout << "gru h_n: " << gru_h_gpu.to(pluss::pCPU) << '\n';
    std::cout << "lstm output: " << lstm_y_gpu.to(pluss::pCPU) << '\n';
    std::cout << "lstm h_n: " << std::get<0>(lstm_state_gpu).to(pluss::pCPU) << '\n';
    std::cout << "lstm c_n: " << std::get<1>(lstm_state_gpu).to(pluss::pCPU) << '\n';
    std::cout << "lstm(proj) output: " << lstm_proj_y_gpu.to(pluss::pCPU) << '\n';
    std::cout << "lstm(proj) h_n: " << std::get<0>(lstm_proj_state_gpu).to(pluss::pCPU) << '\n';
    std::cout << "lstm(proj) c_n: " << std::get<1>(lstm_proj_state_gpu).to(pluss::pCPU) << '\n';
    std::cout << "rnn(training=true, dropout active) output: " << rnn_y_gpu_train.to(pluss::pCPU) << '\n';
    std::cout << "rnn(training=true, dropout active) h_n: " << rnn_h_gpu_train.to(pluss::pCPU) << '\n';
    std::cout << "gru(training=true, dropout active) output: " << gru_y_gpu_train.to(pluss::pCPU) << '\n';
    std::cout << "gru(training=true, dropout active) h_n: " << gru_h_gpu_train.to(pluss::pCPU) << '\n';
    std::cout << "lstm(training=true, dropout active) output: " << lstm_y_gpu_train.to(pluss::pCPU) << '\n';
    std::cout << "lstm(training=true, dropout active) h_n: " << std::get<0>(lstm_state_gpu_train).to(pluss::pCPU) << '\n';
    std::cout << "lstm(training=true, dropout active) c_n: " << std::get<1>(lstm_state_gpu_train).to(pluss::pCPU) << '\n';
  } else {
    std::cout << "CUDA/cuDNN not ready on this host; cuDNN RNN route demonstration skipped.\n";
  }

  return 0;
}
