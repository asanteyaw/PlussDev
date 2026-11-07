# PlussTils: Print options

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A print option tells the output engine how to render a value: how wide it is, how many decimals it shows, whether it groups thousands, how a list is bracketed, and how a flag reads. You set these options on a call, or once on the engine as defaults. This chapter works through the option groups in the order you are likely to need them, with a complete program for each group.

## The mental model

A `PrintOptions` value is a bundle of settings, and every field has a neutral default, so you set only what you need. A call's options are merged with the engine's defaults, and anything neither sets falls back to the built-in behavior. A trailing `PrintOptions` belongs to the whole call, which means the settings apply to every value that call writes. To format values differently on one line, use the pattern form, where each placeholder carries its own width and alignment.

The options fall into groups by what they change. Layout options set the width, alignment, fill, and truncation. Number options set the notation, precision, sign, grouping, and humanized units. Boolean and optional options set how flags and empty values read. String options set quoting and case. Container options set the brackets, separators, limits, and line layout. Color options set escape sequences on a terminal.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Fitting values into fixed-width places

A status board needs its values to line up and fit. This program pads a label with dots, truncates a long name with an ellipsis, right-aligns numbers, and builds rows with the pattern form.

```cpp
// Fitting values into fixed-width places. A status board pads labels with a fill character, cuts a
// long service name with an ellipsis, and right-aligns a latency so its digits line up.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::string long_name = "inventory-sync-worker-eu-west-2";

  // Padding: a label is widened with dots in its own field, so the value after it starts in one column.
  out.writelnf("{:.<12}{}", "status", "ok");

  // Truncation: the long name is cut to 16 characters and ends with an ellipsis.
  out.writeln(long_name, PrintOptions{.truncate = 16, .ellipsis = "..."});

  // Alignment: each latency is right-aligned in eight characters, so the last digits line up.
  out.writeln(42, PrintOptions{.width = 8, .align = Align::Right});
  out.writeln(1180, PrintOptions{.width = 8, .align = Align::Right});

  // Rows built with the pattern form, where every field carries its own width.
  out.writelnf("{:<12}{:>8}", "checkout", 42);
  out.writelnf("{:<12}{:>8}", "auth", 7);
  return 0;
}
```

Output

```text
status......ok
inventory-syn...
      42
    1180
checkout          42
auth               7
```

The label is widened to twelve characters with dots, so the value after it starts at the same column. The long name is cut to sixteen characters, the ellipsis included. The two latencies right-align in eight characters, so their last digits line up.

## Example 2: Money and rates

A revenue dashboard shows money with grouping and decimals, a rate as a percentage, and a change with its sign. This program covers those, along with a different grouping character and a different decimal point.

```cpp
// A dashboard of money and rates. Revenue has grouping and two decimals, a conversion rate is a
// percentage, and a change from the last period always shows its sign.
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.writeln("revenue:", 1234567.5, PrintOptions{}.fixed(2).grouped(','));
  out.writeln("conversion:", 0.0427, PrintOptions{.notation = Notation::Percent, .precision = 2});
  out.writeln("change (up):", 12.5, PrintOptions{.sign = Sign::Always, .notation = Notation::Fixed, .precision = 1});
  out.writeln("change (down):", -3.25, PrintOptions{.sign = Sign::Always, .notation = Notation::Fixed, .precision = 2});
  out.writeln("ticket count:", 1234567, PrintOptions{.grouping = ','});
  out.writeln("ticket count with spaces:", 1234567, PrintOptions{.grouping = ' '});
  out.writeln("european decimal:", 3.5, PrintOptions{.notation = Notation::Fixed, .precision = 2, .decimal_point = ','});
  return 0;
}
```

Output

```text
revenue: 1,234,567.50
conversion: 4.27%
change (up): +12.5
change (down): -3.25
ticket count: 1,234,567
ticket count with spaces: 1 234 567
european decimal: 3,50
```

Money groups thousands and shows two decimals. A conversion of 0.0427 reads as 4.27%, because the percent notation expects a fraction. A positive change shows its plus sign, and a negative one its minus sign. The decimal point option changes the separator to a comma.

## Example 3: Measurements in scientific and general notation

A sensor log mixes very small and very large numbers, and it has missing readings. This program uses scientific notation for the small value and general notation for the others, and it names the missing and overflowing values.

