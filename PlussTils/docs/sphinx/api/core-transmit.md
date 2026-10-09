# `pluss::tils::transmit`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/bi/specs.h>

#include <pluss/tils/aux/transmit.h>

enum class transmit_exec { seq, par };
struct transmit_options { transmit_exec exec{seq}; std::size_t grain_size{4096}; std::size_t workers{0}; };

template <class F, class T> auto transmit(F&& fn, T&& value);  // T is not a range
template <class F, std::ranges::input_range R> auto transmit(F&& fn, R&& input, transmit_options opts = {});
template <class F, class It, class Sent> auto transmit(F&& fn, It first, Sent last, transmit_options opts = {});
template <class F, std::ranges::input_range R1, std::ranges::input_range R2> auto transmit(F&& fn, R1&& a, R2&& b);
template <class F, std::ranges::input_range R, class OutIt> OutIt transmit_into(F&& fn, R&& input, OutIt out);
```

`transmit` applies a callable to every element of an input and collects the results into a new `std::vector`. The input decides which overload is used: a single value that is not a range is passed to the callable once; a range is visited in order; two iterators mark a part of a container; two ranges are paired element by element and stop at the shorter one. `transmit_into` does the same work as the range form, but writes each result to an output iterator instead of building a vector, so a long or unbounded output does not need to be held in memory.

The callable may take one argument, the element, or two, the element and its zero-based position, which `transmit` detects from the callable's signature. The input is never modified.

The range form and `transmit_into` take a `transmit_options`. The default, `transmit_exec::seq`, visits the input on the calling thread, in order. `transmit_exec::par` splits a random-access, sized range into chunks sized by `grain_size`, runs each chunk on its own thread, up to `workers` threads (0 means the hardware's reported concurrency), and writes each result into its own slot, so the output keeps the input's order regardless of which thread finished first.

## Parameters

| Parameter | Meaning |
|---|---|
| `fn` | The callable. It takes the element, or the element and its position. |
| `value` / `input` / `first, last` / `a, b` | The value, range, iterator pair, or two ranges to map over. |
| `opts` | Execution options: sequential or parallel, the chunk size, and the thread count. Read by the range form and by `transmit_into`. |

## Returns

The range, iterator-pair, and two-range forms return a `std::vector` of the callable's results, in input order. The single-value form returns the callable's result directly. `transmit_into` returns the output iterator, advanced past the last element it wrote.

## Errors

None from `transmit` itself. A callable that accepts neither `(element)` nor `(element, index)` is rejected at compile time.

## Practical Examples

### Example 1

Example 1 maps a function over a vector, the ordinary range form.

```cpp
// Scaling sensor readings to a share of the largest one. The same callable runs over every reading,
// and transmit returns a new vector of the results, so the original readings are untouched.
#include <pluss/tils/aux/transmit.h>

#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  const std::vector<double> readings{12.0, 48.0, 30.0, 6.0};
  const double peak = *std::max_element(readings.begin(), readings.end());

  const auto shares = pluss::tils::transmit([peak](double r) { return r / peak; }, readings);

  for (std::size_t i = 0; i < shares.size(); ++i) {
    std::cout << "reading " << readings[i] << " is " << shares[i] * 100.0 << "% of peak\n";
  }
  std::cout << "original still holds " << readings.size() << " readings\n";
  return 0;
}
```

Output

```text
reading 12 is 25% of peak
reading 48 is 100% of peak
reading 30 is 62.5% of peak
reading 6 is 12.5% of peak
original still holds 4 readings
```

The result is a new vector holding one value per input reading, and the input is unchanged.

### Example 2

Example 2 uses a callable that takes the element's position as a second argument.

```cpp
// Numbering the entries of a checklist. When the callable takes a second argument, transmit passes the
// position of each element, so the label can depend on where the entry sits in the list.
#include <pluss/tils/aux/transmit.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const std::vector<std::string> steps{"pull", "build", "test", "deploy"};

  const auto labels = pluss::tils::transmit(
      [](const std::string& step, std::size_t position) {
        return std::to_string(position + 1) + ". " + step;
      },
      steps);

  for (const auto& label : labels) std::cout << label << '\n';
  return 0;
}
```

Output

```text
1. pull
2. build
3. test
4. deploy
```

Each label is built from its own zero-based position, read as the second argument.

### Example 3

Example 3 streams the results to an output iterator with transmit_into, instead of building a vector.

```cpp
// Writing results straight to an output iterator, so no intermediate vector is built. A report that
// may be very long can stream its lines to a file, a socket, or the terminal as it computes them.
#include <pluss/tils/aux/transmit.h>

#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main() {
  const std::vector<int> minutes{5, 42, 90, 181};

  auto out = pluss::tils::transmit_into(
      [](int m) { return std::to_string(m / 60) + "h " + std::to_string(m % 60) + "m"; },
      minutes,
      std::ostream_iterator<std::string>(std::cout, "\n"));
  (void)out;
  return 0;
}
```

Output

```text
0h 5m
0h 42m
1h 30m
3h 1m
```

Each result is written to standard output as it is produced; no intermediate vector is built.

### Example 4

Example 4 confirms that transmit_into returns the output iterator, advanced past what it wrote.

```cpp
#include <pluss/tils/aux/transmit.h>

