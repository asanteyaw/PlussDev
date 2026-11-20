<!-- generated from missing_nan_and_inf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Missing, NaN and infinity

[Back to PlussMic by example](../guide.md)

A numeric cell can fail to be an ordinary number in three different ways, and PlussMic keeps the three apart on purpose. A cell can be **missing** (nobody recorded a value), it can be **NaN** (a value that is not a number, such as `0 / 0`), or it can be **infinite** (a value that is unbounded, such as `5 / 0`). They look alike on a quick read and they need different remedies, so every question you can ask about them has its own name, and no name quietly means two of them. This page defines the three, shows where they come from, and walks through every function that asks about them, removes them or replaces them, on every PlussMic type.

## How it works

A cell is always exactly one of five things:

| Kind | What it means | How it prints | Asked with |
|---|---|---|---|
| ordinary number | a normal value | `1.5` | none of the questions below is true |
| `NULL` | no value was recorded: the validity mask marks the cell missing | `NULL` | `isnull`, `notnull`, `dropnull`, `fillnull` |
| `NaN` | a value, but "not a number": `0 / 0`, `x % 0`, `sqrt(-1)` | `NaN` | `isnan`, `notnan`, `dropnan`, `fillnan` |
| `+inf` | a value, unbounded above: `5 / 0`, `log(0)`, overflow | `inf` | `isinf`, `isposinf`, `dropinf`, `dropposinf`, `fillinf`, `fillposinf` |
| `-inf` | a value, unbounded below: `-5 / 0` | `-inf` | `isinf`, `isneginf`, `dropinf`, `dropneginf`, `fillinf`, `fillneginf` |

Three rules make the whole page predictable:

1. **`NULL` is not a number, so it is never `NaN` and never infinite.** A missing cell has no value, so `isnan` and `isinf` are `false` for it, and the `not...` forms (`notnan`, `notinf`) are `true`. `NaN` and the infinities, on the other hand, **are** values: `isnull` is `false` for them, and `dropnull` and `fillnull` leave them alone.
2. **`invalid` is the union.** `invalid()` is `true` for `NULL`, `NaN` and both infinities: everything that is not an ordinary number. `dropinvalid()` and `fillinvalid(value)` are its partners.
3. **Data that cannot hold `NaN` or an infinity never has any.** Integers, booleans, text, timestamps and durations are not floating point, so `isnan` and `isinf` are `false` for every one of their cells, with no error and no conversion. Only a missing cell can be non-ordinary in such a column.

The next program builds all five kinds in one column from a situation that produces them naturally: a spending-to-balance ratio over seven accounts. One account has no balance on record, three have a zero balance, and the rest are ordinary.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  // The balance of the fifth account was never recorded: the validity mask marks it missing (the 0.0 under it is only a placeholder).
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));

  const auto ratio = spend / balance;
  DataFrame accounts({{"balance", balance}, {"spend", spend}, {"ratio", ratio}});
  std::cout << accounts << "\n";
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
```
The `ratio` column is the division of `spend` by `balance`. Row 4 has no balance on record, so its ratio is `NULL`: arithmetic on a missing cell gives a missing cell. Rows 1, 3 and 6 divide by a zero balance and give three different answers: `0 / 0` is `NaN`, `120 / 0` is `inf` and `-75 / 0` is `-inf`. Rows 0, 2 and 5 are ordinary. The printout spells all four non-ordinary cells differently, and that spelling is the same on every PlussMic type.

## Where NaN and infinity come from

Division by zero is the usual source, and the rule is the rule of floating-point arithmetic. A positive number divided by zero is `+inf`, a negative one is `-inf`, and zero divided by zero is `NaN`. The remainder by zero (`x % 0`) is `NaN` for every `x`. Nothing raises an error: a computation that cannot give a number gives `NaN` or an infinity and carries on, and you find out afterwards with the questions below.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  // Floating point: x / 0 is +inf for a positive x, -inf for a negative x, and NaN for 0 / 0.
  const auto top = pluss::make_series({6.0, -6.0, 0.0, 6.0});
  const auto bottom = pluss::make_series({0.0, 0.0, 0.0, 3.0});
  std::cout << "float   x / y   " << (top / bottom) << "\n";

  // Integers: with no zero divisor the answer stays an integer ...
  const auto count = pluss::make_series(std::vector<std::int64_t>{7, -7, 0, 5});
  const auto by = pluss::make_series(std::vector<std::int64_t>{2, 2, 2, 5});
  std::cout << "int     x // y  " << count.floor_divide(by) << "\n";

  // ... and with a zero divisor the same call returns +inf, -inf and NaN instead of raising an error, so the result is a float Series.
  const auto zero_by = pluss::make_series(std::vector<std::int64_t>{0, 0, 0, 3});
  std::cout << "int     x // y  " << count.floor_divide(zero_by) << "\n";

  // The remainder by zero is NaN for every dividend.
  std::cout << "int     x % y   " << count.remainder(zero_by) << "\n";
}
```

