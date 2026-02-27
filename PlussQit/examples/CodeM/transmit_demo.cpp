#include <pluss/pluss.h>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

int square_int(int x) { return x * x; }

int add_index(int x, std::size_t index) { return x + static_cast<int>(index); }

int add_pair(int a, int b) { return a + b; }

int add_pair_with_index(int a, int b, std::size_t index) {
  return a + b + static_cast<int>(index);
}

double celsius_to_fahrenheit(double c) { return (c * 9.0 / 5.0) + 32.0; }

pluss::Series add_service_fee(pluss::Series bill) { return bill.add(2.0); }

struct Ticket {
  std::string event;
  double price{0.0};
};

struct AddVAT {
  double rate{0.0};

  Ticket operator()(const Ticket& t, std::size_t index) const {
    Ticket out = t;
    out.price = t.price * (1.0 + rate);
    out.event += " [batch " + std::to_string(index) + "]";
    return out;
  }
};

struct AttachSeats {
  std::string operator()(const Ticket& t, int seats) const {
    return t.event + " -> seats=" + std::to_string(seats) +
           ", total=$" + std::to_string(t.price * static_cast<double>(seats));
  }
};

template <class T>
void print_vec(const std::string& label, const std::vector<T>& values) {
  std::cout << label << ": [";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << values[i];
  }
  std::cout << "]\n";
}

void print_ticket_vec(const std::string& label, const std::vector<Ticket>& values) {
  std::cout << label << ":\n";
  for (const auto& t : values) {
    std::cout << "  - " << t.event << " -> $" << std::fixed << std::setprecision(2) << t.price << "\n";
  }
}

} // namespace

int main() {
  std::cout << "=== pluss::transmit demo ===\n\n";

  std::cout << "1) Single value (non-range)\n";
  const auto single_square = pluss::transmit(square_int, 9);
  std::cout << "transmit(square_int, 9) = " << single_square << "\n\n";

  std::cout << "2) Range + free functions\n";
  const std::vector<int> xs{1, 2, 3, 4, 5};
  const auto squared = pluss::transmit(square_int, xs);
  const auto plus_idx = pluss::transmit(add_index, xs);
  print_vec("squared", squared);
  print_vec("plus_idx", plus_idx);
  std::cout << "\n";

  std::cout << "3) Iterator overload + transmit_into\n";
  const auto tail_squared = pluss::transmit(square_int, xs.begin() + 2, xs.end());
  print_vec("tail_squared", tail_squared);
  std::vector<int> into_out;
  into_out.reserve(xs.size());
  pluss::transmit_into(add_index, xs, std::back_inserter(into_out));
  print_vec("transmit_into(add_index)", into_out);
  std::cout << "\n";

  std::cout << "4) Zip two ranges (binary)\n";
  const std::vector<int> a{10, 20, 30, 40};
  const std::vector<int> b{1, 2, 3};
  const auto zipped_add = pluss::transmit(add_pair, a, b);
  const auto zipped_add_i = pluss::transmit(add_pair_with_index, a, b);
  print_vec("zip add", zipped_add);
  print_vec("zip add + index", zipped_add_i);
  std::cout << "\n";

  std::cout << "5) Parallel policy (same API, different execution)\n";
  pluss::transmit_options opts;
  opts.exec = pluss::transmit_exec::par;
  opts.grain_size = 2;
  opts.workers = 4;
  const auto par_result = pluss::transmit(add_index, xs, opts);
  print_vec("parallel add_index", par_result);
  std::cout << "\n";

  std::cout << "6) User-defined objects (functors)\n";
  const std::vector<Ticket> tickets{
      {"Concert A", 120.0},
      {"Concert B", 80.0},
      {"Conference", 300.0},
  };
  const auto taxed = pluss::transmit(AddVAT{0.15}, tickets);
  print_ticket_vec("after VAT", taxed);

  const std::vector<int> seats{2, 4, 1};
  const auto totals = pluss::transmit(AttachSeats{}, taxed, seats);
  print_vec("ticket totals", totals);
  std::cout << "\n";

  std::cout << "7) Lambdas (captures + ad-hoc transforms)\n";
  const std::vector<double> temps_c{0.0, 10.0, 20.0, 37.0};
  const auto temps_f = pluss::transmit(celsius_to_fahrenheit, temps_c);
  print_vec("temperatures F", temps_f);

  const double scale = 1.25;
  const auto scaled = pluss::transmit([scale](double x, std::size_t i) {
    return x * scale + static_cast<double>(i);
  }, temps_f);
  print_vec("scaled with lambda", scaled);
  std::cout << "\n";

  std::cout << "8) With pluss::Series\n";
  auto bill = pluss::make_series(std::vector<double>{42.5, 35.85, 50.03}, {3});
  const auto with_fee = pluss::transmit(add_service_fee, bill);
  print_vec("single Series via transmit(add_service_fee, bill)", with_fee.to_vec<double>());

  const std::vector<pluss::Series> batches{
      pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}),
      pluss::make_series(std::vector<double>{4.0, 5.0, 6.0}, {3}),
  };
  const auto batch_sums = pluss::transmit([](const pluss::Series& s, std::size_t i) {
    return s.mul(1.0 + static_cast<double>(i)).sum().to_vec<double>(0);
  }, batches);
  print_vec("vector<Series> -> scalar summary with lambda", batch_sums);

  std::cout << "\nDone.\n";
  return 0;
}
