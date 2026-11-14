<!-- generated from slates.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Slates: one typed column

[Back to PlussMic by example](../guide.md)

A `Slate` is the single column underneath every table: one typed array of numbers, text or times, with a validity mask for missing values and an optional label for each case. This page uses slates by themselves, for the jobs that need only one column: a gradebook, a column of survey answers, two measurements reconciled by name, and a time column turned into features.

## The mental model

A slate has a **kind** (numbers, text, timestamps or durations), a **validity mask** that records which entries are present, and optionally **case labels** that name the rows. Operations respect the mask: a reduction skips missing entries and an arithmetic result is missing wherever either input is. The numbers underneath, reached with `as_series()`, have no mask, and show a placeholder zero where a value is missing (the pitfall page in [Files, missing data and time](files_and_time.md) shows the consequence).

## A gradebook

Ten exam scores, two students absent. The program marks the absences as missing, summarises, finds quartile cut points and ranks, assigns grade bands and fills the absences with the class median.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Slate;

int main() {
  const double absent = std::numeric_limits<double>::quiet_NaN();
  // Ten exam scores; two students did not sit the exam. A Slate carries a validity mask next to its numbers: true = present.
  auto values = pluss::make_series({72.0, 85.5, absent, 91.0, 64.0, absent, 78.5, 55.0, 88.0, 69.5});
  Slate raw(values, values.isnan().logical_not());
  std::cout << raw.to_string() << "\n";
  std::cout << "present " << raw.notnull().sum().to_dbl() << " of " << raw.size() << "\n";

  // 1. Summary of the observed scores only.
  std::cout << raw.describe().to_string() << "\n";

  // 2. Quartile cut points and a rank for each student (missing students stay missing).
  std::cout << raw.percentiles({0.25, 0.5, 0.75}).to_string() << "\n";
  std::cout << raw.rank().to_string() << "\n";

  // 3. Grade bands from explicit edges: F below 60, D 60-70, C 70-80, B 80-90, A 90 and above.
  auto bands = raw.bucketize({0.0, 60.0, 70.0, 80.0, 90.0, 100.0});
  std::cout << bands.to_string() << "\n";

  // 4. Absent students sit a make-up exam and score the class median; the table records who was filled.
  auto filled = raw.fillnull(raw.median().to_dbl());
  std::cout << filled.to_string() << "\n";
}
```

Output

```text
0  72
1  85.5
2  NULL
3  91
4  64
5  NULL
6  78.5
7  55
8  88
9  69.5
ptype: Series
present 8 of 10
count  8
 mean  75.4375
  std  12.57104809
  min  55
  25%  68.125
  50%  75.25
  75%  86.125
  max  91
Name: value, ptype: Series
0  68.125
1  75.25
2  86.125
ptype: Series
0  4
1  6
2  NULL
3  8
4  2
5  NULL
6  5
7  1
8  7
9  3
ptype: Series
0  2
1  3
2  NULL
3  4
4  1
5  NULL
6  2
7  0
8  3
9  1
ptype: Series
0  72
1  85.5
2  75.25
3  91
4  64
5  75.25
6  78.5
7  55
8  88
9  69.5
ptype: Series
```
`present 8 of 10` counts the entries that are present. `describe` summarises only those eight: mean `75.44`, median `75.25`, range `55` to `91`. `percentiles` gives the quartile cut points `68.125`, `75.25` and `86.125`, and `rank` ranks the present scores while leaving the absent students `NULL`. `bucketize` with the edges `0, 60, 70, 80, 90, 100` returns the band index of each score: `0` is an F and `4` is an A, so the 91 is band `4` and the 55 band `0`. The fill uses `median()`, and it is `75.25`, the same value `describe` reports, because the median of an even number of values is the average of the two middle ones on every path.

## A column of answers

Eight free-text answers to a yes-or-no question, two blank. The `nt()` accessor holds the text operations; the program trims and lower-cases, classifies with a pattern, and counts.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Slate;

int main() {
  // Eight free-text answers to "Do you use the product daily?". Two are blank (missing).
  auto raw = pluss::make_note({"  YES ", "no", "Yes, mostly", "", "N/A", "yes", "", "Not really"});
  auto present = pluss::make_series({true, true, true, false, true, true, false, true});
  Slate answers(raw, present);
  std::cout << answers.to_string() << "\n";

  // 1. Normalise: trim, lower-case. The nt() accessor holds the text operations; missing values stay missing.
  auto clean = answers.nt().strip().nt().lower();
  std::cout << clean.to_string() << "\n";

  // 2. Classify with patterns: answers that start with "yes" and answers that start with "no" (not "n/a").
  auto yes = clean.nt().matches("yes");
  auto no = clean.nt().matches("no");
  std::cout << "yes?\n" << yes.to_string() << "\n";
  std::cout << "no?\n" << no.to_string() << "\n";

  // 3. How many of each, and what remains unclassified?
  std::cout << "yes " << yes.sum().to_dbl() << ", no " << no.sum().to_dbl() << ", missing " << clean.isnull().sum().to_dbl() << ", total " << clean.size() << "\n";

  // 4. The distinct normalised answers and how often each occurs.
  std::cout << clean.distinct().to_string() << "\n";
}
```

