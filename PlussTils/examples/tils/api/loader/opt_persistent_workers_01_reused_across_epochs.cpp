// persistent_workers keeps worker threads alive between epochs instead of joining and respawning
// them. Both epochs still deliver every batch; the option changes how the threads are managed, not
// what is delivered.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  options.num_workers = 2;
  options.persistent_workers = true;
  loader::DataLoader data({x}, options);

  std::size_t first_epoch = 0;
  for (auto batch : data) first_epoch += batch.x().sizes()[0];
  std::size_t second_epoch = 0;
  for (auto batch : data) second_epoch += batch.x().sizes()[0];
  std::cout << "first epoch rows: " << first_epoch << '\n';
  std::cout << "second epoch rows: " << second_epoch << '\n';
  return 0;
}