Output

```text
float   x / y   pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[inf, -inf, nan, 2]
int     x // y  pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[3, -4, 0, 1]
int     x // y  pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[inf, -inf, nan, 1]
int     x % y   pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[nan, nan, nan, 2]
```
The first line is ordinary floating-point division: `6/0` is `inf`, `-6/0` is `-inf`, `0/0` is `nan`, and `6/3` is `2`. The second line divides integers with no zero divisor, and the result stays an integer (`3, -4, 0, 1`, rounded down). The third line repeats it with zero divisors: the same call now gives `inf`, `-inf` and `nan` in the first three places, and the result has become a floating-point Series so that it can hold them. The dtype change happens only when a zero divisor is present, so integer code that never divides by zero is untouched. The last line shows the remainder: `nan` wherever the divisor is zero.

A bare `Series` has no `NULL`, because it has no validity mask: only `NaN` and the infinities apply to it, and it has `isnan`, `isinf`, `isposinf`, `isneginf` and `isfinite`. `NULL` begins one level up, in a `Shelf`, and the tables and stacks built on shelves have all five kinds. Printing a raw `double` shows `nan`, `inf` and `-inf` in lower case; every table and column type prints `NaN`, `inf`, `-inf` and `NULL`.

## Asking about each kind

The next program puts every question next to the column it is about, so the table below is not a claim: it is the program's own output.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto ratio = spend / balance;

  // Every column below is the answer to one question about the `ratio` column.
  DataFrame questions({
      {"ratio", ratio},
      {"isnull", ratio.isnull()},
      {"isnan", ratio.isnan()},
      {"isinf", ratio.isinf()},
      {"isposinf", ratio.isposinf()},
      {"isneginf", ratio.isneginf()},
      {"invalid", ratio.invalid()},
  });
  std::cout << questions << "\n";

  // The complements: notnan is true for NULL (a missing cell is not a NaN) and notinf is true for NaN.
  DataFrame complements({{"ratio", ratio}, {"notnull", ratio.notnull()}, {"notnan", ratio.notnan()}, {"notinf", ratio.notinf()}});
  std::cout << complements << "\n";
}
```

Output

```text
===============================================================
     | ratio  isnull  isnan  isinf  isposinf  isneginf  invalid
---------------------------------------------------------------
0    |  0.25  false   false  false  false     false     false  
1    |  NaN   false   true   false  false     false     true   
2    |  0.2   false   false  false  false     false     false  
3    |  inf   false   false  true   true      false     true   
4    | NULL   true    false  false  false     false     true   
5    |  0.5   false   false  false  false     false     false  
6    | -inf   false   false  true   false     true      true   
---------------------------------------------------------------
[7 cs x 7 feat]
=====================================
     | ratio  notnull  notnan  notinf
-------------------------------------
0    |  0.25  true     true    true  
1    |  NaN   true     false   true  
2    |  0.2   true     true    true  
3    |  inf   true     true    false 
4    | NULL   false    true    true  
5    |  0.5   true     true    true  
6    | -inf   true     true    false 
-------------------------------------
[7 cs x 4 feat]
```
Read the first table one row at a time. Row 0 is ordinary: every answer is `false`. Row 1 is `NaN`: only `isnan` and `invalid` are `true`. Row 3 is `+inf`: `isinf`, `isposinf` and `invalid` are `true`, but `isneginf` is not. Row 6 is `-inf`: the mirror image. Row 4 is `NULL`: only `isnull` and `invalid` are `true`, and in particular `isnan` and `isinf` are `false`, because a missing cell holds no value to be `NaN`. The second table shows the complements. `notnan` is `true` for the `NULL` row, and `notinf` is `true` for the `NaN` row: each says only "this is not that".

Here is the whole family, and it is the same on every type:

| Question | Flag the cells | Remove them | Replace them |
|---|---|---|---|
| missing | `isnull`, `notnull` | `dropnull` | `fillnull(value)` |
| `NaN` | `isnan`, `notnan` | `dropnan` | `fillnan(value)` |
| `+inf` or `-inf` | `isinf`, `notinf` | `dropinf` | `fillinf(value)` |
| `+inf` only | `isposinf` | `dropposinf` | `fillposinf(value)` |
| `-inf` only | `isneginf` | `dropneginf` | `fillneginf(value)` |
| any of the above | `invalid` | `dropinvalid` | `fillinvalid(value)` |

## Removing the cells that you do not want

Each drop removes one kind and leaves everything else exactly where it was. A table has more than one feature, so the program below adds a second ratio (spend over income) to show the rule that decides which rows go.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto income = Shelf(pluss::make_series({2000.0, 0.0, 900.0, 800.0, 500.0, 0.0, 600.0}));
  DataFrame accounts({{"vs_balance", spend / balance}, {"vs_income", spend / income}});
  std::cout << "accounts\n" << accounts << "\n";

  // Each drop removes one kind of row and leaves the others where they are.
  std::cout << "dropnull\n" << accounts.dropnull() << "\n";
  std::cout << "dropnan\n" << accounts.dropnan() << "\n";
  std::cout << "dropinf\n" << accounts.dropinf() << "\n";
  std::cout << "dropinvalid\n" << accounts.dropinvalid() << "\n";

  // dropposinf / dropneginf remove one sign of infinity.
  std::cout << "dropposinf\n" << accounts.dropposinf() << "\n";

  // The default drops a row when ANY selected feature holds the kind; all = true only when EVERY selected feature does.
  std::cout << "dropnan({\"vs_balance\", \"vs_income\"}, true)\n" << accounts.dropnan({"vs_balance", "vs_income"}, true) << "\n";
}
```

