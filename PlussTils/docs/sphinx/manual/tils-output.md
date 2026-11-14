# PlussTils: Writing output

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

Every program writes something: a progress line, a report, a log entry, a dump of a value that is hard to read on its own. PlussTils's output engine handles all of these with one set of rules, so a log line and a report row look and behave the same way. This chapter shows the engine doing real work, from a run logger to a capture-to-string test, and explains the rules that make it predictable.

## The mental model

An output engine is an object that holds a destination, a separator, the level prefixes, a minimum level, and a set of default print options. Each call hands it some values. The engine renders each value to text, joins the pieces with the separator, and writes the line to the destination. A call may end with a `PrintOptions` value. That trailing value describes how to render the whole call, so it applies to every value on that line, not only the last one.

There are two ways to write. The value form, `writeln(a, b, c)`, renders each value and joins them. The pattern form, `writelnf("{} of {}", a, b)`, places the values into a pattern whose placeholders can carry their own widths and alignments. Logging methods, such as `info` or `warn`, are the value form with a level prefix, and a level below the minimum is dropped without a trace.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: A run logger that filters by level

A batch job needs a log that says what happened, and that can be quieter in production. This program sets the minimum level to info, so a debug line is dropped before it reaches the destination. It also changes the prefix of the error level to a custom tag, and leaves the other prefixes alone.

```cpp
// A run logger for a batch job. It writes levelled lines to standard output, filters out debug
// lines below its minimum level, and shows how a custom prefix changes the look of one level.
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.min_level = OutputLevel::Info;  // trace and debug lines are dropped, not printed
  OutputEngine log{std::cout, options};

  log.info("batch started, jobs queued:", 4);
  log.debug("this debug line is below the minimum level, so it never prints");
  log.info("loading", "sales.csv");
  log.warn("row 17 has an empty region, using 'unknown'");
  log.prefix(OutputLevel::Error, "[FAIL]");  // only the error prefix changes
  log.error("job 3 failed after", 2, "retries");
  log.info("batch finished, failed jobs:", 1);
  return 0;
}
```

Output

```text
[INFO] batch started, jobs queued: 4
[INFO] loading sales.csv
[WARNING] row 17 has an empty region, using 'unknown'
[FAIL] job 3 failed after 2 retries
[INFO] batch finished, failed jobs: 1
```

The debug line never appears, because it is below the minimum. Each info, warning, and error line carries its level prefix, and the error line uses the custom tag, so you can see which change touched which line.

## Example 2: A report whose columns line up

A report is read as a table, so its columns must line up. This program writes a header, a rule, one row per sale, and a total. Each row uses the pattern form, with a left-aligned region and right-aligned numbers, and the trailing options set two decimals and grouping for the whole line.

```cpp
// A monthly sales report whose columns line up. Each row is written with the format path, so the
// widths and alignments are part of the pattern, and the totals use grouping and fixed decimals.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>
#include <vector>

using namespace pluss::tils;

struct Sale {
  std::string region;
  long units;
  double revenue;
};

int main() {
  OutputEngine out{std::cout};
  const std::vector<Sale> sales{
      {"north", 1280, 15360.5},
      {"south", 96, 1152.0},
      {"east", 25400, 304800.25},
  };

  out.writelnf("{:<8}{:>10}{:>16}", "region", "units", "revenue");
  out.writelnf("{:-<34}", "");
  long total_units = 0;
  double total_revenue = 0.0;
  for (const auto& sale : sales) {
    // The trailing options apply to every value on the line, so units and revenue both group,
    // and revenue shows two decimals.
    out.writelnf("{:<8}{:>10}{:>16}", sale.region, sale.units, sale.revenue,
                 PrintOptions{}.fixed(2).grouped(','));
    total_units += sale.units;
    total_revenue += sale.revenue;
  }
  out.writelnf("{:-<34}", "");
  out.writelnf("{:<8}{:>10}{:>16}", "total", total_units, total_revenue,
               PrintOptions{}.fixed(2).grouped(','));
  return 0;
}
```

Output

