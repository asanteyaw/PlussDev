// How sure are we of an average? Resample the data with replacement many times and look at how the average moves.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Fifty delivery times in minutes, skewed to the right as delivery times are.
  auto times = (pluss::series::randn({50}, ScalarType::Float64, 11) * 0.5).exp() * 20.0;
  std::cout << "sample mean " << times.mean().item<double>() << ", sample median " << times.median().item<double>() << "\n";

  // 1. 2000 resamples at once: each row of `pick` is one resample of 50 positions drawn with replacement.
  const std::int64_t resamples = 2000, n = times.numel();
  auto pick = pluss::series::randint(0, n, {resamples, n}, ScalarType::Int64, 5);
  auto draws = times.index_select(0, pick.reshape({resamples * n})).reshape({resamples, n});
  std::cout << "resample matrix " << draws.size(0) << " x " << draws.size(1) << "\n";

  // 2. One statistic per resample: reduce along the resample axis.
  auto means = draws.mean(1);
  auto medians = draws.median(1);

  // 3. The spread of the statistic across resamples is its uncertainty; the 2.5% and 97.5% quantiles give a 95% interval.
  std::cout << "bootstrap standard error of the mean " << means.std().item<double>() << "\n";
  std::cout << "95% interval for the mean   [" << means.quantile(0.025).item<double>() << ", " << means.quantile(0.975).item<double>() << "]\n";
  std::cout << "95% interval for the median [" << medians.quantile(0.025).item<double>() << ", " << medians.quantile(0.975).item<double>() << "]\n";
}