#include <iostream>
#include <vector>

using namespace pluss::tils;

int main() {
  std::vector<int> out;
  auto end_it = transmit_into([](int x) { return x * x; }, std::vector<int>{1, 2, 3}, std::back_inserter(out));
  (void)end_it;  // a back_insert_iterator has no meaningful position to print; the call succeeded if out is filled
  std::cout << "squares:";
  for (int v : out) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
```

Output

```text
squares: 1 4 9
```

The returned iterator is not printed directly, because an insert iterator has no readable position, but the elements it wrote are visible afterward.

### Example 5

Example 5 pairs two ranges of different lengths.

```cpp
// Pairing two lists element by element. A price list and a quantity list are combined into line
// totals. When the lists differ in length, the result stops at the shorter one.
#include <pluss/tils/aux/transmit.h>

#include <iostream>
#include <vector>

int main() {
  const std::vector<double> prices{2.50, 10.00, 4.25};
  const std::vector<int> quantities{4, 1, 3, 9};

  const auto totals = pluss::tils::transmit(
      [](double price, int quantity) { return price * quantity; }, prices, quantities);

  for (double total : totals) std::cout << "line total: " << total << '\n';
  std::cout << "lines: " << totals.size() << " (the ninth quantity has no price)\n";
  return 0;
}
```

Output

```text
line total: 10
line total: 10
line total: 12.75
lines: 3 (the ninth quantity has no price)
```

The result has as many elements as the shorter input, and the extra element of the longer one is never visited.

### Example 6

Example 6 passes an iterator pair that marks the middle of a container.

```cpp
// Transforming a part of a container. Two iterators mark the start and the end, so only the middle
// of a list is changed. The rest of the list is not visited at all.
#include <pluss/tils/aux/transmit.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const std::vector<std::string> names{"ama", "kofi", "efua", "yaw", "akosua"};

  const auto upper = pluss::tils::transmit(
      [](const std::string& s) {
        std::string out = s;
        for (char& c : out) c = static_cast<char>(c - ('a' - 'A'));
        return out;
      },
      names.begin() + 1, names.begin() + 4);

  for (const auto& name : upper) std::cout << name << '\n';
  std::cout << "first name left alone: " << names.front() << '\n';
  return 0;
}
```

Output

```text
KOFI
EFUA
YAW
first name left alone: ama
```

Only the elements between the two iterators are visited; the rest of the container is untouched.

### Example 7

Example 7 runs the same transform sequentially and in parallel over two hundred thousand elements, and compares the results.

```cpp
// Running a slow transformation across all cores. The parallel result must match the sequential one
// exactly, so the program checks that before trusting the speedup. The sum uses integers, so the
// comparison is exact.
#include <pluss/tils/aux/transmit.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  std::vector<std::int64_t> inputs(200000);
  for (std::size_t i = 0; i < inputs.size(); ++i) inputs[i] = static_cast<std::int64_t>(i);

  const auto square_mod = [](std::int64_t x) { return (x * x) % 1000003; };

  const auto sequential = pluss::tils::transmit(square_mod, inputs);
  const auto parallel = pluss::tils::transmit(
      square_mod, inputs, pluss::tils::transmit_options{.exec = pluss::tils::transmit_exec::par,
                                                        .grain_size = 4096, .workers = 4});

  std::cout << "results: " << parallel.size() << '\n';
  std::cout << "parallel equals sequential: " << (parallel == sequential) << '\n';
  return 0;
}
```

Output

```text
results: 200000
parallel equals sequential: 1
```

The parallel result has the same length and the same values as the sequential one, in the same order.

### Example 8

Example 8 reads the default execution options.

```cpp
#include <pluss/tils/aux/transmit.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  const transmit_options defaults{};
  std::cout << "default exec is seq: " << (defaults.exec == transmit_exec::seq) << '\n';
  std::cout << "default grain size: " << defaults.grain_size << '\n';
  std::cout << "default workers (0 means hardware_concurrency): " << defaults.workers << '\n';
  return 0;
}
```

Output

```text
default exec is seq: true
default grain size: 4096
default workers (0 means hardware_concurrency): 0
```

The default is sequential execution, a grain size of 4096, and zero workers, which means the hardware's own concurrency.

### Example 9

Example 9 passes one value that is not a range.

```cpp
// transmit also works on one value that is not a range. It calls the function on the value and
// returns the result, so the same call reads the same whether the input is one value or many.
// A string is a range of characters, so it takes the range form instead; see the pitfalls.
#include <pluss/tils/aux/transmit.h>

#include <iostream>

struct Invoice {
  int items;
  double subtotal;
};

int main() {
  const Invoice invoice{3, 120.0};
  const double with_tax = pluss::tils::transmit(
      [](const Invoice& i) { return i.subtotal * 1.2; }, invoice);
  std::cout << "subtotal with tax: " << with_tax << '\n';

  const int doubled = pluss::tils::transmit([](int x) { return x * 2; }, 21);
  std::cout << "doubled: " << doubled << '\n';
  return 0;
}
```

Output

```text
subtotal with tax: 144
doubled: 42
```

The callable runs once, on the value itself, and the result is returned directly rather than wrapped in a vector.

## See Also

- [`map_axis`](core-map-axis.md) -- the same idea applied along one axis of a table.