Output

```text
accounts
============================
     | vs_balance  vs_income
----------------------------
0    |  0.25        0.15    
1    |  NaN         NaN     
2    |  0.2         0.1     
3    |  inf         0.15    
4    | NULL         0.12    
5    |  0.5         inf     
6    | -inf        -0.125   
----------------------------
[7 cs x 2 feat]
dropnull
============================
     | vs_balance  vs_income
----------------------------
0    |  0.25        0.15    
1    |  NaN         NaN     
2    |  0.2         0.1     
3    |  inf         0.15    
5    |  0.5         inf     
6    | -inf        -0.125   
----------------------------
[6 cs x 2 feat]
dropnan
============================
     | vs_balance  vs_income
----------------------------
0    |  0.25        0.15    
2    |  0.2         0.1     
3    |  inf         0.15    
4    | NULL         0.12    
5    |  0.5         inf     
6    | -inf        -0.125   
----------------------------
[6 cs x 2 feat]
dropinf
============================
     | vs_balance  vs_income
----------------------------
0    | 0.25        0.15     
1    | NaN         NaN      
2    | 0.2         0.1      
4    | NULL        0.12     
----------------------------
[4 cs x 2 feat]
dropinvalid
============================
     | vs_balance  vs_income
----------------------------
0    | 0.25        0.15     
2    | 0.2         0.1      
----------------------------
[2 cs x 2 feat]
dropposinf
============================
     | vs_balance  vs_income
----------------------------
0    |  0.25        0.15    
1    |  NaN         NaN     
2    |  0.2         0.1     
4    | NULL         0.12    
6    | -inf        -0.125   
----------------------------
[5 cs x 2 feat]
dropnan({"vs_balance", "vs_income"}, true)
============================
     | vs_balance  vs_income
----------------------------
0    |  0.25        0.15    
2    |  0.2         0.1     
3    |  inf         0.15    
4    | NULL         0.12    
5    |  0.5         inf     
6    | -inf        -0.125   
----------------------------
[6 cs x 2 feat]
```
Start from the `accounts` table: `vs_balance` holds all four kinds and `vs_income` holds a `NaN` (row 1), an `inf` (row 5) and a `-0.125`. Then each drop in turn. `dropnull` removes only row 4, the one with the missing balance, and the `NaN` and infinities stay. `dropnan` removes only row 1. `dropinf` removes rows 3, 5 and 6, which hold `inf` or `-inf` in either feature. `dropinvalid` removes every row that has any non-ordinary cell, leaving rows 0 and 2. `dropposinf` removes only the rows with `+inf` (rows 3 and 5) and keeps the `-inf` row.

The last call shows the rule for several features. **By default a row is removed when any of the selected features holds the kind.** With `all = true` a row is removed only when **every** selected feature holds it. Row 1 is `NaN` in both features, so it is the only row `dropnan({"vs_balance", "vs_income"}, true)` removes.

The same rule holds for the other two-dimensional types, with the nouns changed: a `Catalog` removes a position from **every** stream when **any** stream holds the kind there, which keeps the streams aligned; an `Atlas` removes a row from **every** frame when **any** frame holds the kind anywhere in that row, because an `Atlas` is a strict cube whose frames must stay the same height. A `Registry`'s fields are independent lists, not columns of one table, so each field is shortened on its own. The types section further down shows the numbers.

## Replacing the cells you do not want

