#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <atomic>
#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(64);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {64}, cfg);

  // The synchronous reference: every batch is computed on the calling thread.
  loader::DataLoaderOptions synchronous{};
  synchronous.batch_size = 8;
  loader::DataLoader reference({x}, synchronous);

  // Two background workers compute batches ahead. In-order delivery is the default,
  // so the batches arrive in the same sequence as the reference.
  std::atomic<int> started{0};
  loader::DataLoaderOptions parallel = synchronous;
  parallel.num_workers = 2;
  parallel.prefetch_factor = 2;
  parallel.worker_init_fn = [&started](std::size_t) { started.fetch_add(1); };
  loader::DataLoader prefetched({x}, parallel);

  std::cout << "batches: " << prefetched.batch_count() << '\n';
  bool identical = reference.batch_count() == prefetched.batch_count();
  for (std::size_t i = 0; identical && i < reference.batch_count(); ++i) {
    identical = reference.batch_at(i).x().host<double>() == prefetched.batch_at(i).x().host<double>();
  }
  std::cout << "same batches in order: " << identical << '\n';

  // Iterating the prefetching loader yields the same order too.
  std::vector<double> streamed;
  for (auto batch : prefetched) {
    const auto v = batch.x().reshape({batch.x().numel()}).host<double>();
    streamed.insert(streamed.end(), v.begin(), v.end());
  }
  std::cout << "streamed order matches: " << (streamed == values) << '\n';
  std::cout << "workers started: " << started.load() << '\n';
}
