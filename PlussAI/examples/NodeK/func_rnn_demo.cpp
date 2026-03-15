#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace {

pluss::Series make_batch(const pluss::Config& cfg) {
  std::vector<double> values;
  values.reserve(4 * 8 * 2);
  for (std::int64_t batch = 0; batch < 4; ++batch) {
    for (std::int64_t step = 0; step < 8; ++step) {
      const double t = 0.4 * static_cast<double>(step) + 0.2 * static_cast<double>(batch);
      values.push_back(std::sin(t));
      values.push_back(std::cos(t));
    }
  }
  return pluss::make_series(values, {4, 8, 2}, cfg);
}

pluss::ai::rnn::NetWeight make_rnn_weight(std::int64_t gates,
                                          std::int64_t input_size,
                                          std::int64_t hidden_size,
                                          const pluss::Config& cfg,
                                          double scale) {
  std::vector<double> ih(static_cast<std::size_t>(gates * hidden_size * input_size));
  std::vector<double> hh(static_cast<std::size_t>(gates * hidden_size * hidden_size));
  std::vector<double> bi(static_cast<std::size_t>(gates * hidden_size));
  std::vector<double> bh(static_cast<std::size_t>(gates * hidden_size));
  for (std::size_t i = 0; i < ih.size(); ++i) ih[i] = scale * std::sin(0.13 * static_cast<double>(i + 1));
  for (std::size_t i = 0; i < hh.size(); ++i) hh[i] = scale * std::cos(0.11 * static_cast<double>(i + 1));
  for (std::size_t i = 0; i < bi.size(); ++i) bi[i] = scale * 0.1 * std::sin(0.17 * static_cast<double>(i + 1));
  for (std::size_t i = 0; i < bh.size(); ++i) bh[i] = scale * 0.1 * std::cos(0.19 * static_cast<double>(i + 1));
  return pluss::ai::rnn::NetWeight{
      .weight_ih = pluss::make_series(ih, {gates * hidden_size, input_size}, cfg),
      .weight_hh = pluss::make_series(hh, {gates * hidden_size, hidden_size}, cfg),
      .bias_ih = pluss::make_series(bi, {gates * hidden_size}, cfg),
      .bias_hh = pluss::make_series(bh, {gates * hidden_size}, cfg),
  };
}

} // namespace

int main() {
  using namespace pluss;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series batch = make_batch(f64);
  const Series hx = zeros({1, 4, 3}, f64);
  const Series cx = zeros({1, 4, 3}, f64);

  ai::rnn::NetConfig cfg;
  cfg.batch_first = true;
  cfg.num_layers = 1;
  cfg.split_bias = true;
  cfg.cfg = f64;

  const auto rnn_params = make_rnn_weight(1, 2, 3, f64, 0.35);
  cfg.nonlinearity = "relu";
  const auto [rnn_out, rnn_hn] = rnn(batch, hx, std::vector<ai::rnn::NetWeight>{rnn_params}, cfg);

  const auto gru_params = make_rnn_weight(3, 2, 3, f64, 0.25);
  cfg.nonlinearity = "tanh";
  const auto [gru_out, gru_hn] = gru(batch, hx, std::vector<ai::rnn::NetWeight>{gru_params}, cfg);

  const auto lstm_params = make_rnn_weight(4, 2, 3, f64, 0.2);
  const auto [lstm_out, lstm_state] = lstm(batch, hx, cx, std::vector<ai::rnn::NetWeight>{lstm_params}, cfg);

  const Series ff_weight = make_series(std::vector<double>{
                                           0.5, -0.2, 0.3,
                                           -0.1, 0.4, 0.2},
                                       {2, 3},
                                       f64);
  const Series ff_bias = make_series(std::vector<double>{0.05, -0.03}, {2}, f64);
  const Series head = pluss::fusion(rnn_out, ff_weight, ff_bias);

  std::cout << "func_rnn_demo\n";
  std::cout << "surface: pluss::fusion/pluss::rnn/pluss::gru/pluss::lstm via #include <pluss/pluss.h> + <pluss/ai.h>\n";
  std::cout << "rnn output: " << rnn_out << "\n";
  std::cout << "rnn h_n: " << rnn_hn << "\n";
  std::cout << "gru output: " << gru_out << "\n";
  std::cout << "gru h_n: " << gru_hn << "\n";
  std::cout << "lstm output: " << lstm_out << "\n";
  std::cout << "lstm h_n: " << std::get<0>(lstm_state) << "\n";
  std::cout << "lstm c_n: " << std::get<1>(lstm_state) << "\n";
  std::cout << "feedforward head on rnn output: " << head << "\n";

  return 0;
}