```cpp
// A log of sensor readings. Small and large measurements switch to scientific notation, a general
// notation keeps significant digits, and missing or invalid readings print as named text.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.writeln("electron mass (kg):", 9.109e-31, PrintOptions{.notation = Notation::Scientific, .precision = 3});
  out.writeln("sample rate (Hz):", 48000.0, PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("pressure (Pa):", 101325.0, PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("readings:", std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::infinity(),
              PrintOptions{.nan_text = "missing", .pos_inf_text = "overflow"});
  return 0;
}
```

Output

```text
electron mass (kg): 9.109e-31
sample rate (Hz): 4.8e+04
pressure (Pa): 1.013e+05
readings: missing overflow
```

The electron mass reads in scientific form with three digits. The sample rate reads as 4.8e+04 under general notation, because general notation switches to the scientific form for large numbers and keeps four significant digits. The NaN and infinite readings print their configured names.

## Example 4: Optional values and flags

A profile record has optional fields and yes-or-no flags. This program shows an empty optional with a placeholder, a set optional, flags in two styles, and a tag that marks presence.

```cpp
// A user profile record. Optional fields show a placeholder when they are empty, flags print as
// words that a reader understands, and a tag marks which values are present.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <optional>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::optional<std::string> nickname;          // not set
  const std::optional<std::string> city{"Accra"};     // set
  const bool subscribed = true;
  const bool two_factor = false;

  out.writeln("nickname:", nickname, PrintOptions{.null_text = "<none>"});
  out.writeln("city:", city, PrintOptions{.null_text = "<none>"});
  out.writeln("subscribed:", subscribed, two_factor, PrintOptions{.bool_style = BoolStyle::YesNo});
  out.writeln("two factor:", subscribed, two_factor, PrintOptions{.bool_style = BoolStyle::OnOff});
  out.writeln("tagged:", nickname, city, PrintOptions{.optional_tag = true});
  return 0;
}
```

Output

```text
nickname: <none>
city: Accra
subscribed: yes no
two factor: on off
tagged: None Some(Accra)
```

The empty nickname shows its placeholder, and the set city shows its value. The flags read as yes and no in one line and as on and off in another. The optional tag shows None for the empty value and Some with the value for the set one.

## Example 5: Names, quotes, and case

A badge run lists attendee names and prints headings in a chosen case. Quoting keeps a name with a comma clear, and case transforms change how words read without changing the stored text.

```cpp
// A list of attendee names for a badge run. Names are title-cased, and inside a list each name is
// quoted so a name with a space or a comma is still clear.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::vector<std::string> attendees{"ama mensah", "kofi, jr", "efua boateng"};

  out.writeln("badges:", attendees, PrintOptions{.quote_strings = true, .quote_char = '"'});
  out.writelnf("title case: {}", "ama mensah", PrintOptions{.case_transform = CaseXf::Title});
  out.writelnf("shout: {}", "room 4", PrintOptions{.case_transform = CaseXf::Upper});
  out.writelnf("sentence: {}", "the BADGE printer is out", PrintOptions{.case_transform = CaseXf::Sentence});
  return 0;
}
```

Output

```text
badges: ["ama mensah", "kofi, jr", "efua boateng"]
title case: Ama Mensah
shout: ROOM 4
sentence: The badge printer is out
```

Inside the list every name is quoted, so the comma in the second name stays part of it. Title case capitalizes each word, and upper case shouts a room number. Sentence case capitalizes only the first letter of each sentence and lowercases the rest, so the sentence reads naturally.

## Example 6: Pairs and maps

A feature-flag report prints a map of rollout percentages, and a comparison of before and after values. This program sorts the keys, changes the key-value separator and the brackets, and shows a pair in two styles.

```cpp
// A feature-flag report. Each flag is a key and a value, shown as a map with a chosen key-value
// separator and brackets, and a pair shows the same shape for one comparison.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <map>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  std::map<std::string, int> rollout{{"beta-search", 25}, {"new-checkout", 100}, {"dark-mode", 0}};

  out.writeln("rollout:", rollout, PrintOptions{.sort_keys = true, .kv_sep = " = ", .map_brackets = std::array<std::string, 2>{"{ ", " }"}});
  out.writeln("rollout list:", rollout, PrintOptions{.sort_keys = true, .kv_sep = ": ", .item_sep = " | "});
  out.writeln("before and after:", std::pair{25, 60}, PrintOptions{.pair_style = PairStyle::Arrow});
  out.writeln("before and after:", std::pair{25, 60}, PrintOptions{.pair_style = PairStyle::Tuple});
  return 0;
}
```

