#include <pluss/pluss.h>
#include <pluss/tils.h>
#include <pluss/nn.h>

#include <cstdint>
#include <filesystem>
#include <iostream>

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

int main() {
  const auto csv_path = pluss::loader::file_path("data/bank-full.csv");
  if (!std::filesystem::exists(csv_path)) {
    std::cerr << "could not find bank-full.csv\n";
    return 1;
  }

  pluss::loader::ReadCsvOptions opts;
  opts.sep = ';';
  opts.ncases = 12000;
  opts.usefeats = {"age", "balance", "day", "duration", "campaign", "pdays", "previous", "y"};

  const auto df = pluss::table::DataFrame::load_csv(csv_path.string(), opts);
  const auto standardize = [](const pluss::Series& s) {
    const auto sf = s.to(ScalarType::Float32);
    return sf.sub(sf.mean()).div(sf.std().clamp_min(1e-6));
  };

  const std::int64_t in_features = 7;
  if (df.ncases() <= 1000) {
    std::cerr << "expected > 1000 rows in bank-full.csv\n";
    return 1;
  }

  const auto x = pluss::pile(
                     {
                         standardize(df.features("age").as_series()),
                         standardize(df.features("balance").as_series()),
                         standardize(df.features("day").as_series()),
                         standardize(df.features("duration").as_series()),
                         standardize(df.features("campaign").as_series()),
                         standardize(df.features("pdays").as_series()),
                         standardize(df.features("previous").as_series()),
                     },
                     0)
                     .transpose(0, 1)
                     .to(pluss::pCPU, ScalarType::Float32);
  const auto y = df.features("y")
                     .cast()
                     .series()
                     .as_series()
                     .to(pluss::pCPU, ScalarType::Float32)
                     .gt(0.5)
                     .to(pluss::pCPU, ScalarType::Float32)
                     .reshape({df.ncases(), 1});

  const auto split = pluss::loader::split_fields(
      {x, y},
      pluss::loader::DataSplitOptions{
          .train_ratio = 0.8,
          .val_ratio = 0.1,
          .shuffle = true,
          .seed = 2026ULL,
      });

  const pluss::Platform platform = pluss::pCPU;
  const ScalarType dtype = ScalarType::Float32;
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);

  const auto x_train = split.train_fields[0].to(platform, dtype);
  const auto y_train = split.train_fields[1].to(platform, dtype);
  const auto x_val = split.val_fields[0].to(platform, dtype);
  const auto y_val = split.val_fields[1].to(platform, dtype);
  const auto x_test = split.test_fields[0].to(platform, dtype);
  const auto y_test = split.test_fields[1].to(platform, dtype);

  pluss::nn::Schedule model(
      pluss::nn::Fusion(in_features, 24, false, cfg, 11ULL),
      pluss::nn::ReLU(),
      pluss::nn::Fusion(24, 1, false, cfg, 13ULL)
    );
    
  model->train(true);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 0.02;
  pluss::nn::optim::Adam optimizer(model->parameters(), opt);
  pluss::loader::DataLoader train_loader(
      {x_train, y_train},
      pluss::loader::DataLoaderOptions{
          .batch_size = 256,
          .shuffle = true,
          .drop_last = false,
          .seed = 2026ULL,
      });

  for (int epoch = 1; epoch <= 120; ++epoch) {
    pluss::Series loss{};
    for (const auto batch : train_loader) {
      const auto logits = model->forward(batch.x());
      loss = pluss::nn::losses::BCEWithLogits(logits, batch.y());
      optimizer.clear();
      loss.reverse();
      optimizer.step();
    }
    if (epoch == 1 || epoch % 30 == 0) {
      const auto val_loss = pluss::nn::losses::BCEWithLogits(model->forward(x_val), y_val);
      std::cout << "epoch=" << epoch
                << " train_loss=" << pluss::as_f64(loss)
                << " val_loss=" << pluss::as_f64(val_loss) << "\n";
    }
  }

  model->eval();
  const Series test_logits = model->forward(x_test);
  const Series test_loss = pluss::nn::losses::BCEWithLogits(test_logits, y_test);
  const Series test_pred = test_logits.sigmoid().ge(0.5).to(dtype);
  const Series test_acc = test_pred.eq(y_test).to(dtype).mean();

  std::cout << "nn_schedule_bank_default_minimal_demo\n";
  std::cout << "source=" << csv_path.string() << "\n";
  std::cout << "rows=" << df.ncases()
            << " train=" << x_train.sizes()[0]
            << " val=" << x_val.sizes()[0]
            << " test=" << x_test.sizes()[0] << "\n";
  std::cout << "test_loss=" << pluss::as_f64(test_loss)
            << " test_acc=" << pluss::as_f64(test_acc) << "\n";

  return 0;
}
