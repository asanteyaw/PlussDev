# PlussTils: Mapping along an axis with translate

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A table has rows and columns, and many questions ask about one of them: the total of each row, the share of each column, the label for each slice. `map_axis` is the auxiliary utility for those questions. It cuts a Series, Note, or Timeline along one axis, passes each slice to a function, and collects the results. This chapter shows the axis rules, the index argument, the three value types, the parallel option, and an end-to-end task.

## The mental model

Choose an axis, and the table is cut into slices, one for each position along the other axes. Along the last axis, each slice is a row. Along the first axis, each slice is a column. A negative axis counts from the end, so -1 is the last axis. The function receives one slice at a time as a table of one lower rank, and the results come back in the order of the slices.

The same call works for three value types. A Series is cut into numeric slices, a Note into slices of text, and a Timeline into slices of timestamps. An axis that the table does not have is refused with an error that names the operation.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Summing each row of a table

A table of weekly sales has one row per store. This program cuts the table along its second axis, so each slice is one store's week, and sums each slice.

```cpp
// Summing each row of a small table. The table holds three weeks of sales for two stores. map_axis
// hands the function one row at a time, so the function sees a single store's week as a vector.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto sales = pluss::make_series(std::vector<double>{10, 20, 30, 5, 15, 25}, {2, 3}, cfg);

  const auto totals = pluss::tils::map_axis(
      [](const pluss::Series& row) {
        double sum = 0.0;
        for (double v : row.host<double>()) sum += v;
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

Store one totals 60 and store two totals 45. Each slice arrived as a vector, so the function could sum it with an ordinary loop.

## Example 2: Choosing the axis, including a negative one

The same table can be cut by rows or by columns. This program sums the rows with axis -1, which is the last axis, and the columns with axis 0.

```cpp
// The same table can be cut two ways. Along the last axis each slice is a row, and along the first
// axis each slice is a column. A negative axis counts from the end, so -1 is the last axis.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <iostream>
#include <vector>

double sum_of(const pluss::Series& slice) {
  double total = 0.0;
  for (double v : slice.host<double>()) total += v;
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

The row sums are 6 and 15. The column sums are 5, 7, and 9. A negative axis counts from the end, so -1 and 1 cut the same table here.

## Example 3: Labelling each slice by its position

Sensor readings are arranged one sensor per row. When the function takes a second argument, the position is passed too, so the output can name the sensor it describes.

```cpp
// Labelling each slice by its position. When the function takes a second argument, map_axis passes the
// position of the slice, so the output can say which row it describes.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto readings = pluss::make_series(std::vector<double>{20.5, 21.0, 19.5, 30.0, 31.5, 29.0}, {3, 2}, cfg);

  const auto averages = pluss::tils::map_axis(
      [](const pluss::Series& slice, std::size_t position) {
        const auto values = slice.host<double>();
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

Each sensor's average is printed with its position, sensor one first, and the averages are 20.75, 24.75, and 30.25.

## Example 4: Working with text rows

A grid of region names is cut into rows, and each row is a note. This program counts the names in each row, which is the kind of check a text table needs before it is joined.

```cpp
// Working with text. A grid of region names is cut into rows, and each row is a note of its own, so a
// function can count or join the names that belong together.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

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

Each row holds two names. The slice is a note, so the function can use the note's own count.

## Example 5: A one-dimensional timeline has one slice

A one-dimensional timeline has a single axis, so the only slice holds every timestamp. This program shows that the function receives all four at once.

```cpp
// A one-dimensional timeline has a single slice along its only axis, which holds every tick. The
// function receives all the timestamps at once, so it can find the span they cover.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

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

There is one slice, and it holds four timestamps. A one-dimensional table never splits into several slices along its only axis.

## Example 6: A parallel run that matches the sequential one

Summing sixty-four columns of two thousand rows is a good job for several threads. This program runs the same column sums sequentially and in parallel, then compares them.

```cpp
// Summing many columns at once across threads. The parallel result must match the sequential one,
// so the program compares the two before trusting the parallel answer.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

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
    for (double v : s.host<double>()) total += v;
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

Both runs give sixty-four column sums, and the two sets of sums are equal. The parallel run splits the columns between threads without changing the answer.

## Example 7: An axis the table does not have is refused

A table with two axes has no axis two. This program asks for one, and the error names the operation, so the caller can find the bad call.

```cpp
// Asking for an axis the table does not have. The table is two-dimensional, so axis 2 is refused, and
// the error names the operation so the caller can find the bad call.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

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

The call is refused, and the message names the axis that is out of range.

## Example 8: Shares of each column's total

Before regions are compared, each month should be turned into shares of that month's total, so a large month does not dominate. This program cuts the table by its columns and returns each column's shares.

```cpp
// Turning each column of a region-by-month table into shares of that month's total. This is a
// common step before comparing regions fairly, because a big month would otherwise dominate.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  // Rows are regions north, south, east. Columns are January, February.
  const auto revenue = pluss::make_series(std::vector<double>{60, 20, 40, 30, 20, 50}, {3, 2}, cfg);

  const auto shares = pluss::tils::map_axis(
      [](const pluss::Series& month) {
        const auto values = month.host<double>();
        double total = 0.0;
        for (double v : values) total += v;
        std::vector<double> out;
        for (double v : values) out.push_back(v / total);
        return out;
      },
      revenue, 0);

  std::cout << std::fixed << std::setprecision(2);
  const char* regions[] = {"north", "south", "east"};
  for (std::size_t r = 0; r < 3; ++r) {
    std::cout << regions[r] << ": january " << shares[0][r] << ", february " << shares[1][r] << '\n';
  }
  return 0;
}
```

Output

```text
north: january 0.50, february 0.20
south: january 0.33, february 0.30
east: january 0.17, february 0.50
```

In January, north holds half the revenue, south a third, and east a sixth. The shares of each month add to one, which is what makes the regions comparable.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Work on rows | `map_axis` with the last axis | Each slice is one row. |
| Work on columns | `map_axis` with axis 0 | Each slice is one column. |
| Label slices by position | a function that takes `(slice, index)` | The position is zero-based. |
| Use every core on a wide table | `transmit_options` with `transmit_exec::par` | Compare with the sequential run first. |

## Pitfalls

**Expecting an axis that does not exist to be ignored.** m07 shows the call is refused, with an error that names the operation.

**Expecting a negative axis to mean something other than the last axis.** m02 shows -1 and the last positive axis cut the same table.

**Expecting a one-dimensional table to split.** m05 shows its only axis gives a single slice.

## Where next

- [Mapping with transmit](tils-aux-transmit.md) -- the element-wise form.
- [Scalar casts](tils-aux-scalar-casts.md) -- converting the values a slice function returns.
- [`map_axis` reference](../api/core-map-axis.md) -- every value type, with its own examples.