Output

```text
rollout: { beta-search = 25, dark-mode = 0, new-checkout = 100 }
rollout list: {beta-search: 25 | dark-mode: 0 | new-checkout: 100}
before and after: 25 => 60
before and after: (25, 60)
```

The map sorts its keys alphabetically, and the separator and brackets follow the options. The same pair prints as an arrow or as a tuple, depending on the pair style.

## Example 7: Long values in a log line

A log that prints a long history must stay short enough to read. This program caps a flat list with and without a count of what was left out, and limits how deep a nested value is shown.

```cpp
// A log line that quotes a long history. Lists are capped with a count of what was left out, and
// nested lists are cut at a depth so one value cannot fill the terminal.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  std::vector<int> history{4, 8, 15, 16, 23, 42, 8, 4, 15, 16};
  std::vector<std::vector<int>> grid{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

  out.writeln("last ten values:", history, PrintOptions{.max_items = 4});
  out.writeln("last ten, with a count:", history, PrintOptions{.max_items = 4, .more_verbose = true});
  out.writeln("grid, nested:", grid);
  out.writeln("grid, top level only:", grid, PrintOptions{.max_depth = 1});
  return 0;
}
```

Output

```text
last ten values: [4, 8, 15, 16, …]
last ten, with a count: [4, 8, 15, 16, … (+6 more)]
grid, nested: [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
grid, top level only: [[…], […], […]]
```

The capped list ends with an ellipsis, and the verbose form also says how many items were left out. The nested grid prints in full at the default depth, and at a depth of one each inner list collapses to an ellipsis.

## Example 8: Presets and builders

Some outputs want a standard shape: plain for a message, a debug form for a value you are inspecting, and a fixed-width cell for a table. This program uses the three presets, a pretty JSON layout, and a chain of builder calls.

```cpp
// The same value shown three ways: as a plain value, as a debug representation, and as a table
// cell. Presets are named starting points, and builder calls adjust them in a chain.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::vector<int> ids{7, 42, 99};
  const std::optional<std::string> name{"Ama"};

  out.writeln("plain:", ids, name, PrintOptions::plain());
  out.writeln("repr:", ids, name, PrintOptions::repr());
  out.writelnf("cell: [{}]", "checkout", PrintOptions::table_cell(10));
  out.writeln("built with a chain:", 1234567.5, PrintOptions{}.fixed(1).grouped(',').with_sep(" "));
  out.writeln("pretty json:", ids, PrintOptions::pretty_json());
  return 0;
}
```

Output

```text
plain: 7, 42, 99 Ama
repr: [7, 42, 99] Some(Ama)
cell: [  checkout]
built with a chain: 1,234,567.5
pretty json: [
  7,
  42,
  99
]
```

The plain preset prints the list without brackets and the name without quotes. The debug form adds brackets and the Some wrapper. The table cell pads the value to ten characters, and the chained builders combine two decimals, grouping, and a separator in one expression.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Shape one value on a line | a trailing `PrintOptions` | It applies to every value in that call, so keep one value per call when the settings differ. |
| Shape each column of a row | the pattern form with `{:<12}` and `{:>8}` | Each placeholder carries its own width and alignment. |
| Show a fraction as a percentage | `Notation::Percent` | The value is read as a fraction, so 0.0427 shows as 4.27%. |
| Keep large numbers readable | `Humanize` for sizes and durations, or grouping for counts | Each humanize mode names its unit. |
| Keep a long list short | `max_items`, with `more_verbose` if you want the count | The rest is marked with an ellipsis. |
| Show the same style everywhere | engine `print_defaults` | A call can still override it. |

## Pitfalls

**Expecting a trailing option to apply to one value.** p01 and o02 show that every value on the line takes the same settings.

**Putting the options in the middle of the arguments.** The trailing options must be the last argument, or the call does not compile. Put the options after the values they describe.

**Expecting general notation to keep large numbers in plain form.** p03 shows 48000 printed as 4.8e+04, because general notation switches to scientific form for large values.

**Expecting a percentage to take a whole number.** p02 shows 0.0427 as 4.27%. Pass a fraction, not a percentage.

## Where next

- [Writing output](tils-output.md) -- the engine that uses these options.
- [Values in pipelines: suppose and let](tils-suppose-let.md) -- the wrappers that often hold the values you print.
- [`PrintOptions` reference](../api/core-print-options.md) -- every field group, builder, and preset.
