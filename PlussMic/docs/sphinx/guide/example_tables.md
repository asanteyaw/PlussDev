# Example tables

[Back to PlussMic by example](../guide.md)

A first program, a guide or a bug report needs a table to work on. PlussMic has five small ones built in. Each is made in memory from numbers written into the library or from a fixed formula, so it is the same on every machine and needs no file, no network and no licence.

## How it works

`pluss::table::examples()` lists the tables with a description and the size of each, and `pluss::table::example(name)` returns one as a `DataFrame`.

| Name | What it holds |
|---|---|
| `sales` | Units, revenue and returns for four cities over six months |
| `accounts` | Seven accounts with a missing balance, NaN, `inf` and `-inf`: all four kinds of cell |
| `sensors` | Two days of hourly readings: a `Timeline` column, and a six-hour gap in the humidity |
| `anscombe` | Anscombe's quartet: four pairs of columns with the same mean, variance, correlation and fitted line, and four different shapes |
| `messages` | Twelve short order and delivery-problem messages with a label |

A name that is not on the list is an error that shows the names that exist and suggests the closest.

## Signatures

```cpp
std::vector<pluss::table::ExampleInfo> pluss::table::examples()
pluss::table::DataFrame pluss::table::example(std::string_view name)
```

An `ExampleInfo` has the `name`, a description in `about`, and the number of `cases` and `features`.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: What there is

The program lists the tables and prints the first rows of one.

```cpp
// The built-in example tables.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  for (const auto& item : pluss::table::examples()) {
    std::cout << item.name << " (" << item.cases << " cases, " << item.features << " features): " << item.about << "\n";
  }
  std::cout << "\n" << pluss::table::example("anscombe").head(4) << "\n";
}
```

Output

```text
sales (24 cases, 5 features): Units, revenue and returns for four cities over six months (invented)
accounts (7 cases, 3 features): Seven accounts with a missing balance, NaN, inf and -inf (invented)
sensors (48 cases, 3 features): Two days of hourly readings with a gap in the humidity (invented)
anscombe (11 cases, 6 features): Anscombe's quartet: four datasets with the same statistics and different shapes
messages (12 cases, 3 features): Twelve short order and delivery-problem messages with a label (invented)

==========================================
     | x     y1    y2    y3     x4    y4  
------------------------------------------
0    | 10    8.04  9.14  7.46   8     6.58
1    | 8     6.95  8.14  6.77   8     5.76
2    | 13    7.58  8.74  12.74  8     7.71
3    | 9     8.81  8.77  7.11   8     8.84
------------------------------------------
[4 cs x 6 feat]
```

Anscombe's quartet is the classic reason to look at data and not only at its summary statistics: the four pairs have the same statistics, and plotting them shows four different pictures.

## Example 2: Every kind of cell, and a wrong name

The `accounts` table divides spending by balance, so it holds an ordinary number, a missing value, and the three ways a division can fail. The program prints it, then asks for a table that does not exist.

```cpp
// An example table with every kind of cell, and a misspelt name.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << pluss::table::example("accounts") << "\n";
  try {
    (void)pluss::table::example("salez");
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\n";
  }
}
```

Output

```text
============================
     | balance  spend  ratio
----------------------------
0    | 1200      300    0.25
1    | 0         0      NaN 
2    | 450       90     0.2 
3    | 0         120    inf 
4    | NULL      60    NULL 
5    | 80        40     0.5 
6    | 0        -75    -inf 
----------------------------
[7 cs x 3 feat]
pluss::table::example: no example table called 'salez' Did you mean 'sales'?
```

Row 4 has no balance on record, so its ratio is `NULL`. Rows 1, 3 and 6 divide by a zero balance and give `NaN`, `inf` and `-inf`. The [missing, NaN and infinity guide](missing_nan_and_inf.md) explains how to find, remove and fill each kind. The second error names the mistake and the table that was meant.