A drop shortens the data. A fill keeps the shape and puts a value where the kind was.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto ratio = spend / balance;
  std::cout << "ratio\n" << ratio << "\n";

  // Each filler touches only its own kind; the others are left exactly as they were.
  std::cout << "fillnull(-9)\n" << ratio.fillnull(-9.0) << "\n";
  std::cout << "fillnan(0)\n" << ratio.fillnan(0.0) << "\n";
  std::cout << "fillinf(99)\n" << ratio.fillinf(99.0) << "\n";

  // A cap for +inf and a floor for -inf: one call each.
  std::cout << "fillposinf(10).fillneginf(-10)\n" << ratio.fillposinf(10.0).fillneginf(-10.0) << "\n";

  // A different value for every kind: chain the fillers.
  std::cout << "one value per kind\n" << ratio.fillnull(-9.0).fillnan(0.0).fillposinf(10.0).fillneginf(-10.0) << "\n";

  // The all-in-one: the SAME value for every kind. Use it when the kinds need no distinction.
  std::cout << "fillinvalid(0)\n" << ratio.fillinvalid(0.0) << "\n";
}
```

Output

```text
ratio
0  0.25
1  NaN
2  0.2
3  inf
4  NULL
5  0.5
6  -inf
ptype: Series

fillnull(-9)
0  0.25
1  NaN
2  0.2
3  inf
4  -9
5  0.5
6  -inf
ptype: Series

fillnan(0)
0  0.25
1  0
2  0.2
3  inf
4  NULL
5  0.5
6  -inf
ptype: Series

fillinf(99)
0  0.25
1  NaN
2  0.2
3  99
4  NULL
5  0.5
6  99
ptype: Series

fillposinf(10).fillneginf(-10)
0  0.25
1  NaN
2  0.2
3  10
4  NULL
5  0.5
6  -10
ptype: Series

one value per kind
0  0.25
1  0
2  0.2
3  10
4  -9
5  0.5
6  -10
ptype: Series

fillinvalid(0)
0  0.25
1  0
2  0.2
3  0
4  0
5  0.5
6  0
ptype: Series
```
Each filler touches only its own kind. `fillnull(-9)` replaces the missing cell (row 4) and nothing else. `fillnan(0)` replaces only the `NaN`. `fillinf(99)` replaces both infinities with the same value. When the two infinities should become different values (a ceiling for `+inf` and a floor for `-inf`), `fillposinf(10).fillneginf(-10)` does it in two calls. `fillnull(-9).fillnan(0).fillposinf(10).fillneginf(-10)` gives every kind a value of its own, and that is usually the right shape for a repair: a missing reading, an invalid computation and an unbounded one are three different situations. `fillinvalid(0)` is the all-in-one that writes the **same** value into all of them; use it when the kinds need no distinction.

A fill never invents an answer for a different kind: `fillnan` does not touch the `NULL`, and `fillnull` does not touch the `NaN`. If you only want one thing repaired, only that thing changes.

## Counting and auditing

Before choosing a remedy, find out what you have. `count_kinds()` returns one row per feature with five counts that never overlap and add up to the total.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto income = Shelf(pluss::make_series({2000.0, 0.0, 900.0, 800.0, 500.0, 0.0, 600.0}));
  DataFrame accounts({{"balance", balance}, {"spend", spend}, {"vs_balance", spend / balance}, {"vs_income", spend / income}});

  // One call, one row per feature. The five counts never overlap and always add up to the total.
  std::cout << accounts.count_kinds() << "\n";

  // Read one kind across every feature: which of them contain NaN, and how many cells?
  std::cout << "NaN per feature\n" << accounts.count_kinds().features("nan") << "\n";

  // The three one-cell questions behind the report are available on their own.
  std::cout << "missing in balance: " << accounts.features("balance").nnull() << "\n";
  std::cout << "NaN in vs_balance:  " << accounts.features("vs_balance").nnan() << "\n";
  std::cout << "+inf in vs_income:  " << accounts.features("vs_income").nposinf() << "\n";
  std::cout << "-inf in vs_balance: " << accounts.features("vs_balance").nneginf() << "\n";
}
```

Output

```text
======================================================
           | null  nan   posinf  neginf  finite  total
------------------------------------------------------
balance    | 1     0     0       0       6       7    
spend      | 0     0     0       0       7       7    
vs_balance | 1     1     1       1       3       7    
vs_income  | 0     1     1       0       5       7    
------------------------------------------------------
[4 cs x 6 feat]
NaN per feature
   balance  0
     spend  0
vs_balance  1
 vs_income  1
Name: nan, ptype: Series

missing in balance: 1
NaN in vs_balance:  1
+inf in vs_income:  1
-inf in vs_balance: 1
```
`balance` has one missing cell and six ordinary ones; `spend` is clean. `vs_balance` has one cell of each non-ordinary kind (`null`, `nan`, `posinf`, `neginf`) and three ordinary ones. `vs_income` has one `NaN` and one `+inf`. The counts tell you the remedy: a non-zero `null` means values were never recorded (`fillnull`, `dropnull`); a non-zero `nan` means an invalid computation, so look for the cause before you decide (`fillnan`, `dropnan`); a non-zero `posinf` or `neginf` means an unbounded one (`fillposinf`, `fillneginf`, `dropinf`). The report is an ordinary table, so `features("nan")` pulls out one kind across every column. A single column answers the same questions directly: `nnull()`, `nnan()`, `nposinf()` and `nneginf()`.