```text
region       units         revenue
----------------------------------
north        1,280       15,360.50
south           96        1,152.00
east        25,400      304,800.25
----------------------------------
total       26,776      321,312.75
```

The columns line up because the widths live in the pattern. The trailing options grouped the units and the revenue on each row, and fixed the revenue to two decimals, so the total reads 321,312.75. Notice that the units also grouped, because the options apply to the whole line.

## Example 3: Dumping a configuration for a support ticket

When you ask someone to look at a problem, a dump of the settings is more useful than a list of guesses. This program prints a map of settings sorted by key with pretty layout, a list capped to its first four items, and a list written one item per line with an indent.

```cpp
// Dumping a configuration and a lookup table for a support ticket. Containers print in a readable
// form, maps can be sorted by key, long lists can be capped, and multiline output is indented.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  std::map<std::string, std::string> settings{
      {"timeout", "30s"}, {"region", "eu-west"}, {"retries", "3"}};
  std::vector<int> recent_codes{200, 200, 404, 500, 200, 503, 200, 200, 429, 200};

  out.writeln("settings:", settings, PrintOptions{.sort_keys = true}.pretty());
  out.writeln("recent codes, first four:", recent_codes, PrintOptions{}.capped(4));
  out.writeln("recent codes, one per line:", recent_codes,
              PrintOptions{.multiline = true, .indent = "    "});
  return 0;
}
```

Output

```text
settings: {
  region: eu-west,
  retries: 3,
  timeout: 30s
}
recent codes, first four: [200, 200, 404, 500, …]
recent codes, one per line: [
    200,
    200,
    404,
    500,
    200,
    503,
    200,
    200,
    429,
    200
]
```

Keys come out in alphabetical order, so two dumps can be compared by eye. The capped list ends with an ellipsis, so a long history stays short. The multiline list indents each item, which makes a long value easy to scan.

## Example 4: Human-readable sizes and durations

Raw byte counts and nanosecond durations are hard to read. This program prints the same kinds of numbers with the humanize options, in binary and decimal bytes, in a metric count, and as a duration.

```cpp
// Sizes and durations for a storage report. Raw counts are hard to read, so the output options
// turn them into IEC bytes, SI bytes, metric counts, and durations.
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.writeln("cache size:", 1536, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("disk size:", 3221225472LL, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("upload size (SI):", 2500000LL, PrintOptions{.humanize = Humanize::BytesSI});
  out.writeln("rows processed:", 6700000LL, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("job took:", 90000000000LL, PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("cache size, two decimals:", 1536,
              PrintOptions{.humanize = Humanize::BytesIEC, .humanize_precision = 2});
  return 0;
}
```

Output

```text
cache size: 1.5 KiB
disk size: 3.0 GiB
upload size (SI): 2.5 MB
rows processed: 6.7M
job took: 90.0 s
cache size, two decimals: 1.50 KiB
```

The disk size reads as 3.0 GiB, a binary unit, and the upload size reads as 2.5 MB, a decimal unit. The duration of 90 billion nanoseconds reads as 90.0 s. The last line sets the precision to two places, so the cache size reads 1.50 KiB.

## Example 5: Capturing output into a string

A test, or a program that saves its summary to a file, needs the text rather than a printed line. This program points the engine at a string stream, writes two lines with formatting, and then reads what was captured.

```cpp
// Capturing output into a string instead of printing it. This is how a test checks what a report
// would say, or how a program saves a summary to a file it writes itself.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};
  const std::vector<double> latencies{12.5, 48.0, 7.25};

  out.writeln("latencies (ms):", latencies, PrintOptions{}.fixed(1));
  out.writelnf("count={} slowest={}", latencies.size(), 48.0);

  const std::string report = captured.str();
  std::cout << "captured " << report.size() << " characters\n";
  std::cout << "captured text:\n" << report;
  return 0;
}
```

Output

```text
captured 53 characters
captured text:
latencies (ms): [12.5, 48.0, 7.2]
count=3 slowest=48
```

