// x() is the first field, and y() is the second. has_y() tells you whether a second field exists
// before you reach for it, since a batch with only one field has no y() to call.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  loader::DataBatch two_fields{};
  two_fields.fields.push_back(pluss::make_series(std::vector<double>{1, 2}, {2}, cfg));
  two_fields.fields.push_back(pluss::make_series(std::vector<double>{9, 8}, {2}, cfg));
  std::cout << "has_y: " << two_fields.has_y() << '\n';
  std::cout << "x is the first field: " << (two_fields.x().host<double>() == two_fields.at(0).host<double>()) << '\n';
  std::cout << "y is the second field: " << (two_fields.y().host<double>() == two_fields.at(1).host<double>()) << '\n';

  loader::DataBatch one_field{};
  one_field.fields.push_back(pluss::make_series(std::vector<double>{5}, {1}, cfg));
  std::cout << "one field has_y: " << one_field.has_y() << '\n';
  return 0;
}