## Statistics and comparisons

The statistics follow the kinds exactly. A missing cell is **skipped** (it holds no value). A `NaN` is a value, so it is **counted**, and every statistic that adds it up becomes `NaN`. An infinity propagates the way arithmetic says it should.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Shelf;

namespace {
double value(const pluss::table::ShelfValue& v) { return v.to_dbl(); }
}  // namespace

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const auto present = pluss::make_series(std::vector<bool>{true, true, true, false, true});

  // Ordinary numbers and one missing cell: the missing cell is skipped, so the mean is over the four that are there.
  const auto with_null = Shelf(pluss::make_series({2.0, 4.0, 6.0, 0.0, 8.0}), present);
  std::cout << "missing cell:   count " << value(with_null.count()) << "  mean " << value(with_null.mean()) << "  sum " << value(with_null.sum()) << "\n";

  // A NaN is a value, not a gap: it is counted, and every statistic that adds it up becomes NaN.
  const auto with_nan = Shelf(pluss::make_series({2.0, 4.0, nan, 0.0, 8.0}), present);
  std::cout << "NaN value:      count " << value(with_nan.count()) << "  mean " << value(with_nan.mean()) << "  sum " << value(with_nan.sum())
            << "  max " << value(with_nan.max()) << "\n";

  // An infinity is a value too, and it propagates the way arithmetic says it should.
  const auto with_inf = Shelf(pluss::make_series({2.0, 4.0, inf, 0.0, 8.0}), present);
  std::cout << "+inf value:     count " << value(with_inf.count()) << "  mean " << value(with_inf.mean()) << "  sum " << value(with_inf.sum())
            << "  max " << value(with_inf.max()) << "  min " << value(with_inf.min()) << "\n";

  // When a statistic should ignore the NaN, remove it first. dropnan keeps the infinities; dropinvalid removes everything that is not ordinary.
  std::cout << "after dropnan:  mean " << value(with_nan.dropnan().mean()) << "\n";
  std::cout << "inf, dropinf:   mean " << value(with_inf.dropinf().mean()) << "\n";
  std::cout << "inf, dropnan:   mean " << value(with_inf.dropnan().mean()) << "\n";
}
```

Output

```text
missing cell:   count 4  mean 5  sum 20
NaN value:      count 4  mean nan  sum nan  max nan
+inf value:     count 4  mean inf  sum inf  max inf  min 2
after dropnan:  mean 4.66667
inf, dropinf:   mean 4.66667
inf, dropnan:   mean inf
```
With one missing cell the mean is over the four values that are there (`5`, and the count is `4`). With a `NaN` in the same position the count is still `4`, but the mean, the sum and the maximum are all `nan`: one invalid computation makes the whole column's statistic invalid, which is the honest answer. With a `+inf` the sum, the mean and the maximum are `inf`, and the minimum is the ordinary `2`. When a statistic should ignore these cells, remove them first: `dropnan()` keeps the infinities, `dropinf()` keeps the `NaN`, and `dropinvalid()` leaves only ordinary numbers. (The first column above prints raw doubles, which is why `nan` is lower case.)

Comparisons follow the kinds too (a `NaN` is not equal to anything, and a comparison with a missing cell is missing); the section on mistakes below shows the results.

## Files and joins

A CSV file has no way to write a missing cell except a blank or a marker word, so the reader recognises the usual ones: a blank cell, `NA`, `null` and `nan` all load as **missing**. `inf` and `-inf` load as the infinities they say.

```cpp
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto path = std::filesystem::temp_directory_path() / "pluss_guide_nonfinite.csv";
  {
    std::ofstream file(path);
    // A blank cell, NA, null and nan are the usual ways a spreadsheet or a database export says "no value"; inf and -inf are real numbers.
    file << "id,reading\n1,1.5\n2,\n3,NA\n4,null\n5,nan\n6,inf\n7,-inf\n8,2.5\n";
  }
  auto loaded = DataFrame::load_csv(path.string());
  std::cout << loaded << "\n";
  std::cout << loaded.count_kinds() << "\n";

  // Writing: a missing cell is written as a blank, a NaN VALUE is written as the word NaN, infinities as inf and -inf.
  DataFrame computed({{"ratio", Shelf(pluss::make_series({0.5, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}),
                                      pluss::make_series(std::vector<bool>{true, true, true}))},
                      {"gap", Shelf(pluss::make_series({1.0, 0.0, 3.0}), pluss::make_series(std::vector<bool>{true, false, true}))}});
  computed.save_csv(path.string());
  std::ifstream written(path);
  std::cout << written.rdbuf() << "\n";
  std::filesystem::remove(path);
}
```

Output

```text
====================
     | id    reading