Nothing reached the terminal until the program printed the captured text. The capture is an ordinary string, so you can measure it, compare it, or write it to a file.

## Example 6: A house style set once for every number

A finance team wants every number to group thousands and show two decimals. Setting that on each call would be repetitive, so this program sets it once as the engine's defaults. One call overrides it, and the rest keep it. The last part clears the defaults, so the output goes back to plain form.

```cpp
// A house style for a finance team: every number groups thousands and shows two decimals. The
// style is set once on the engine. One call overrides it, and the rest keep it.
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.print_defaults(PrintOptions{}.grouped(',').fixed(2));

  out.writeln("invoice total:", 1234567.5);                       // house style applies
  out.writeln("tax, zero decimals:", 1234567.5, PrintOptions{}.fixed(0));  // this call overrides
  out.writeln("discount:", 250.0);                                // house style again

  out.print_defaults(PrintOptions{});                             // clear the house style
  out.writeln("plain again:", 1234567.5);
  return 0;
}
```

Output

```text
invoice total: 1,234,567.50
tax, zero decimals: 1,234,568
discount: 250.00
plain again: 1234567.5
```

The invoice total and the discount both use the house style. The tax line overrides it to zero decimals, and the rounding moves 1,234,567.5 to 1,234,568. The plain line after clearing the defaults prints the number as it is.

## Example 7: Strict and forgiving format strings

A format string in a configuration file can drift from the values it is given, and a silent mismatch is hard to find. A string literal cannot drift: with too few arguments, an unmatched brace, or a malformed spec it does not compile. A pattern that arrives at run time is wrapped in `runtime_format` and is checked when the call runs. Strict mode turns an extra value into an error, and a missing value can show a visible marker, so the mistake is seen but the program keeps going.

```cpp
// Format strings in a configuration file can drift from the values they are given. A pattern that
// arrives at run time is checked when the call runs: strict mode turns a mismatch into an error, and a
// missing argument can be shown as a visible marker instead. (A string literal with too few arguments
// does not compile at all.)
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::string shipped = "order {} shipped to {}";             // as read from a configuration file
  const std::string dated = "order {} shipped to {} on {}";

  // A pattern with two placeholders is given three values. Strict mode refuses the extra one.
  try {
    out.writelnf(runtime_format(shipped), 1001, "Lagos", "express", PrintOptions{.strict_args = true});
  } catch (const pluss::Error& error) {
    const std::string text = error.what();
    out.writeln("strict mode refused the line:", text.substr(0, text.find('\n')));
  }

  // A pattern with three placeholders is given two values. The missing one shows the marker.
  out.writelnf(runtime_format(dated), 1001, "Lagos", PrintOptions{.missing_arg = "<?>"});
  return 0;
}
```

Output

```text
strict mode refused the line: OutputEngine: format placeholder / argument count mismatch
order 1001 shipped to Lagos on <?>
```

The first line is refused with the engine's message, so the mismatch cannot pass unnoticed. The second line prints the marker where the value is missing, so the output shows exactly what was not supplied.

## Example 8: Colored status lines for a terminal tool

A tool that reports build status uses color to make a failure stand out. This program forces color on so the escape sequences appear in the captured output, and writes one colored status, one colored failure, and one line with color turned off. The program replaces the escape byte with the text ESC so the sequences are readable here.

```cpp
// Colored status lines for a terminal tool. Color is forced on here so the escape sequences show
// up in the captured output. In a real terminal, color_mode Auto colors only when the output is a
// TTY, which is the usual choice.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

// Replaces the escape byte with the text ESC, so the sequences are readable in the document.
std::string visible(std::string text) {
  std::string out;
  for (char c : text) {
    if (c == '\x1b') out += "ESC";
    else out += c;
  }
  return out;
}

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};
  out.writeln("build:", "passed", PrintOptions{.color_mode = ColorMode::Always,
                                               .style = Style{.fg = Color::Green, .bold = true}});
  out.writeln("deploy:", "blocked", PrintOptions{.color_mode = ColorMode::Always,
                                                 .style = Style{.fg = Color::Red}});
  out.writeln("notes:", "plain text", PrintOptions{.color_mode = ColorMode::Never});
  std::cout << visible(captured.str());
  return 0;
}
```