Output

```text
0    YES 
1  no
2  Yes, mostly
3  NULL
4  N/A
5  yes
6  NULL
7  Not really
ptype: Note
0  yes
1  no
2  yes, mostly
3  NULL
4  n/a
5  yes
6  NULL
7  not really
ptype: Note
yes?
0  true
1  false
2  true
3  NULL
4  false
5  true
6  NULL
7  false
ptype: Series
no?
0  false
1  true
2  false
3  NULL
4  false
5  false
6  NULL
7  true
ptype: Series
yes 3, no 2, missing 2, total 8
0  yes
1  no
2  yes, mostly
3  n/a
4  not really
ptype: Note
```
`strip` and `lower` leave the blanks `NULL`. `matches("yes")` and `matches("no")` test whether a cell *starts* with the pattern, so `yes, mostly` counts as a yes and `not really` counts as a no, while `n/a` is neither: three yes, two no, two missing, and one answer unclassified. That last line is why you print the counts that must add up to the total.

## Two measurements, matched by name

Today's readings are in the order the stations reported; yesterday's are in another order and include a station that has since closed. The program gives each slate its case labels, aligns yesterday to today by name, and subtracts.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::AlignBy;
using pluss::table::Slate;

static void show(const char *label, const Slate &s) { std::cout << label << "\n" << s.to_string() << "\n"; }

int main() {
  // Today's readings, in the order the stations reported.
  Slate today(pluss::make_series({14.2, 11.8, 15.0, 9.5}));
  auto today_by_name = today.label().set(Slate(pluss::make_note({"north", "east", "south", "hill"})));
  // Yesterday's readings, in another order, and without "hill" but with a station that has since closed.
  Slate yesterday(pluss::make_series({15.1, 13.0, 12.2, 10.0}));
  auto yesterday_by_name = yesterday.label().set(Slate(pluss::make_note({"south", "north", "east", "dock"})));
  show("today", today_by_name);
  show("yesterday", yesterday_by_name);

  // 1. Align yesterday to today's stations by NAME, not position. Stations that did not exist yesterday become NULL.
  pluss::table::AlignOptions by_name;
  by_name.align = AlignBy::Caselbl;
  auto aligned = yesterday_by_name.label().align(today_by_name.label().values(), by_name);
  show("yesterday, in today's order", aligned);

  // 2. The change per station is a plain subtraction now that the rows correspond; NULL propagates.
  auto change = today_by_name - aligned;
  show("change since yesterday", change);

  // 3. Which stations have no previous reading, and which of the old stations are gone?
  std::cout << "no previous reading: " << change.isnull().sum().to_dbl() << " station(s)\n";
  auto gone = yesterday_by_name.label().find(Slate(pluss::make_note({"dock"})));
  show("position of the closed station in yesterday's list", gone);

  // 4. Order by station name for a report.
  show("report, sorted by name", change.label().sort());
}
```

Output

```text
today
north  14.2
 east  11.8
south  15
 hill  9.5
ptype: Series
yesterday
south  15.1
north  13
 east  12.2
 dock  10