--------------------
0    | 1     1.5    
1    | 2     NULL   
2    | 3     NULL   
3    | 4     NULL   
4    | 5     NULL   
5    | 6     inf    
6    | 7     -inf   
7    | 8     2.5    
--------------------
[8 cs x 2 feat]
===================================================
        | null  nan   posinf  neginf  finite  total
---------------------------------------------------
id      | 0     0     0       0       8       8    
reading | 4     0     0       0       4       8    
---------------------------------------------------
[2 cs x 6 feat]
ratio,gap
0.5,1
NaN,
inf,3
```
The first file has a blank, `NA`, `null` and `nan` in the `reading` column, and all four load as `NULL`, which `count_kinds()` confirms (`null` is 4). `inf` and `-inf` arrive as real infinities. Writing goes the other way: a missing cell is written as a blank, a `NaN` **value** is written as the word `NaN`, and the infinities as `inf`. Because `nan` is one of the words the reader treats as "no value", a `NaN` written to a file is read back as missing. If a pipeline must keep the two apart across a file, repair them first (`fillnan` or `fillnull`) so that the file holds a number or a blank, never the word `NaN`.

Join keys follow the same split. A missing key matches nothing, not even another missing key. A `NaN` key is a value, so it matches another `NaN` key.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  // Left keys: 1, NaN, 3 and a missing key (the 0.0 under the mask is only a placeholder).
  Catalog left({"k"_k = Stream(pluss::make_series({1.0, nan, 3.0, 0.0}), pluss::make_series(std::vector<bool>{true, true, true, false})),
                "x"_k = Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0}))});
  // Right keys: NaN, 3 and a missing key.
  Catalog right({"k"_k = Stream(pluss::make_series({nan, 3.0, 0.0}), pluss::make_series(std::vector<bool>{true, true, false})),
                 "y"_k = Stream(pluss::make_series({1.0, 2.0, 3.0}))});

  pluss::kindle::CatalogJoinSpec spec;
  spec.how = pluss::relational::JoinHow::Inner;
  spec.left_keys = {"k"};
  spec.right_keys = {"k"};
  auto joined = left.join(right, spec);
  std::cout << "x\n" << joined.keys("x") << "\n" << "y\n" << joined.keys("y") << "\n";
}
```

Output

```text
x
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 20                                                                                                       
1            | 30                                                                                                       
========================================================================================================================

y
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
========================================================================================================================
```
The left side has keys `1`, `NaN`, `3` and a missing key; the right side has `NaN`, `3` and a missing key. The join keeps the two records whose keys match: the `NaN` pair (`x = 20`, `y = 1`) and the `3` pair (`x = 30`, `y = 2`). The two missing keys, one on each side, do not match each other.

## Printing and PlussView

`head()`, `tail()`, `std::cout` and `to_string()` print the four kinds as `NULL`, `NaN`, `inf` and `-inf`, exactly as in every table above. A PlussView window (what `show()` opens) and the native web view print them in capitals, `NULL`, `NAN`, `INF` and `-INF`, so that they are easy to pick out in a long table, and they colour them differently: a `NULL` cell has a **red** background and a `NaN` or infinity cell has a **neutral** one, because a `NaN` is a value and not a gap. Sorting a column in PlussView puts `-INF` before every number, `INF` after every number, then `NAN`, and `NULL` last. The Summary tab counts `missing`, `nan` and `inf` separately. None of this changes the data: the stored values are real floating-point numbers, and a statistic or a join reads those, never the printed text.

## The same family on every type

Everything above was shown on a `DataFrame` and a `Shelf`, but the family is the same on every PlussMic type. A one-dimensional type holds the rule (`Shelf`, `Stream`, `Slot`), a two-dimensional type delegates to it (`DataFrame` to its shelves, `Catalog` to its streams, `Registry` to its slots), and a stack or a workspace delegates once more (`Atlas` and `Nexus` to their frames, `Archive` and `Locus` to their catalogs, `Axel` to its registries). The following program builds the same six cells in each type and applies three drops.

