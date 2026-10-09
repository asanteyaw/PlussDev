# PlussTils: Mapping with transmit

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

Most programs apply one operation to many values: scale every reading, label every row, price every line. `transmit` is the auxiliary utility for that job. It takes a callable and a range, applies the callable to each element, and returns the results. This chapter covers the forms it takes, the position argument, streaming output, pairing two ranges, iterator pairs, the parallel option, and the single-value form, with a complete program for each.

## The mental model

`transmit(fn, input)` reads the input once, in order, and collects one result per element. The input is never changed, and the output is a new vector whose element type is whatever `fn` returns. If `fn` takes a second argument, `transmit` passes the element's position as well. Two inputs are paired element by element and stop at the shorter one. A value that is not a range is passed to `fn` on its own.

The options object decides how the work is done. The default runs sequentially on one thread. A parallel run splits the input into blocks, runs each block on its own thread, and writes each result into its own slot, so the output order matches the input order.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Scaling each value to a share of the largest

Sensor readings are easier to compare as a share of the peak. This program finds the peak once, then passes a function that divides each reading by it to `transmit`.

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

Each reading is shown as a percentage of the peak, and the 48 that is the peak reads 100 percent. The original readings are still there afterwards, because `transmit` returns a new vector.

## Example 2: Numbering a list by position

A checklist needs numbers that depend on where each step sits. When the function takes a second argument, `transmit` passes the position, so this program numbers each step as it builds the label.

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

The position is zero-based inside the program, so the function adds one to show a human count. The labels come out in the order of the steps.

## Example 3: Writing results as they are produced

A long report should not be built in memory when it can be written as it goes. `transmit_into` writes each result to an output iterator. This program sends the formatted durations straight to standard output through an output stream iterator.

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

Each duration is written on its own line, converted from minutes to hours and minutes. No result vector was built, so a very long input would not need the memory for one.

## Example 4: Pairing two lists element by element

A price list and a quantity list are combined into line totals. This program passes both lists to `transmit`, so the function receives one price and one quantity at a time.

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

The third line total is 12.75, from a price of 4.25 and a quantity of three. The fourth quantity has no price, so the result has three lines, and the shorter list decides the length.

## Example 5: Changing only part of a container

Sometimes only a middle section of a list needs to change. This program passes two iterators, the start and the end, so `transmit` visits only those elements.

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

The three names in the middle are upper-cased, and the first name is untouched, because it lies outside the range the iterators marked.

## Example 6: A parallel run that matches the sequential one

A parallel run is only useful if it gives the same answer. This program runs the same transformation sequentially and in parallel over two hundred thousand integers, then compares the two results element by element.

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

The parallel result has the same number of values and equals the sequential result, so the speed-up does not change the answer.

## Example 7: Transforming one value that is not a range

A single value can go through the same call as a list. This program transforms an invoice record and an integer, each with one function call.

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

The invoice's subtotal is multiplied by 1.2 to add tax, and 144 comes out. The integer 21 doubles to 42. A string would take the range form instead, because it is a range of characters.

## Example 8: A small pipeline of transforms

A batch of order lines is read as text. This program parses each line into a quantity and a unit price, prices the lines in a second pass, and sums them. Each stage is one `transmit` call, so the stages are easy to read and to test separately.

```cpp
// A small pipeline for a batch of order lines. Each line is parsed into a quantity and a unit price,
// the lines are priced in one pass, and the totals are summed. Each stage is one transmit call.
#include <pluss/tils/aux/transmit.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct Line {
  int quantity;
  double unit_price;
};

int main() {
  const std::vector<std::string> raw{"3 x 4.50", "1 x 19.99", "12 x 0.75"};

  const auto parsed = pluss::tils::transmit(
      [](const std::string& text) {
        std::istringstream in(text);
        Line line{};
        std::string separator;
        in >> line.quantity >> separator >> line.unit_price;
        return line;
      },
      raw);

  const auto totals = pluss::tils::transmit(
      [](const Line& line) { return line.quantity * line.unit_price; }, parsed);

  double grand_total = 0.0;
  for (double t : totals) grand_total += t;

  std::cout << std::fixed << std::setprecision(2);
  for (std::size_t i = 0; i < totals.size(); ++i) {
    std::cout << raw[i] << " -> " << totals[i] << '\n';
  }
  std::cout << "grand total: " << grand_total << '\n';
  return 0;
}
```

Output

```text
3 x 4.50 -> 13.50
1 x 19.99 -> 19.99
12 x 0.75 -> 9.00
grand total: 42.49
```

The three lines are priced at 13.50, 19.99, and 9.00, and the grand total is 42.49. Each stage keeps its own vector, so you can print or check any stage.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Keep one result per input value | `transmit(fn, input)` | The output is a new vector in the input's order. |
| Write results without storing them | `transmit_into` with an output iterator | Use it when the output is long or goes to another destination. |
| Use the position of each element | a function that takes `(element, index)` | The position is zero-based. |
| Pair two sequences | `transmit(fn, a, b)` | The shorter input decides the length. |
| Work on part of a container | `transmit(fn, first, last)` | Only the elements between the iterators are visited. |
| Use every core on a long input | `transmit_options` with `transmit_exec::par` | Check the result against the sequential run before you trust it. |

## Pitfalls

**Expecting a string to be one value.** A string is a range of characters, so `transmit` maps each character. t07 shows the single-value form for a non-range value.

**Expecting the shorter input to be an error.** t04 shows the result stops at the shorter list, so a missing price or quantity disappears silently. Check the lengths when they should match.

**Expecting the input to change.** t01 shows the original readings are unchanged after the call.

## Where next

- [Mapping along an axis with translate](tils-aux-translate.md) -- the same idea applied to one axis of a table.
- [Scalar casts](tils-aux-scalar-casts.md) -- converting the values that a map produces.
- [Writing output](tils-output.md) -- printing the results.
- [`transmit` reference](../api/core-transmit.md) -- every call form, with its own examples.
