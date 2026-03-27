#include <pluss/pluss.h>
#include <pluss/tils.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <tuple>
#include <vector>

namespace {

struct RNNForecastModelImpl final : pluss::nn::Module {
  pluss::nn::RNN rnn;
  pluss::nn::Tanh activation;
  pluss::nn::Fusion head;

  explicit RNNForecastModelImpl(const pluss::Config& cfg)
      : Module(true),
        rnn(pluss::nn::RNNOptions{
            .input_size = 1,
            .hidden_size = 8,
            .num_layers = 1,
            .nonlinearity = "tanh",
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .cfg = cfg,
            .seed = 17ULL,
            .training = true,
        }),
        activation(),
        head(8, 1, true, cfg, 37ULL) {
    module_record("rnn", rnn);
    module_record("activation", activation);
    module_record("head", head);
  }

  RNNForecastModelImpl* operator->() noexcept { return this; }
  const RNNForecastModelImpl* operator->() const noexcept { return this; }

  std::tuple<pluss::Series, pluss::Series, pluss::Series> forward(const pluss::Series& input) const {
    const auto [features, state] = rnn->forward(input);
    const pluss::Series activated = activation->forward(features);
    return {head->forward(activated), activated, state};
  }
};

PLUSS_MODULE(RNNForecastModel);

pluss::Series make_series_dataset(const pluss::Config& cfg) {
  std::vector<double> values;
  values.reserve(6 * 12);
  for (std::int64_t batch = 0; batch < 6; ++batch) {
    for (std::int64_t step = 0; step < 12; ++step) {
      const double t = 0.25 * static_cast<double>(step) + 0.15 * static_cast<double>(batch);
      values.push_back(std::sin(t));
    }
  }
  return pluss::make_series(values, {6, 12, 1}, cfg);
}

pluss::Series take_range(const pluss::Series& x, std::int64_t dim, std::int64_t start, std::int64_t length) {
  std::vector<std::int64_t> indices(static_cast<std::size_t>(length));
  for (std::int64_t i = 0; i < length; ++i) indices[static_cast<std::size_t>(i)] = start + i;
  const pluss::Config icfg = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(x.platform());
  return x.index_select(dim, pluss::make_series(indices, {length}, icfg));
}

double mse(const pluss::Series& prediction, const pluss::Series& target) {
  return prediction.sub(target).pow(2.0).mean().to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

void report_model(const char* label,
                  const pluss::Series& features,
                  const pluss::Series& target,
                  const pluss::Series& hidden_features,
                  const pluss::Series& prediction) {
  std::cout << label << "\n";
  std::cout << "  feature shape: " << hidden_features.sizes()[0] << " x "
            << hidden_features.sizes()[1] << " x " << hidden_features.sizes()[2] << "\n";
  std::cout << "  prediction shape: " << prediction.sizes()[0] << " x "
            << prediction.sizes()[1] << " x " << prediction.sizes()[2] << "\n";
  std::cout << "  initial mse: " << mse(prediction, target) << "\n";
  std::cout << "  sample prediction: " << take_range(prediction, 0, 0, 1) << "\n";
  (void)features;
}

} // namespace

int main() {
  using namespace pluss;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series sequence = make_series_dataset(f64);
  const Series features = take_range(sequence, 1, 0, 11);
  const Series target = take_range(sequence, 1, 1, 11);

  RNNForecastModel rnn_model(f64);
  nn::GRU gru(nn::GRUOptions{
      .input_size = 1,
      .hidden_size = 8,
      .num_layers = 1,
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = true,
      .cfg = f64,
      .seed = 23ULL,
      .training = true,
  });
  nn::LSTM lstm(nn::LSTMOptions{
      .input_size = 1,
      .hidden_size = 8,
      .num_layers = 1,
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = true,
      .proj_size = 0,
      .cfg = f64,
      .seed = 31ULL,
      .training = true,
  });

  rnn_model->train(true);
  gru->train(true);
  lstm->train(true);
  rnn_model->to(pluss::pCPU, ScalarType::Float64);

  const let [rnn_pred, rnn_features, rnn_state] = rnn_model->forward(features);
  const let [gru_features, gru_state] = gru->forward(features);
  const let [lstm_features, lstm_state] = lstm->forward(features);
  const Series gru_pred = rnn_model->head->forward(gru_features);
  const Series lstm_pred = rnn_model->head->forward(lstm_features);

  std::cout << "nn_rnn_demo\n";
  std::cout << "task: next-step forecasting on a small sine-wave batch\n";
  std::cout << "composition: custom nn::Module with module_record(rnn, activation, head)\n";
  report_model("RNNModel", features, target, rnn_features, rnn_pred);
  report_model("GRU", features, target, gru_features, gru_pred);
  report_model("LSTM", features, target, lstm_features, lstm_pred);
  std::cout << "RNN h_n: " << rnn_state << "\n";
  std::cout << "GRU h_n: " << gru_state << "\n";
  std::cout << "LSTM h_n: " << std::get<0>(lstm_state) << "\n";
  std::cout << "LSTM c_n: " << std::get<1>(lstm_state) << "\n";

  return 0;
}