```cpp
#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>

using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::scribble::Axel;
using pluss::scribble::Registry;
using pluss::scribble::Slot;
using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Shelf;

namespace {
void row(const char* type, std::int64_t dropnan, std::int64_t dropinf, std::int64_t dropinvalid) {
  std::cout << std::left << std::setw(10) << type << std::right << std::setw(8) << dropnan << std::setw(8) << dropinf << std::setw(12)
            << dropinvalid << "\n";
}
}  // namespace

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
  const auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});

  // One dimension: the shelf holds the rule, a stream wraps a shelf, and a slot holds cells (an empty cell is NULL).
  Shelf shelf(values, present);
  Stream stream(values, present);
  Slot slot({pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}), pluss::make_series({-inf}), std::monostate{},
             pluss::make_series({5.0})});

  // Two dimensions delegate to the one-dimensional rule, and the stacks delegate once more.
  DataFrame frame({{"x", shelf}});
  Catalog catalog(std::vector<std::pair<std::string, Stream>>{{"x", stream}});
  Registry registry({{"x", slot}});
  Atlas atlas(std::vector<DataFrame>{frame, frame});
  Archive archive(std::vector<Catalog>{catalog, catalog});
  Axel axel(std::vector<Registry>{registry, registry});

  std::cout << std::left << std::setw(10) << "cells kept" << std::right << std::setw(8) << "dropnan" << std::setw(8) << "dropinf" << std::setw(12)
            << "dropinvalid" << "\n";
  row("Shelf", shelf.dropnan().size(), shelf.dropinf().size(), shelf.dropinvalid().size());
  row("Stream", stream.dropnan().size(), stream.dropinf().size(), stream.dropinvalid().size());
  row("Slot", slot.dropnan().size(), slot.dropinf().size(), slot.dropinvalid().size());
  row("DataFrame", frame.dropnan().ncases(), frame.dropinf().ncases(), frame.dropinvalid().ncases());
  row("Catalog", catalog.dropnan().keys("x").size(), catalog.dropinf().keys("x").size(), catalog.dropinvalid().keys("x").size());
  row("Registry", registry.dropnan().fields("x").size(), registry.dropinf().fields("x").size(), registry.dropinvalid().fields("x").size());
  row("Atlas", atlas.dropnan().frame(1).ncases(), atlas.dropinf().frame(1).ncases(), atlas.dropinvalid().frame(1).ncases());
  row("Archive", archive.dropnan().catalog(1).keys("x").size(), archive.dropinf().catalog(1).keys("x").size(),
      archive.dropinvalid().catalog(1).keys("x").size());
  row("Axel", axel.dropnan().registry(1).fields("x").size(), axel.dropinf().registry(1).fields("x").size(),
      axel.dropinvalid().registry(1).fields("x").size());
}
```

Output

```text
cells kept dropnan dropinf dropinvalid
Shelf            5       4           2
Stream           5       4           2
Slot             5       4           2
DataFrame        5       4           2
Catalog          5       4           2
Registry         5       4           2
Atlas            5       4           2
Archive          5       4           2
Axel             5       4           2
```
Every type keeps `5` cells after `dropnan` (the `NaN` goes), `4` after `dropinf` (both infinities go) and `2` after `dropinvalid` (everything but the ordinary `1` and `5` goes). The shape of each type adds a few details that matter in practice:

- **`Catalog`:** streams may differ in length. A drop removes a position from every stream when any stream holds the kind at that position, which keeps them aligned.
- **`Registry`:** fields are independent lists, so each field is processed on its own and may end up a different length. There is no alignment between fields.
- **`Slot`:** a slot cell that is a single number is treated exactly like a record of a `Stream`. A cell that holds a **list** of numbers is one cell: it is flagged, dropped or filled according to what is inside it, so `isnan` is `true` for a cell that contains a `NaN`, `dropnan` removes that cell, and `fillnan` replaces the `NaN` inside it. Text, timestamp and duration cells cannot hold either and are never touched.
- **`Atlas`:** a strict cube, so a drop is decided across the whole stack and removes the same rows from every frame.
- **`Archive`, `Axel`, `Nexus`, `Locus`:** each member is processed independently. In `Nexus` and `Locus` the relationships and table metadata are kept; relationships are by key, so removing rows does not undeclare them.

A `Timeline` has its own "not a time" (`NaT`). It is a different thing from `NaN`, it is asked about with `isnat`, and none of the functions on this page look at it.

## Mistakes the kinds make easy