Output

```text
ESC[1;32mbuild: passedESC[0m
ESC[31mdeploy: blockedESC[0m
notes: plain text
```

The escape sequences wrap the text, and the line with color off has none. In a real terminal you would leave the color mode on automatic, which colors only when the output is a terminal, so pipes and log files stay clean.

## Example 9: Building text before writing it

Some text is not printed: it goes into an error message, a column label, or a file name. `OutputEngine::format` returns the formatted text as a string with the same patterns as `writelnf`, and `OutputEngine::join` turns a range into one string. A spec can set the fill, alignment, sign, base, zero padding, width, and precision, and the width or precision can come from another argument with a nested `{}`.

```cpp
// Building text before writing it: a message for an error, a label for a column, a file name.
// format returns a string, join turns a range into one, and a string-literal pattern is checked
// against its arguments while compiling.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const std::vector<std::string> columns{"id", "city", "sales"};
  const std::string missing = "revenue";

  // An error message that lists what exists.
  const std::string message = OutputEngine::format("unknown column '{}'; available: [{}]", missing, OutputEngine::join(columns, ", "));

  // A zero-padded file name and a right-aligned amount with a sign and two decimals.
  const std::string file = OutputEngine::format("report_{:04d}.csv", 7);
  const std::string amount = OutputEngine::format("[{:>+10.2f}]", 1234.5);

  // The width itself comes from a value.
  const std::string padded = OutputEngine::format("[{:<{}}]", "id", 6);

  OutputEngine out{std::cout};
  out.writeln(message);
  out.writeln(file, amount, padded);
  return 0;
}
```

Output

```text
unknown column 'revenue'; available: [id, city, sales]
report_0007.csv [  +1234.50] [id    ]
```

The error message lists the columns that exist, the file name is zero-padded, the amount is aligned and signed, and the last width came from a value. None of these wrote anything until the engine printed them.

## Decisions

| You want | Use | Notes |
|---|---|---|
| A line of values, joined | `writeln` | Each value is rendered, and the separator joins them. |
| A line with widths and alignment | `writelnf` | The pattern holds the layout, so columns line up. |
| A message that has a severity | `info`, `warn`, `error` and friends | A level below the minimum is dropped. |
| A format that applies to one call | a trailing `PrintOptions` | It applies to every value on that call. |
| A format that applies to every call | `print_defaults` | Any call can still override it. |
| Text you need as a string | `OutputEngine::format` and `OutputEngine::join` | A literal pattern is checked while compiling. |
| The text a call would have printed | an `OutputEngine` over an `std::ostringstream` | The capture is the text the engine would have printed. |
| A pattern that arrives at run time | `runtime_format(text)` | It is checked when the call runs. |

## Pitfalls

**Expecting a trailing option to format only the last value.** o02 shows the grouping and decimals applying to units as well as revenue.

**Expecting a debug line to print after a minimum level is set above it.** o01 shows the debug line is dropped without a trace.

**Expecting a format mismatch to be silent.** A literal pattern with too few arguments does not compile. o07 shows that a run-time pattern behaves differently: strict mode refuses an extra value, and a missing value shows the marker only when you ask for it.

**Passing a pattern held in a variable straight to a formatting call.** The call wants a string literal it can check while compiling, so wrap the variable in `runtime_format(pattern)`.

**Expecting color to appear in a pipe.** o08 turns color on explicitly. The default automatic mode colors only in a terminal.

## Where next

- [Print options](tils-print-options.md) -- every option a trailing `PrintOptions` can carry.
- [Values in pipelines: flow and let](tils-suppose-let.md) -- the wrappers that often hold the values you print.
- [`OutputEngine` reference](../api/core-output-engine.md) -- every member, with its own examples.
- [`OutputLevel`](../api/core-output-level.md) and [`OutputOptions`](../api/core-output-options.md) -- the level enum and the bulk configuration.
