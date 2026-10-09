# `pluss::tils::map_axis`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/bi/specs.h>

#include <pluss/tils/aux/translate.h>

template <class F> auto map_axis(F&& fn, const Series& x, std::int64_t axis, transmit_options opts = {});
template <class F> auto map_axis(F&& fn, const Note& x, std::int64_t axis, transmit_options opts = {});
template <class F> auto map_axis(F&& fn, const codeh::Timeline& x, std::int64_t axis, transmit_options opts = {});
```

`map_axis` is `transmit` for one axis of a table. It cuts a `Series`, `Note`, or `Timeline` into one-lower-rank slices along the given axis, applies the callable to each slice, and collects the results into a `std::vector`, in the order the slices were cut. Along the last axis, each slice is a row; along the first axis, each slice is a column. A negative axis counts from the end, so `-1` is the last axis. An axis the table does not have is refused.

A one-dimensional table has a single axis, so it produces exactly one slice, holding the whole table. The callable may take the slice alone, or the slice and its zero-based position, the same rule `transmit` follows. The `transmit_options` argument runs the slices in parallel the same way the range form of `transmit` does.

## Parameters

| Parameter | Meaning |
|---|---|
| `fn` | The callable. It takes the slice, or the slice and its position. |
| `x` | The table to cut: a `Series`, a `Note`, or a `Timeline`. |
| `axis` | The axis to cut along. Negative counts from the end. |
| `opts` | Execution options, the same as `transmit`'s. |

## Returns

A `std::vector` of the callable's results, one per slice, in the order the slices were cut.

## Errors

An axis outside the table's rank is refused, with a message that names the operation.

## Practical Examples

### Example 1

Example 1 sums each row of a small table of numbers.

```cpp
// Summing each row of a small table. The table holds three weeks of sales for two stores. map_axis
// hands the function one row at a time, so the function sees a single store's week as a vector.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto sales = pluss::make_series(std::vector<double>{10, 20, 30, 5, 15, 25}, {2, 3}, cfg);

  const auto totals = pluss::tils::map_axis(
      [](const pluss::Series& row) {
        double sum = 0.0;
        for (double v : row.to_vec<double>()) sum += v;
        return sum;
      },
      sales, 1);

  for (std::size_t store = 0; store < totals.size(); ++store) {
    std::cout << "store " << store + 1 << " total: " << totals[store] << '\n';
  }
  return 0;
}
```

Output

```text
store 1 total: 60
store 2 total: 45
```

Each result is the sum of one row, in row order.

### Example 2

Example 2 cuts the same table two ways, by its last axis and by its first.

```cpp
// The same table can be cut two ways. Along the last axis each slice is a row, and along the first
// axis each slice is a column. A negative axis counts from the end, so -1 is the last axis.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <vector>

double sum_of(const pluss::Series& slice) {
  double total = 0.0;
  for (double v : slice.to_vec<double>()) total += v;
  return total;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto table = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}, cfg);

  const auto row_sums = pluss::tils::map_axis([](const pluss::Series& s) { return sum_of(s); }, table, -1);
  const auto column_sums = pluss::tils::map_axis([](const pluss::Series& s) { return sum_of(s); }, table, 0);

  std::cout << "row sums (axis -1):";
  for (double v : row_sums) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "column sums (axis 0):";
  for (double v : column_sums) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