Four mistakes come from treating the kinds as one. Each is shown below with what happens, and the call that was meant.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto ratio = spend / balance;
  std::cout << "ratio\n" << ratio << "\n";

  // 1. Testing for NaN with ==. A NaN is not equal to anything, itself included, so the comparison never finds it. A missing cell stays missing.
  std::cout << "ratio == NaN  (finds nothing)\n" << (ratio == nan) << "\n";
  std::cout << "ratio.isnan()  (finds it)\n" << ratio.isnan() << "\n";

  // 2. dropnull() to remove "the bad rows". It removes the missing cell only; the NaN and the infinities are values and stay.
  std::cout << "dropnull() keeps " << ratio.dropnull().size() << " of " << ratio.size() << " cells; NaN still present: " << ratio.dropnull().nnan()
            << "\n";

  // 3. fillnull(0) to repair the column. It fills the missing cell and leaves the NaN and both infinities exactly as they were.
  const auto repaired = ratio.fillnull(0.0);
  std::cout << "after fillnull(0): missing " << repaired.nnull() << ", NaN " << repaired.nnan() << ", infinite " << repaired.nposinf() + repaired.nneginf()
            << "\n\n";

  // 4. A mean over a column that holds a NaN. The missing cell is skipped, the NaN is not, so one invalid division makes the whole mean NaN.
  const auto finite_only = ratio.dropnan().dropinf();
  std::cout << "mean(ratio)                      " << ratio.mean().to_dbl() << "\n";
  std::cout << "mean(ratio.dropinvalid())        " << ratio.dropinvalid().mean().to_dbl() << "\n";
  std::cout << "mean(ratio.dropnan().dropinf())  " << finite_only.mean().to_dbl() << "\n\n";

  // Comparisons follow the kinds too: inf > 0.4 is true, NaN > 0.4 is false, and a missing cell gives a missing answer (a query drops it).
  std::cout << "ratio > 0.4\n" << (ratio > 0.4) << "\n";
  DataFrame accounts({{"ratio", ratio}});
  std::cout << accounts.query("ratio > 0.4") << "\n";
}
```

Output

```text
ratio
0  0.25
1  NaN
2  0.2
3  inf
4  NULL
5  0.5
6  -inf
ptype: Series

ratio == NaN  (finds nothing)
0  false
1  false
2  false
3  false
4  NULL
5  false
6  false
ptype: Series

ratio.isnan()  (finds it)
0  false
1  true
2  false
3  false
4  false
5  false
6  false
ptype: Series

dropnull() keeps 6 of 7 cells; NaN still present: 1
after fillnull(0): missing 0, NaN 1, infinite 2

mean(ratio)                      nan
mean(ratio.dropinvalid())        0.316667
mean(ratio.dropnan().dropinf())  0.316667

ratio > 0.4
0  false
1  false
2  false
3  true
4  NULL
5  true
6  false
ptype: Series

============
     | ratio
------------
3    | inf  
5    | 0.5  
------------
[2 cs x 1 feat]
```
1. **Testing for `NaN` with `==`.** A `NaN` is not equal to anything, not even itself, so `ratio == nan` finds nothing (every answer is `false`, and the missing cell stays missing). Ask `isnan()`.
2. **`dropnull()` to remove "the bad rows".** It removes the missing cell and only that: 6 of 7 cells remain and the `NaN` is still there. Use `dropnan()`, `dropinf()` or `dropinvalid()` for the others.
3. **`fillnull(0)` to repair the column.** It fills the missing cell and leaves the `NaN` and both infinities exactly as they were (missing 0, NaN 1, infinite 2).
4. **A mean over a column holding a `NaN`.** The mean is `nan`. Drop the `NaN` first, or all three kinds with `dropinvalid()`; both give `0.316667` here.

The last two tables show comparisons. `inf > 0.4` is `true`, `NaN > 0.4` is `false`, `-inf > 0.4` is `false`, and the missing cell gives a missing answer, which is why the query above keeps exactly rows 3 and 5.

## Decisions

- **Ask which kind it is before repairing it.** `count_kinds()` is one call, and `null`, `nan`, `posinf` and `neginf` have different causes.
- **Use the single-kind methods when the kinds need different treatment,** and chain them: `fillnull(a).fillnan(b).fillposinf(c).fillneginf(d)`.
- **Use `invalid`, `dropinvalid` and `fillinvalid` only when the kinds need no distinction:** "keep only ordinary numbers" or "make everything this one value".
- **Test for `NaN` with `isnan`,** never with `==`.
- **For a statistic that should ignore bad cells,** drop them first (`dropnan`, or `dropinvalid`); a missing cell is skipped on its own, but a `NaN` and an infinity are not.
- **Remember that a file cannot tell `NaN` from missing:** the word `nan` loads as missing. Repair before saving if the difference matters.

## Where next

Reference pages for the whole family, one page per method and per type: [`isnan`](../api/shelf/isnan.md), [`isinf`](../api/shelf/isinf.md), [`isposinf`](../api/shelf/isposinf.md), [`isneginf`](../api/shelf/isneginf.md), [`dropnan`](../api/shelf/dropnan.md), [`dropinf`](../api/shelf/dropinf.md), [`fillnan`](../api/shelf/fillnan.md), [`fillinf`](../api/shelf/fillinf.md), [`dropinvalid`](../api/shelf/dropinvalid.md), [`fillinvalid`](../api/shelf/fillinvalid.md), [`count_kinds`](../api/dataframe/count_kinds.md), [`invalid`](../api/shelf/invalid.md), [`isnull`](../api/shelf/isnull.md), [`dropnull`](../api/shelf/dropnull.md) and [`fillnull`](../api/shelf/fillnull.md). Next: [Reports and joins](reports_and_joins.md).