ptype: Series
yesterday, in today's order
north  13
 east  12.2
south  15.1
 hill  NULL
ptype: Series
change since yesterday
north  1.2
 east  -0.4
south  -0.1
 hill  NULL
ptype: Series
no previous reading: 1 station(s)
position of the closed station in yesterday's list
dock  3
Name: find, ptype: Series
report, sorted by name
 east  -0.4
 hill  NULL
north  1.2
south  -0.1
ptype: Series
```
Aligning by position would have compared north with south. `AlignBy::Caselbl` matches by name, and `hill`, which did not exist yesterday, becomes `NULL`, so the change is `NULL` for it rather than a wrong number. `find` locates the closed station in yesterday's list, and sorting by label orders the report alphabetically with the values attached.

## Timestamps and a price stream

A column of timestamps and a column of prices, seven trading days. The `tl()` accessor derives calendar features from the timestamps and `ts()` derives lags, returns and rolling statistics from the numbers.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Slate;

static void show(const char *label, const Slate &s) { std::cout << label << "\n" << s.to_string() << "\n"; }

int main() {
  Slate when(pluss::make_timeline({"2026-11-02T09:00:00Z", "2026-11-03T09:00:00Z", "2026-11-04T09:00:00Z", "2026-11-05T09:00:00Z",
                                    "2026-11-06T09:00:00Z", "2026-11-07T09:00:00Z", "2026-11-09T09:00:00Z"}));
  Slate close(pluss::make_series({100.0, 102.0, 101.0, 105.0, 107.0, 106.0, 110.0}));

  // 1. Calendar features from the timestamps: weekday name, whether it is a weekend, the month.
  show("weekday", when.tl().weekday_name());
  show("weekend?", when.tl().weekend());
  show("month", when.tl().month());

  // 2. Features of the numeric stream: yesterday's value, the change, and the one-day return.
  show("lag 1", close.ts().lag(1));
  show("return", close.ts().returns(1));

  // 3. A smoothed value: the mean of the last three observations (the first positions use the observations available so far).
  show("rolling mean (3)", close.ts().rolling(3));
}
```

Output

```text
weekday
0  Monday
1  Tuesday
2  Wednesday
3  Thursday
4  Friday
5  Saturday
6  Monday
ptype: Note
weekend?
0  false
1  false
2  false
3  false
4  false
5  true
6  false
ptype: Series
month
0  11
1  11
2  11
3  11
4  11
5  11
6  11
ptype: Series
lag 1
0  NULL
1  100
2  102
3  101
4  105
5  107
6  106
ptype: Series
return
0  NULL
1  0.02
2  -0.009803921569
3  0.0396039604
4  0.01904761905
5  -0.009345794393
6  0.03773584906
ptype: Series
rolling mean (3)
0  100
1  101
2  101
3  102.6666667
4  104.3333333
5  106
6  107.6666667
ptype: Series
```
The weekday names show the Saturday and the Monday after it, `weekend` flags the Saturday, and the first lag and the first return are `NULL` because there is no previous day. The return on the second day is `0.02` (`102 / 100 - 1`). The rolling mean of three observations uses what is available at the start (`100`, then `101`) and the full window from the third day on.

## Decisions

- **Missing is a state:** build slates with a validity mask, and use the slate's own reductions so that the mask is respected.
- **Labels for alignment:** give a slate case labels when its order is not guaranteed and combine slates by name, not by position.
- **Text operations:** use `nt()` for text slates; a prefix pattern is `matches`, a whole-cell pattern is `fullmatch`.
- **Calendar and time-series features:** `tl()` and `ts()` return slates, so they feed a table directly.

## Where next

Reference pages: [`describe`](../api/slate/describe.md), [`percentiles`](../api/slate/percentiles.md), [`rank`](../api/slate/rank.md), [`bucketize`](../api/slate/bucketize.md), [`fillnull`](../api/slate/fillnull.md), [`median`](../api/slate/median.md), [`matches`](../api/slate/matches.md), [`label`](../api/slate/label.md), [`to_string`](../api/slate/to_string.md). Next: [Reports and joins](reports_and_joins.md).
