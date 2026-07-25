// A tour of ice's current raw public API surface -- deliberately built from
// nothing but ice's headers plus plain pluss::/pluss::nn:: calls (no new
// convenience wrappers around ice itself), so any awkwardness in the API
// shows up here rather than being smoothed over. Doubles as a documentation
// seed: every section below is runnable, self-contained, and prints real
// output.
//
// Covers: a custom ice::Model subclass (raw register_parameter), ice::Outline
// (composed nn:: layers), all three TrainOptions fields and all three
// LossKind values, all three Model::learn() overloads, infer() with
// chunking, assess(), save()/load(), the standalone ice::make_optimizer()
// escape hatch, TrainPlatform/resolve_platform(), ice::io::save_parameters/
// load_parameters used directly (not through Model), ice::NoGradGuard used
// directly, ice::models::TemporalFusionTransformer end to end, a
// TransformerEncoder composed into an ice::Outline (Series-in/Series-out
// fits Schedule directly), an LSTM trained via a raw ice::make_optimizer
// loop (its forward() returns a tuple, so it doesn't fit Outline's Schedule
// contract), and ice::data::TimeSeriesDataSet turning a long-format
// DataFrame into a windowed TFT-ready batch with no manual TFTOptions/
// TFTBatch shape bookkeeping.
//
// [gap] notes mark places where the raw API turned out awkward or
// incomplete -- input for a future ice pass, not something this demo works
// around.

#include <pluss/ai/ice/data/time_series_dataset.h>
#include <pluss/ai/ice/io.h>
#include <pluss/ai/ice/model.h>
#include <pluss/ai/ice/models/tft.h>
#include <pluss/ai/ice/no_grad.h>
#include <pluss/ai/ice/optim.h>
#include <pluss/ai/ice/options.h>
#include <pluss/ai/ice/outline.h>
#include <pluss/ai/ice/platform.h>

#include <pluss/mic.h>
#include <pluss/ml.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pluss::Config;
using pluss::Platform;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
namespace ice = pluss::ai::ice;
namespace data = pluss::ai::ice::data;
namespace fs = std::filesystem;

Config f32() { return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

double scalar(const Series& x) { return x.to(pluss::pCPU, ScalarType::Float64).item<double>(); }

void print_section(std::string_view title) {
  std::cout << "\n=== " << title << " ===\n";
}

void report(const ice::AssessReport& r) {
  std::cout << "  loss=" << r.loss << "  " << r.metric_name << "=" << r.metric << "\n";
}

// ---------------------------------------------------------------------------
// A custom ice::Model subclass: the escape hatch for a model that isn't just
// a chain of existing nn:: layers. Registers two raw Series directly via the
// protected register_parameter() API rather than composing modules.
// ---------------------------------------------------------------------------
class LinearRegressionModel final : public ice::Model {
 public:
  LinearRegressionModel(std::int64_t in_features, const Config& cfg, std::uint64_t seed) {
    weight_ = pluss::randn({in_features, 1}, cfg, seed).mul(0.1);
    bias_ = pluss::randn({1}, cfg, seed + 1).mul(0.1);
    weight_.tracks_slope_(true);
    bias_.tracks_slope_(true);
    register_parameter(weight_);
    register_parameter(bias_);
  }

  [[nodiscard]] Series forward(const Series& x) const override { return x.matmul(weight_).add(bias_); }

 private:
  Series weight_;
  Series bias_;
};

} // namespace

