#include <pluss/pluss.h>
#include <pluss/tils.h>
#include <pluss/nn.h>
#include <pluss/mic.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

struct StockLSTMImpl final : pluss::nn::Module {
  pluss::nn::LSTM lstm;
  pluss::nn::Fusion head;

  explicit StockLSTMImpl(const Config& cfg)
      : Module(true),
        lstm(pluss::nn::LSTMOptions{
            .input_size = 4,
            .hidden_size = 32,
            .num_layers = 2,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .proj_size = 0,
            .cfg = cfg,
            .seed = 2026ULL,
            .training = true,
        }),
        head(32, 1, true, cfg, 2027ULL) {
    module_record("lstm", lstm);
    module_record("head", head);
  }

  Series forward(const Series& x) const {
    const auto [seq, state] = lstm->forward(x);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(StockLSTM);

} // namespace


int main() {
  namespace fs = std::filesystem;

  fs::path csv_path;
#ifdef PLUSSDEV_SOURCE_DIR
  {
    const fs::path root = fs::path(PLUSSDEV_SOURCE_DIR);
    const fs::path candidate = root / "PlussMic" / "data" / "sp500_data.csv";
    if (fs::exists(candidate)) csv_path = candidate;
  }
#endif
  if (csv_path.empty()) {
    for (const fs::path& candidate : {
             fs::path("PlussMic/data/sp500_data.csv"),
             fs::path("../PlussMic/data/sp500_data.csv"),
             fs::path("../../PlussMic/data/sp500_data.csv"),
             fs::path("../../../PlussMic/data/sp500_data.csv"),
         }) {
      if (fs::exists(candidate)) {
        csv_path = candidate;
        break;
      }
    }
  }
  if (csv_path.empty()) {
    std::cerr << "could not find PlussMic/data/sp500_data.csv\n";
    return 1;
  }

  pluss::loader::ReadCsvOptions read_opts;
  read_opts.skipcases = {1, 2};
  read_opts.parse_dates = {"Price"};

  auto df = pluss::table::DataFrame::load_csv(csv_path.string(), read_opts);
  df = df.rename("Price", "Date");
  std::cout << "source=" << csv_path << "\n";
  std::cout << "raw rows=" << df.ncases() << " features=" << df.nfeatures() << "\n";
  std::cout << fmt::format("feature names=[{}]\n", fmt::join(df.features(), ", "));

  auto work = df.features({"Date", "Adj Close", "High", "Low", "Volume"});
  work.set("Ret1D") = work.features("Adj Close") / work.features("Adj Close").shift( 1) - 1.0;

  work.set({"RangePct", "LogVolume"}) = {
      (work.features("High") - work.features("Low")) / work.features("Adj Close"),
      (work.features("Volume") + 1.0).log(),
  };
  work = work.dropnull().label().set("Date", true);
  // Full-history mode: keep all available rows for stress coverage.
  work.head(10);
  std::cout << "prepared rows=" << work.ncases() << " features=" << work.nfeatures() << "\n";
  
  const pluss::Platform platform = pluss::mps::is_ready() ? pluss::pMPS : pluss::pCPU;
  const ScalarType dtype = ScalarType::Float32;
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);
  const Config cpu_f32 = Config{}.with_dtype(dtype).with_platform(pluss::pCPU);
  const Config cpu_i64 = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);

  // Build training windows on CPU in f32; move only final batches to target platform.
  const Series close = work.features("Adj Close").as_series().to(ScalarType::Float32);
  const Series ret1d = work.features("Ret1D").as_series().to(ScalarType::Float32);
  const Series range_pct = work.features("RangePct").as_series().to(ScalarType::Float32);
  const Series vol = work.features("LogVolume").as_series().to(ScalarType::Float32);

  const Series eps = pluss::make_series(1e-8f, cpu_f32);
  const Series close_min = close.min();
  const Series close_max = close.max();
  const Series ret_min = ret1d.min();
  const Series ret_max = ret1d.max();
  const Series range_min = range_pct.min();
  const Series range_max = range_pct.max();
  const Series vol_min = vol.min();
  const Series vol_max = vol.max();

  const Series close_scaled = close.sub(close_min).div(close_max.sub(close_min).add(eps));
  const Series ret_scaled = ret1d.sub(ret_min).div(ret_max.sub(ret_min).add(eps));
  const Series range_scaled = range_pct.sub(range_min).div(range_max.sub(range_min).add(eps));
  const Series vol_scaled = vol.sub(vol_min).div(vol_max.sub(vol_min).add(eps));

  const std::int64_t lookback = 40;
  const std::int64_t n = close_scaled.numel() - lookback - 1;
  if (n <= 256) {
    std::cerr << "not enough rows after preprocessing\n";
    return 1;
  }

  const auto close_vec = close_scaled.to_vec<float>();
  const auto ret_vec = ret_scaled.to_vec<float>();
  const auto range_vec = range_scaled.to_vec<float>();
  const auto vol_vec = vol_scaled.to_vec<float>();

  const std::size_t samples = static_cast<std::size_t>(n);
  const std::size_t lb = static_cast<std::size_t>(lookback);
  std::vector<float> xbuf(samples * lb * 4);
  std::vector<float> ybuf(samples * lb);

  for (std::size_t i = 0; i < samples; ++i) {
    for (std::size_t t = 0; t < lb; ++t) {
      const std::size_t src = i + t;
      const std::size_t base = (i * lb + t) * 4;
      xbuf[base + 0] = close_vec[src];
      xbuf[base + 1] = ret_vec[src];
      xbuf[base + 2] = range_vec[src];
      xbuf[base + 3] = vol_vec[src];
      ybuf[i * lb + t] = close_vec[src + 1];
    }
  }
  Series features = pluss::make_series(xbuf, {n, lookback, 4}, cpu_f32);
  Series target = pluss::make_series(ybuf, {n, lookback, 1}, cpu_f32);

  const std::int64_t train_n = static_cast<std::int64_t>(static_cast<double>(n) * 0.8);
  const std::int64_t test_n = n - train_n;
  const Series train_idx = pluss::series::arange(0, train_n, cpu_i64);
  const Series test_idx = pluss::series::arange(train_n, n, cpu_i64);
  const Series x_train_cpu = features.index_select(0, train_idx);
  const Series y_train_cpu = target.index_select(0, train_idx);
  const Series x_test_cpu = features.index_select(0, test_idx);
  const Series y_test_cpu = target.index_select(0, test_idx);

  StockLSTM model(cfg);
  model->train(true);
  model->to(platform, dtype);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 2e-3;
  pluss::nn::optim::Adam optimizer(model->parameters(), opt);

  const std::int64_t batch_size = 256;
  for (int epoch = 1; epoch <= 160; ++epoch) {
    double train_loss_sum = 0.0;
    std::int64_t train_batches = 0;
    for (std::int64_t start = 0; start < train_n; start += batch_size) {
      const std::int64_t end = std::min(start + batch_size, train_n);
      const Series batch_idx = pluss::series::arange(start, end, cpu_i64);
      const Series xb = x_train_cpu.index_select(0, batch_idx).to(platform, dtype);
      const Series yb = y_train_cpu.index_select(0, batch_idx).to(platform, dtype);
      const Series loss = optimizer.step([&]() -> Series {
        return pluss::nn::losses::MSE(model->forward(xb), yb);
      });
      train_loss_sum += loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
      ++train_batches;
    }

    const double train_mse = train_loss_sum / static_cast<double>(std::max<std::int64_t>(1, train_batches));
    if (epoch == 1 || epoch % 20 == 0) {
      std::cout << "epoch=" << epoch << " train_mse=" << train_mse << "\n";
    }
  }

  model->eval();
  const Series close_span = close_max.sub(close_min).to(platform, dtype);
  const Series close_floor = close_min.to(platform, dtype);

  double test_sq_sum = 0.0;
  double price_abs_sum = 0.0;
  double price_sq_sum = 0.0;
  double dir_correct = 0.0;
  std::int64_t test_elem_count = 0;
  std::int64_t test_price_count = 0;
  std::vector<double> preview_pred;
  std::vector<double> preview_actual;

  for (std::int64_t start = 0; start < test_n; start += batch_size) {
    const std::int64_t end = std::min(start + batch_size, test_n);
    const std::int64_t bs = end - start;
    const Series batch_idx = pluss::series::arange(start, end, cpu_i64);
    const Series xb = x_test_cpu.index_select(0, batch_idx).to(platform, dtype);
    const Series yb = y_test_cpu.index_select(0, batch_idx).to(platform, dtype);
    const Series pred = model->forward(xb);

    const Series diff = pred.sub(yb);
    test_sq_sum += diff.square().sum().to(pluss::pCPU, ScalarType::Float64).item<double>();
    test_elem_count += diff.numel();

    const Series pred_next_scaled = pred.select(1, lookback - 1).reshape({bs, 1, 1});
    const Series actual_next_scaled = yb.select(1, lookback - 1).reshape({bs, 1, 1});
    const Series pred_price = pred_next_scaled.mul(close_span).add(close_floor);
    const Series actual_price = actual_next_scaled.mul(close_span).add(close_floor);
    const Series price_diff = pred_price.sub(actual_price);
    price_abs_sum += price_diff.abs().sum().to(pluss::pCPU, ScalarType::Float64).item<double>();
    price_sq_sum += price_diff.square().sum().to(pluss::pCPU, ScalarType::Float64).item<double>();
    test_price_count += price_diff.numel();

    const Series last_close_scaled = xb.select(1, lookback - 1).select(1, 0).reshape({bs, 1, 1});
    const Series last_close_price = last_close_scaled.mul(close_span).add(close_floor);
    const Series direction_match = pred_price.sub(last_close_price)
                                       .gt(0.0)
                                       .eq(actual_price.sub(last_close_price).gt(0.0))
                                       .to(dtype);
    dir_correct += direction_match.sum().to(pluss::pCPU, ScalarType::Float64).item<double>();

    if (preview_pred.size() < 5) {
      const auto pred_vec = pred_price.reshape({bs}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      const auto actual_vec = actual_price.reshape({bs}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      for (std::size_t i = 0; i < pred_vec.size() && preview_pred.size() < 5; ++i) {
        preview_pred.push_back(pred_vec[i]);
        preview_actual.push_back(actual_vec[i]);
      }
    }
  }

  const double test_mse_scaled = test_sq_sum / static_cast<double>(std::max<std::int64_t>(1, test_elem_count));
  const double test_mae_price = price_abs_sum / static_cast<double>(std::max<std::int64_t>(1, test_price_count));
  const double test_rmse_price = std::sqrt(price_sq_sum / static_cast<double>(std::max<std::int64_t>(1, test_price_count)));
  const double dir_acc = dir_correct / static_cast<double>(std::max<std::int64_t>(1, test_price_count));

  std::cout << "platform=" << (platform == pluss::pMPS ? "mps" : "cpu")
            << " dtype=f32 train_rows=" << train_n << " test_rows=" << test_n << "\n";
  std::cout << "test_mse_scaled=" << test_mse_scaled
            << " test_mae_price=" << test_mae_price
            << " test_rmse_price=" << test_rmse_price
            << " dir_acc=" << dir_acc << "\n";

  for (std::size_t i = 0; i < preview_pred.size(); ++i) {
    std::cout << "test[" << i << "] pred=" << preview_pred[i] << " actual=" << preview_actual[i] << "\n";
  }

  return 0;
}