```

Output

```text
row sums (axis -1): 6 15
column sums (axis 0): 5 7 9
```

The last axis gives the row sums, and the first axis gives the column sums; a negative axis and the equivalent positive one cut the table the same way.

### Example 3

Example 3 uses a callable that takes the slice's position as a second argument.

```cpp
// Labelling each slice by its position. When the function takes a second argument, map_axis passes the
// position of the slice, so the output can say which row it describes.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto readings = pluss::make_series(std::vector<double>{20.5, 21.0, 19.5, 30.0, 31.5, 29.0}, {3, 2}, cfg);

  const auto averages = pluss::tils::map_axis(
      [](const pluss::Series& slice, std::size_t position) {
        const auto values = slice.to_vec<double>();
        const double mean = (values[0] + values[1]) / 2.0;
        std::ostringstream line;
        line << "sensor " << position + 1 << " average " << mean;
        return line.str();
      },
      readings, 1);

  for (const auto& line : averages) std::cout << line << '\n';
  return 0;
}
```

Output

```text
sensor 1 average 20.75
sensor 2 average 24.75
sensor 3 average 30.25
```

Each line names the position of the slice it describes.

### Example 4

Example 4 cuts a table of text into rows.

```cpp
// Working with text. A grid of region names is cut into rows, and each row is a note of its own, so a
// function can count or join the names that belong together.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const auto regions = pluss::make_note(
      std::vector<std::string>{"north", "south", "east", "west"}, {2, 2});

  const auto counts = pluss::tils::map_axis(
      [](const pluss::Note& row) { return static_cast<std::int64_t>(row.numel()); }, regions, 1);

  for (std::size_t r = 0; r < counts.size(); ++r) {
    std::cout << "row " << r << " holds " << counts[r] << " names\n";
  }
  return 0;
}
```

Output

```text
row 0 holds 2 names
row 1 holds 2 names
```

Each slice is a Note, and the callable can use the Note's own count.

### Example 5

Example 5 cuts a one-dimensional timeline.

```cpp
// A one-dimensional timeline has a single slice along its only axis, which holds every tick. The
// function receives all the timestamps at once, so it can find the span they cover.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <vector>

int main() {
  const auto stamps = pluss::make_timeline({"2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04"});

  const auto counts = pluss::tils::map_axis(
      [](const pluss::Timeline& slice) { return static_cast<std::int64_t>(slice.numel()); }, stamps, 0);

  std::cout << "slices: " << counts.size() << '\n';
  std::cout << "timestamps in the slice: " << counts.front() << '\n';
  return 0;
}
```

Output

```text
slices: 1
timestamps in the slice: 4
```

There is exactly one slice, holding every timestamp, because a one-dimensional table has only one axis to cut along.

### Example 6

Example 6 sums many columns sequentially and in parallel, and compares the results.

```cpp
// Summing many columns at once across threads. The parallel result must match the sequential one,
// so the program compares the two before trusting the parallel answer.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <cstddef>
#include <iostream>
#include <vector>

int main() {
  const std::int64_t rows = 2000;
  const std::int64_t cols = 64;
  std::vector<double> values(static_cast<std::size_t>(rows * cols));
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i % 7);
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto table = pluss::make_series(values, {rows, cols}, cfg);

  const auto column_sum = [](const pluss::Series& s) {
    double total = 0.0;
    for (double v : s.to_vec<double>()) total += v;
    return total;
  };

  const auto sequential = pluss::tils::map_axis(column_sum, table, 0);
  const auto parallel = pluss::tils::map_axis(
      column_sum, table, 0,
      pluss::tils::transmit_options{.exec = pluss::tils::transmit_exec::par, .grain_size = 8, .workers = 4});

  std::cout << "column sums: " << parallel.size() << '\n';
  std::cout << "parallel equals sequential: " << (parallel == sequential) << '\n';
  return 0;
}
```

Output

```text
column sums: 64
parallel equals sequential: 1
```

Both runs produce the same column sums, in the same order.

### Example 7

Example 7 asks for an axis a two-dimensional table does not have.

```cpp
// Asking for an axis the table does not have. The table is two-dimensional, so axis 2 is refused, and
// the error names the operation so the caller can find the bad call.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto table = pluss::make_series(std::vector<double>{1, 2, 3, 4}, {2, 2}, cfg);
  try {
    (void)pluss::tils::map_axis([](const pluss::Series& s) { return s.numel(); }, table, 2);
    std::cout << "axis 2 accepted\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << "axis 2 refused, message names the axis: " << (message.find("axis out of range") != std::string::npos) << '\n';
  }
  return 0;
}
```

Output

```text
axis 2 refused, message names the axis: 1
```

The call is refused, and the error message names the axis that is out of range.

## See Also

- [`transmit`](core-transmit.md) -- the same idea over an ordinary range.