int main() {
  const Config cfg = f32();

  // -------------------------------------------------------------------------
  print_section("1. Custom ice::Model subclass -- learn(x, y, options) [internal val split]");
  // -------------------------------------------------------------------------
  {
    const std::int64_t n = 400;
    const Series true_w = pluss::make_series(std::vector<float>{3.0f, -2.0f}, {2, 1}, cfg);
    const Series x = pluss::randn({n, 2}, cfg, 1001ULL);
    const Series noise = pluss::randn({n, 1}, cfg, 1002ULL).mul(0.05);
    const Series y = x.matmul(true_w).add(1.0).add(noise);

    LinearRegressionModel model(2, cfg, 2001ULL);

    ice::TrainOptions options{};
    options.max_epochs = 150;
    options.batch_size = 32;
    options.learning_rate = 0.05;
    options.optimizer = ice::OptimizerKind::Adam;
    options.loss = ice::LossKind::MSE;
    options.grad_clip.mode = pluss::nn::optim::GradientClipMode::ByNorm;
    options.grad_clip.threshold = 1.0;
    options.fail_on_non_finite = true;
    options.shuffle = true;
    options.seed = 7;
    options.val_ratio = 0.15;
    options.early_stopping = true;
    options.patience = 15;
    options.min_delta = 1e-6;
    options.platform = ice::TrainPlatform::CPU;

    std::cout << "  ready() before learn(): " << std::boolalpha << model.ready() << "\n";
    model.learn(x, y, options);
    std::cout << "  ready() after learn(): " << model.ready() << "\n";
    report(model.assess(x, y, options.loss));
  }

  // -------------------------------------------------------------------------
  print_section("2. ice::Outline (composed nn:: layers) -- learn(x_train, y_train, x_val, y_val, options) [explicit split]");
  // -------------------------------------------------------------------------
  ice::Outline model_b(
      pluss::nn::Fusion(2, 16, true, cfg, 4001ULL),
      pluss::nn::ReLU(),
      pluss::nn::Fusion(16, 1, true, cfg, 4002ULL));
  {
    const std::int64_t n = 500;
    const Series x = pluss::randn({n, 2}, cfg, 3001ULL);
    const Series x0 = x.select(1, 0).reshape({n, 1});
    const Series x1 = x.select(1, 1).reshape({n, 1});
    const Series noise = pluss::randn({n, 1}, cfg, 3002ULL).mul(0.05);
    const Series y = x0.square().sub(x1.mul(2.0)).add(1.0).add(noise); // nonlinear -- needs the hidden layer

    const std::int64_t split = static_cast<std::int64_t>(n * 0.8);
    const Series x_train = x.narrow_copy(0, 0, split);
    const Series y_train = y.narrow_copy(0, 0, split);
    const Series x_val = x.narrow_copy(0, split, n - split);
    const Series y_val = y.narrow_copy(0, split, n - split);

    ice::TrainOptions options{};
    options.max_epochs = 200;
    options.optimizer = ice::OptimizerKind::AdamW;
    options.loss = ice::LossKind::MSE;
    options.learning_rate = 0.01;
    options.platform = ice::TrainPlatform::CPU;

    model_b.learn(x_train, y_train, x_val, y_val, options);
    report(model_b.assess(x_val, y_val, options.loss));
  }

  // -------------------------------------------------------------------------
  print_section("3. ice::Outline -- learn(DataFrame, target, options) [BCEWithLogits, standardize=true]");
  // -------------------------------------------------------------------------
  {
    const std::int64_t n = 400;
    const Series x = pluss::randn({n, 2}, cfg, 5001ULL);
    const Series x0 = x.select(1, 0).reshape({n});
    const Series x1 = x.select(1, 1).reshape({n});
    const Series label = x0.add(x1).gt(0.0).to(ScalarType::Float32);

    const DataFrame df({
        {"x0", Thread(x0)},
        {"x1", Thread(x1)},
        {"label", Thread(label)},
    });

    ice::Outline model_c(
        pluss::nn::Fusion(2, 8, true, cfg, 6001ULL),
        pluss::nn::ReLU(),
        pluss::nn::Fusion(8, 1, true, cfg, 6002ULL));

    ice::TrainOptions options{};
    options.max_epochs = 100;
    options.optimizer = ice::OptimizerKind::RMSProp;
    options.loss = ice::LossKind::BCEWithLogits;
    options.learning_rate = 0.02;
    options.standardize = true;
    options.val_ratio = 0.2;

    model_c.learn(df, "label", options);

    // The DataFrame overloads of assess()/infer() reapply whatever
    // standardization learn() fit -- no manual mean/std recomputation
    // needed, and it survives a save()/load() round trip (verified below).
    report(model_c.assess(df, "label", options.loss));

    const fs::path out_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "ice_full_capability_demo";
    fs::create_directories(out_dir);
    const fs::path model_c_path = out_dir / "outline_bce_standardized.icemodel";
    model_c.save(model_c_path.string());

    ice::Outline model_c_reloaded(
        pluss::nn::Fusion(2, 8, true, cfg, 999999ULL), // seed irrelevant, overwritten by load()
        pluss::nn::ReLU(),
        pluss::nn::Fusion(8, 1, true, cfg, 999998ULL));
    model_c_reloaded.load(model_c_path.string());
    const ice::AssessReport report_c_reloaded = model_c_reloaded.assess(df, "label", options.loss);
    std::cout << "  after save()/load() round trip (standardization stats included):\n";
    report(report_c_reloaded);
  }

  // -------------------------------------------------------------------------
  print_section("4. ice::Outline -- LossKind::CrossEntropy (3-class) via learn(x, y, options)");
  // -------------------------------------------------------------------------
  {
    const std::int64_t n = 450;
    const Series x = pluss::randn({n, 3}, cfg, 7001ULL);
    const Series y = x.argmax(1).to(ScalarType::Int64); // [N] class indices: which raw feature is largest

    ice::Outline model_d(
        pluss::nn::Fusion(3, 16, true, cfg, 8001ULL),
        pluss::nn::ReLU(),
        pluss::nn::Fusion(16, 3, true, cfg, 8002ULL));

    ice::TrainOptions options{};
    options.max_epochs = 150;
    options.optimizer = ice::OptimizerKind::SGD;
    options.loss = ice::LossKind::CrossEntropy;
    options.learning_rate = 0.1;
    options.val_ratio = 0.15;

    model_d.learn(x, y, options);
    report(model_d.assess(x, y, options.loss));
  }

  // -------------------------------------------------------------------------
  print_section("5. Model::infer() with and without chunk_size -- results must match");
  // -------------------------------------------------------------------------
  {
    const Series x = pluss::randn({37, 2}, cfg, 9001ULL); // not a multiple of the chunk size below
    const Series whole = model_b.infer(x);          // chunk_size=0 -> one batched forward() call
    const Series chunked = model_b.infer(x, 8);      // chunk_size=8 -> 5 chunks (4x8 + 1x5), concatenated
    std::cout << "  max |chunked - whole| = " << scalar(whole.sub(chunked).abs().max())
              << " (should be ~0)\n";
  }

  // -------------------------------------------------------------------------
  print_section("6. Model::save() / load() round trip");
  // -------------------------------------------------------------------------
  {
    LinearRegressionModel trained(2, cfg, 2001ULL);
    const std::int64_t n = 200;
    const Series x = pluss::randn({n, 2}, cfg, 1001ULL);
    const Series true_w = pluss::make_series(std::vector<float>{3.0f, -2.0f}, {2, 1}, cfg);
    const Series y = x.matmul(true_w).add(1.0);
    ice::TrainOptions options{};
    options.max_epochs = 80;
    options.val_ratio = 0.0;
    trained.learn(x, y, options);

    fs::path out_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "ice_full_capability_demo";
    fs::create_directories(out_dir);
    const fs::path model_path = out_dir / "linear_regression.icemodel";
    trained.save(model_path.string());

    LinearRegressionModel reloaded(2, cfg, /*seed=*/999999ULL); // architecture must match; values get overwritten
    reloaded.load(model_path.string());

    const Series probe = pluss::randn({5, 2}, cfg, 10001ULL);
    const double diff = scalar(trained.infer(probe).sub(reloaded.infer(probe)).abs().max());
    std::cout << "  save/load round-trip max diff = " << diff << " (should be ~0)\n";
    std::cout << "  saved to " << model_path.string() << "\n";
  }

  // -------------------------------------------------------------------------
  print_section("7. Standalone ice::make_optimizer() -- for training loops that don't fit Model's shape");
  // -------------------------------------------------------------------------
  {
    Series w = pluss::randn({2, 1}, cfg, 11001ULL).mul(0.1);
    Series b = pluss::randn({1}, cfg, 11002ULL).mul(0.1);
    w.tracks_slope_(true);
    b.tracks_slope_(true);
    const std::vector<Series*> raw_parameters{&w, &b};

    ice::TrainOptions options{};
    options.optimizer = ice::OptimizerKind::Adagrad;
    options.learning_rate = 0.15;
    auto optimizer = ice::make_optimizer(raw_parameters, options);

    const std::int64_t n = 128;
    const Series true_w = pluss::make_series(std::vector<float>{2.0f, -1.0f}, {2, 1}, cfg);
    const Series x = pluss::randn({n, 2}, cfg, 11003ULL);
    const Series y = x.matmul(true_w).add(0.5);

    double last_loss = 0.0;
    for (int step = 0; step < 300; ++step) {
      const Series pred = x.matmul(w).add(b);
      const Series loss = pluss::nn::losses::MSE(pred, y);
      optimizer->clear();
      loss.reverse();
      optimizer->step();
      if (step == 299) last_loss = scalar(loss);
    }
    std::cout << "  hand-rolled loop, ice-constructed Adagrad optimizer, final MSE = " << last_loss << "\n";
  }

  // -------------------------------------------------------------------------
  print_section("8. TrainPlatform / resolve_platform() -- standalone device-selection utility");
  // -------------------------------------------------------------------------
  {
    for (const auto [name, requested] :
         {std::pair{"CPU", ice::TrainPlatform::CPU}, std::pair{"GPU", ice::TrainPlatform::GPU},
          std::pair{"Auto", ice::TrainPlatform::Auto}}) {
      const Platform resolved = ice::resolve_platform(requested);
      std::cout << "  TrainPlatform::" << name << " -> " << resolved << "\n";
    }
  }

  // -------------------------------------------------------------------------
  print_section("9. ice::io::save_parameters()/load_parameters() used directly (not through Model)");
  // -------------------------------------------------------------------------
  {
    Series a = pluss::randn({3, 3}, cfg, 12001ULL);
    Series b = pluss::randn({4}, cfg, 12002ULL);
    const fs::path out_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "ice_full_capability_demo";
    fs::create_directories(out_dir);
    const fs::path raw_path = out_dir / "raw_parameters.icemodel";
    ice::io::save_parameters(raw_path.string(), {&a, &b});

    Series a2 = Series::zeros({3, 3}, ScalarType::Float32, pluss::pCPU);
    Series b2 = Series::zeros({4}, ScalarType::Float32, pluss::pCPU);
    ice::io::load_parameters(raw_path.string(), {&a2, &b2});

    std::cout << "  round-trip max diff = "
              << std::max(scalar(a.sub(a2).abs().max()), scalar(b.sub(b2).abs().max())) << " (should be ~0)\n";
  }

  // -------------------------------------------------------------------------
  print_section("10. ice::NoGradGuard used directly -- toggles tracks_slope_() for its lifetime");
  // -------------------------------------------------------------------------
  {
    Series w = pluss::randn({2, 1}, cfg, 13001ULL);
    w.tracks_slope_(true);
    std::cout << "  tracks_slope() before guard: " << std::boolalpha << w.tracks_slope() << "\n";
    {
      const std::vector<Series*> params{&w};
      const ice::NoGradGuard guard(params);
      std::cout << "  tracks_slope() inside guard:  " << w.tracks_slope() << "\n";
    }
    std::cout << "  tracks_slope() after guard:   " << w.tracks_slope() << "\n";
  }

  // -------------------------------------------------------------------------
  print_section("11. ice::models::TemporalFusionTransformer -- end to end");
  // -------------------------------------------------------------------------
  {
    constexpr std::int64_t kEncoderLength = 10;
    constexpr std::int64_t kDecoderLength = 5;

    struct SyntheticBatch {
      Series static_inputs, encoder_inputs, decoder_inputs, target;
    };
    auto make_batch = [&](std::int64_t n, std::uint64_t seed) {
      std::mt19937_64 rng(seed);
      std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
      std::normal_distribution<double> noise(0.0, 0.02);
      std::vector<float> static_flat(static_cast<std::size_t>(n));
      std::vector<float> encoder_flat(static_cast<std::size_t>(n * kEncoderLength));
      std::vector<float> decoder_flat(static_cast<std::size_t>(n * kDecoderLength));
      std::vector<float> target_flat(static_cast<std::size_t>(n * kDecoderLength));
      for (std::int64_t i = 0; i < n; ++i) {
        const double level = level_dist(rng);
        static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
        for (std::int64_t t = 0; t < kEncoderLength; ++t) {
          encoder_flat[static_cast<std::size_t>(i * kEncoderLength + t)] = static_cast<float>(level + noise(rng));
        }
        for (std::int64_t t = 0; t < kDecoderLength; ++t) {
          const double position = static_cast<double>(t + 1) / static_cast<double>(kDecoderLength);
          decoder_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(position);
          target_flat[static_cast<std::size_t>(i * kDecoderLength + t)] =
              static_cast<float>(level + 0.5 * position + noise(rng));
        }
      }
      SyntheticBatch batch{};
      batch.static_inputs = pluss::make_series(static_flat, {n, 1}, f32());
      batch.encoder_inputs = pluss::make_series(encoder_flat, {n, kEncoderLength, 1}, f32());
      batch.decoder_inputs = pluss::make_series(decoder_flat, {n, kDecoderLength, 1}, f32());
      batch.target = pluss::make_series(target_flat, {n, kDecoderLength}, f32());
      return batch;
    };

    const auto train_data = make_batch(300, 21ULL);
    const auto val_data = make_batch(60, 22ULL);
    const auto test_data = make_batch(80, 97ULL);

    ice::models::TFTOptions options{};
    options.static_size = 1;
    options.encoder_size = 1;
    options.decoder_size = 1;
    options.encoder_length = kEncoderLength;
    options.decoder_length = kDecoderLength;
    options.hidden_size = 16;
    options.lstm_layers = 1;
    options.attention_heads = 2;
    options.dropout = 0.0;
    options.quantiles = {0.1, 0.5, 0.9};
    options.cfg = f32();
    options.seed = 5;

    ice::models::TemporalFusionTransformer model(options);
    std::cout << "  ready() before learn(): " << std::boolalpha << model.ready() << "\n";

    ice::models::TFTBatch train_batch{};
    train_batch.static_inputs = train_data.static_inputs;
    train_batch.encoder_inputs = train_data.encoder_inputs;
    train_batch.decoder_inputs = train_data.decoder_inputs;
    train_batch.target = train_data.target;
    ice::models::TFTBatch val_batch{};
    val_batch.static_inputs = val_data.static_inputs;
    val_batch.encoder_inputs = val_data.encoder_inputs;
    val_batch.decoder_inputs = val_data.decoder_inputs;
    val_batch.target = val_data.target;
    ice::models::TFTBatch test_batch{};
    test_batch.static_inputs = test_data.static_inputs;
    test_batch.encoder_inputs = test_data.encoder_inputs;
    test_batch.decoder_inputs = test_data.decoder_inputs;
    test_batch.target = test_data.target;

    ice::models::TFTTrainOptions train_options{};
    train_options.max_epochs = 150;
    train_options.batch_size = 64;
    train_options.learning_rate = 1e-2;
    train_options.optimizer = ice::OptimizerKind::Adam;
    train_options.early_stopping = true;
    train_options.patience = 15;
    train_options.seed = 3;
    train_options.platform = ice::TrainPlatform::CPU;

    // The explicit train/val overload -- mirrors ice::Model's two-Series-pair
    // learn(), for callers managing their own splitting.
    model.learn(train_batch, val_batch, train_options);
    std::cout << "  ready() after learn(): " << model.ready() << "\n";

    const auto forecast = model.predict(test_batch);
    const auto qsizes = forecast.quantiles.sizes();
    std::cout << "  predict() quantiles shape: [" << qsizes[0] << ", " << qsizes[1] << ", " << qsizes[2] << "]\n";
    const auto asizes = forecast.attention_weights.sizes();
    std::cout << "  attention_weights shape:   [" << asizes[0] << ", " << asizes[1] << ", " << asizes[2] << "]\n";
    std::cout << "  attention row sums ~1:     "
              << scalar(forecast.attention_weights.sum(-1, false).sub(1.0).abs().max()) << " (should be ~0)\n";

    const Series median = forecast.quantiles.narrow_copy(2, 1, 1).reshape({qsizes[0], kDecoderLength});
    const double mean_abs_error = scalar(median.sub(test_data.target).abs().mean());
    std::cout << "  median forecast mean abs error: " << mean_abs_error << "\n";

    std::cout << "  assess(): " << model.assess(test_batch) << "\n";

    // Bring a (possibly GPU-)trained model back to CPU for downstream
    // inference -- a no-op here since training already ran on CPU, but
    // exercises the same to() path a GPU caller relies on.
    model.to(pluss::pCPU);
    const auto forecast_after_to = model.predict(test_batch);
    std::cout << "  predict() diff after to(pCPU): "
              << scalar(forecast.quantiles.sub(forecast_after_to.quantiles).abs().max()) << " (should be ~0)\n";
  }

  // -------------------------------------------------------------------------
  print_section("12. pluss::nn::TransformerEncoder -- ice::Outline sequence classification");
  // -------------------------------------------------------------------------
  {
    // TransformerEncoder/TransformerEncoderLayer's forward(src, mask={},
    // padding={}, is_causal=false) is Series-in/Series-out (the rest
    // default), so unlike LSTM below it composes directly into an
    // ice::Outline's Schedule -- along with a plain lambda mean-pooling
    // stage, which Schedule accepts just as readily as an nn:: module (any
    // callable with forward(Series)->Series or operator()(Series)->Series
    // works).
    const std::int64_t n = 300;
    const std::int64_t seq_len = 8;
    const std::int64_t d_model = 16;
    const Series x = pluss::randn({n, seq_len, d_model}, cfg, 16001ULL);
    // label: 1 iff the time-averaged first feature channel is positive --
    // forces the model to aggregate over the whole sequence, not just one
    // timestep.
    const Series label = x.select(2, 0).mean(1).gt(0.0).to(ScalarType::Float32).reshape({n, 1});

    ice::Outline model_transformer(
        pluss::nn::TransformerEncoder(d_model, /*nhead=*/4, /*num_layers=*/2, /*dim_feedforward=*/32,
                                       /*dropout=*/0.0, pluss::nn::TransformerActivation::ReLU,
                                       /*batch_first=*/true, /*norm_first=*/false, /*bias=*/true, cfg, 16002ULL),
        [](const Series& seq) { return seq.mean(1); }, // [N, T, D] -> [N, D]
        pluss::nn::Fusion(d_model, 1, true, cfg, 16003ULL));

    ice::TrainOptions options{};
    options.max_epochs = 80;
    options.optimizer = ice::OptimizerKind::Adam;
    options.loss = ice::LossKind::BCEWithLogits;
    options.learning_rate = 0.01;
    options.val_ratio = 0.15;

    model_transformer.learn(x, label, options);
    report(model_transformer.assess(x, label, options.loss));
  }

  // -------------------------------------------------------------------------
  print_section("13. pluss::nn::LSTM -- raw training loop via ice::make_optimizer");
  // -------------------------------------------------------------------------
  {
    // LSTM::forward(input, state={}) returns std::tuple<Series,
    // std::tuple<Series, Series>> (the output sequence plus final
    // (h, c)) -- unlike Transformer above, that doesn't fit Schedule's
    // Series-in/Series-out stage contract, so this is exactly the "doesn't
    // fit ice::Model's generic shape" scenario ice::make_optimizer() exists
    // for (section 7 demonstrated the same escape hatch with a hand-rolled
    // linear layer; this is the same pattern with a real multi-module
    // parameter list spanning two separate nn:: modules).
    const std::int64_t n = 300;
    const std::int64_t seq_len = 10;
    const std::int64_t input_size = 4;
    const std::int64_t hidden_size = 16;

    const Series x = pluss::randn({n, seq_len, input_size}, cfg, 17001ULL);
    const Series last_step = x.select(1, seq_len - 1); // [N, input_size]
    // label: 1 iff the last timestep's first two features sum positive.
    const Series label =
        last_step.select(1, 0).add(last_step.select(1, 1)).gt(0.0).to(ScalarType::Float32).reshape({n, 1});

    pluss::nn::LSTM lstm(pluss::nn::LSTMOptions{
        .input_size = input_size,
        .hidden_size = hidden_size,
        .num_layers = 1,
        .bias = true,
        .batch_first = true,
        .dropout = 0.0,
        .bidirectional = false,
        .split_bias = false,
        .proj_size = 0,
        .cfg = cfg,
        .seed = 17002ULL,
    });
    pluss::nn::Fusion head(hidden_size, 1, true, cfg, 17003ULL);

    std::vector<Series*> raw_parameters = lstm.parameters();
    const auto head_parameters = head.parameters();
    raw_parameters.insert(raw_parameters.end(), head_parameters.begin(), head_parameters.end());

    ice::TrainOptions optimizer_options{};
    optimizer_options.optimizer = ice::OptimizerKind::Adam;
    optimizer_options.learning_rate = 0.01;
    auto optimizer = ice::make_optimizer(raw_parameters, optimizer_options);

    double last_loss = 0.0;
    for (int step = 0; step < 200; ++step) {
      const auto [output, state] = lstm->forward(x);
      (void)state;
      const Series last_hidden = output.select(1, seq_len - 1); // [N, hidden_size]
      const Series logits = head->forward(last_hidden);
      const Series loss = pluss::nn::losses::BCEWithLogits(logits, label);
      optimizer->clear();
      loss.reverse();
      optimizer->step();
      if (step == 199) last_loss = scalar(loss);
    }

    const auto [final_output, final_state] = lstm->forward(x);
    (void)final_state;
    const Series predicted =
        head->forward(final_output.select(1, seq_len - 1)).sigmoid().ge(0.5).to(ScalarType::Float32);
    const double accuracy = scalar(predicted.eq(label).to(ScalarType::Float32).mean());
    std::cout << "  hand-rolled loop, LSTM+Fusion via ice-constructed Adam optimizer: final BCE loss="
              << last_loss << " accuracy=" << accuracy << "\n";
  }

  // -------------------------------------------------------------------------
  print_section("14. ice::data::TimeSeriesDataSet -- long-format DataFrame to a TFT-ready batch");
  // -------------------------------------------------------------------------
  {
    // A long-format panel: one row per series per timestep. "region" is a
    // static categorical covariate; "position" is known for the whole
    // window (encoder+decoder); "noisy_level" is observed (encoder-only) --
    // TimeSeriesDataSet figures out static_size/encoder_size/decoder_size
    // and which slots are categorical from this column typing alone.
    constexpr std::int64_t kSeries = 150;
    constexpr std::int64_t kStepsPerSeries = 20;
    constexpr std::int64_t kEncoderLength = 10;
    constexpr std::int64_t kDecoderLength = 5;

    std::mt19937_64 rng(18001ULL);
    std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
    std::normal_distribution<double> noise(0.0, 0.02);
    std::vector<std::string> series_col{};
    std::vector<std::string> region_col{};
    std::vector<float> time_col{}, level_col{}, position_col{}, noisy_col{}, y_col{};
    for (std::int64_t s = 0; s < kSeries; ++s) {
      const double level = level_dist(rng);
      const std::string region = (s % 2 == 0) ? "north" : "south";
      for (std::int64_t t = 0; t < kStepsPerSeries; ++t) {
        const double position = static_cast<double>(t) / static_cast<double>(kStepsPerSeries - 1);
        series_col.push_back("series" + std::to_string(s));
        region_col.push_back(region);
        time_col.push_back(static_cast<float>(t));
        level_col.push_back(static_cast<float>(level));
        position_col.push_back(static_cast<float>(position));
        noisy_col.push_back(static_cast<float>(level + noise(rng)));
        y_col.push_back(static_cast<float>(level + 0.5 * position + noise(rng)));
      }
    }
    const DataFrame panel({
        {"series", Thread(pluss::make_note(series_col))},
        {"region", Thread(pluss::make_note(region_col))},
        {"t", Thread(pluss::make_series(time_col, {static_cast<std::int64_t>(time_col.size())}, cfg))},
        {"level", Thread(pluss::make_series(level_col, {static_cast<std::int64_t>(level_col.size())}, cfg))},
        {"position", Thread(pluss::make_series(position_col, {static_cast<std::int64_t>(position_col.size())}, cfg))},
        {"noisy_level", Thread(pluss::make_series(noisy_col, {static_cast<std::int64_t>(noisy_col.size())}, cfg))},
        {"y", Thread(pluss::make_series(y_col, {static_cast<std::int64_t>(y_col.size())}, cfg))},
    });

    data::TimeSeriesDataSetOptions ds_options{};
    ds_options.time_idx = "t";
    ds_options.group_ids = {"series"};
    ds_options.target = "y";
    ds_options.variables = {
        {"region", data::VariableKind::StaticCategorical},
        {"level", data::VariableKind::StaticContinuous},
        {"position", data::VariableKind::KnownContinuous},
        {"noisy_level", data::VariableKind::ObservedContinuous},
    };
    ds_options.encoder_length = kEncoderLength;
    ds_options.decoder_length = kDecoderLength;
    ds_options.cfg = cfg;

    const auto dataset = data::TimeSeriesDataSet::from_dataframe(panel, ds_options);
    std::cout << "  windowed samples: " << dataset.size() << "\n";
    std::cout << "  static_size=" << dataset.batch().static_size << " encoder_size=" << dataset.batch().encoder_size
              << " decoder_size=" << dataset.batch().decoder_size << "\n";
    std::cout << "  fitted categorical vocabularies: " << dataset.vocabularies().size() << " ('"
              << dataset.vocabularies().front().variable_name << "', "
              << dataset.vocabularies().front().categories.size() << " codes incl. the reserved unknown bucket)\n";

    // TFTOptions::from_dataset() derives static_size/encoder_size/
    // decoder_size/*_categoricals from the dataset directly -- no manual
    // bookkeeping to keep in sync with what the DataFrame actually
    // contained. Hyperparameters (hidden_size, quantiles, ...) still need
    // setting, same as building TFTOptions by hand.
    auto tft_options = ice::models::TFTOptions::from_dataset(dataset);
    tft_options.encoder_length = kEncoderLength;
    tft_options.decoder_length = kDecoderLength;
    tft_options.hidden_size = 16;
    tft_options.attention_heads = 2;
    tft_options.quantiles = {0.1, 0.5, 0.9};
    tft_options.cfg = cfg;
    tft_options.seed = 5;

    ice::models::TemporalFusionTransformer dataset_model(tft_options);
    ice::models::TFTTrainOptions train_options{};
    train_options.max_epochs = 150;
    train_options.val_ratio = 0.15;
    train_options.seed = 3;

    // learn()/predict() take the data::TimeSeriesBatch directly -- no
    // TFTBatch wrapping needed at the call site.
    dataset_model.learn(dataset.batch(), train_options);
    std::cout << "  assess(): " << dataset_model.assess(dataset.batch()) << "\n";
  }

  std::cout << "\nice full-capability demo finished.\n";
  return 0;
}
