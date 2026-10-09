// ─────────────────────────────────────────────────────────────────────────────
//  output_engine_pluss_types_tour.cpp
//
//  pluss::tils::OutputEngine + PrintOptions, driven entirely by Pluss types --
//  Series, Note, Timeline / TimeIndex / Duration, Slate, DataFrame -- plus the
//  scalars and vectors you pull out of them. Raw API calls, no helper functions.
//
//  Two things to understand up front:
//
//   1. Pluss's rich types render *themselves*. `Series` / `Note` / `Timeline`
//      go through `operator<<`; `Slate` / `DataFrame` / `DataFrameView` go
//      through their ADL `output_render`. So for those, a per-call PrintOptions
//      controls the ENVELOPE around the rendered block -- separator between
//      several of them, `prefix` / `suffix`, `line_prefix` (indent a multi-line
//      frame), whole-block ANSI `style`, `end`, `flush`, and top-level
//      `width` / `align` / `truncate` / `case_transform`.
//
//   2. The inner knobs -- `notation`, `precision`, `grouping`, `sign`,
//      `humanize`, `bool_style`, container brackets, `quote_strings` -- bite on
//      C++ scalars and containers. With Pluss data you reach them by extracting:
//      `series.item<double>()`, `series.to_vec<double>()`, `df.features()`,
//      `df.shape()`, `timeline.to_epoch_nanos_vector()`, `slate.sum().to_dbl()`.
//
//  Build (from the CMake build dir):
//    c++ -std=c++20 -O2 -I../PlussMic/src -I../PlussQit/include -I../PlussQit/src \
//        -I../PlussTils/include -Igenerated  ... (see compile_commands.json) \
//        ../PlussMic/examples/table/output_engine_pluss_types_tour.cpp -o tour \
//        ../libPlussMic.a ../libpluss.a ... && ./tour
// ─────────────────────────────────────────────────────────────────────────────
#include "table/DataFrame.h"
#include "pluss/tils/aux/output.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;
using pluss::Series;
using pluss::Note;
using pluss::Timeline;
using pluss::table::DataFrame;
using pluss::table::Slate;
using Lvl = OutputLevel;

int main() {
  OutputEngine out;                       // -> std::cout
  std::ostringstream sink;
  OutputEngine cap{sink};                 // -> capture buffer

  // ── some Pluss data to render ──────────────────────────────────────────
  Series prices  = pluss::make_series({1899.0, 249.5, 39999.0, 4.25, 128000.0});
  Series counts  = pluss::make_series(std::vector<std::int64_t>{3, 0, 12, 7, 41});
  Note   cities  = pluss::make_note({"Accra", "Kumasi", "Tema", "Accra", "Tema"});
  Timeline built = pluss::make_timeline({"2021-06-14T00:00:00Z",
                                         "2019-02-03T09:30:00Z",
                                         "2022-11-30T18:15:00Z"});

  DataFrame homes = DataFrame({
      {"home_id",  Slate(pluss::make_note({"H-104", "H-887", "H-231"}))},
      {"price",    Slate(pluss::make_series({412000.0, 268500.0, 559900.0}))},
      {"sqft",     Slate(pluss::make_series(std::vector<std::int64_t>{1850, 1420, 2610}))},
      {"built_on", Slate(built)},
      {"sold",     Slate(pluss::make_series(std::vector<bool>{true, false, true}))},
  }).title("homes");

  Slate price_col = homes.features("price");     // a Series-kind Slate

  // ══════════════════════════════════════════════════════════════════════════
  //  A. RENDERING PLUSS TYPES DIRECTLY  (each renders itself)
  // ══════════════════════════════════════════════════════════════════════════
  // Series / Note stream through operator<< -- Series prints a DEBUG repr;
  // wrap it in a Slate for the aligned table view.
  out.writeln("A1 Series via operator<< (debug repr):", prices);
  out.writeln("A2 same Series as a Slate (table view):", Slate(prices));
  out.writeln("A3 Note via operator<<       :", cities);
  // A bare *multi-element* Timeline has a scalar-only operator<< (it throws),
  // so wrap it in a Slate -- which renders a temporal column properly.
  out.writeln("A4 Timeline (1 element)      :", pluss::make_timeline("2024-01-01T00:00:00Z"));
  out.writeln("A5 Timeline column via Slate:", Slate(built));
  out.writeln("A6 Slate via output_render  :", price_col);
  // A reduction is a SlateValue -- a scalar box; extract with .to_dbl()/.to_int()/.to_str().
  out.writeln("A7 SlateValue -> .to_dbl()  :", price_col.mean().to_dbl());
  out.writeln("A8 DataFrame via output_render:");
  out.writeln(homes);
  out.writeln("A9 DataFrameView (head)      :");
  out.writeln(homes.head(2));
  out.writeln("A10 DataFrameView (observe/record card):");
  out.writeln(homes.observe(0));
  out.writeln("A11 explicit .to_string()    :", homes.features("home_id").to_string());
  out.writeln("A12 two Slates, one call, with a separator:", Slate(prices), Slate(counts),
              PrintOptions{}.with_sep("\n   ---\n"));

  // ══════════════════════════════════════════════════════════════════════════
  //  B. ENVELOPE CONTROL over a Pluss render  (PrintOptions is the LAST arg)
  // ══════════════════════════════════════════════════════════════════════════

  // B1 -- separator between two frames
  out.writeln("B1 two frames, sep = a rule:",
              homes.head(1), homes.tail(1),
              PrintOptions{}.with_sep("\n----------------------------------------\n"));

  // B2 -- prefix / suffix wrap the WHOLE call output (label + value + newline)
  out.writeln("B2 mean price:", price_col.mean().to_dbl(),
              PrintOptions{.prefix = "  >> ", .suffix = " <<"});

  // B3 -- line_prefix: indent / comment every physical line of a multi-line frame
  out.writeln("B3 frame indented with line_prefix:");
  out.writeln(homes, PrintOptions{.line_prefix = "    | "});

  // B4 -- whole-block ANSI style (color_mode = Always forces the escapes so you
  //       can see them even in a pipe; on a real terminal this is green+bold).
  out.writeln("B4 styled frame block:");
  out.writeln(homes.head(1),
              PrintOptions{.color_mode = ColorMode::Always,
                           .style = Style{.fg = Color::Green, .bold = true}});

  // B5 -- capture a DataFrame render to a std::string, then reuse it
  cap.writeln(homes.tail(1));
  out.writeln("B5 captured frame text length =", static_cast<long>(sink.str().size()),
              "bytes; first line:");
  out.writeln(sink.str().substr(0, sink.str().find('\n')));
  sink.str({});

  // B6 -- format path: a Slate inside {} (renders itself); scalars via .to_dbl()
  out.writelnf("B6 the price column is:\n{}\nand its mean is {}",
               price_col, price_col.mean().to_dbl());

  // B7 -- level-tagged frame
  out.log(Lvl::Info, "B7 loaded frame ->", homes.shape());   // shape() is a std::pair (see C)

  // B8 -- top-level width/align on a rendered scalar (Slate reduction -> double)
  out.writelnf("B8 right-aligned reduction: [{}]", price_col.max().to_dbl(),
               PrintOptions{.align = Align::Right, .width = 20, .grouping = ','});

  // B9 -- flush after a specific line
  out.writeln("B9 flushed immediately", PrintOptions{.flush = true});

  // ══════════════════════════════════════════════════════════════════════════
  //  C. FULL PrintOptions POWER on values EXTRACTED from Pluss types
  // ══════════════════════════════════════════════════════════════════════════

  // C1 -- a single scalar: Series::item<double>()  (needs a 1-element Series)
  Series total = homes.features("price").as_series().sum(0, false);   // reduce -> 1 elem
  out.writeln("C1 grand total price:", total.item<double>(),
              PrintOptions{}.fixed(2).grouped(','));

  // C2 -- Slate reduction -> double, with sign + notation
  out.writeln("C2 mean price (scientific, +sign):", price_col.mean().to_dbl(),
              PrintOptions{.notation = Notation::Scientific, .precision = 3,
                           .sign = Sign::Always});

  // C3 -- Series -> std::vector<double>, capped + verbose
  out.writeln("C3 first prices (capped):", prices.to_vec<double>(),
              PrintOptions{.max_items = 3, .more_verbose = true});

  // C4 -- Series<double> as a % column
  Series ratios = pluss::make_series({0.031, 0.5, 0.812, 1.0});
  out.writeln("C4 ratios as percents:", ratios.to_vec<double>(),
              PrintOptions{.notation = Notation::Percent, .precision = 1});

  // C5 -- Series<int64> of bytes -> human sizes
  Series bytes = pluss::make_series(std::vector<std::int64_t>{512, 1536, 3221225472LL});
  out.writeln("C5 sizes (IEC):", bytes.to_vec<std::int64_t>(),
              PrintOptions{.humanize = Humanize::BytesIEC});

  // C6 -- Humanize::DurationNs is for ELAPSED spans (nanoseconds). Here: the
  //       gaps between consecutive build timestamps, pulled from the Timeline.
  std::vector<std::int64_t> ns = built.epoch_nanos().to_vec<std::int64_t>();
  std::vector<std::int64_t> gaps;
  for (std::size_t i = 1; i < ns.size(); ++i) gaps.push_back(ns[i] - ns[i - 1]);
  out.writeln("C6 gaps between build dates :", gaps,
              PrintOptions{.humanize = Humanize::DurationNs, .humanize_precision = 1});

  // C7 -- DataFrame::features() -> std::vector<std::string>, quoted + custom brackets
  out.writeln("C7 columns:", homes.features(),
              PrintOptions{.quote_strings = true,
                           .list_brackets = std::array<std::string, 2>{"( ", " )"}});

  // C8 -- DataFrame::shape() -> std::pair<int64,int64>, pair_style
  out.writeln("C8 shape as rows=>cols:", homes.shape(),
              PrintOptions{.pair_style = PairStyle::Arrow});

  // C9 -- Series<int64> -> grouped (values large enough to see the separator)
  Series big = pluss::make_series(std::vector<std::int64_t>{1234567, 89, 900000000});
  out.writeln("C9 big counts, grouped:", big.to_vec<std::int64_t>(),
              PrintOptions{.grouping = '_'});

  // C10 -- multiline vector pulled from a Series
  out.writeln("C10 prices, one per line:", prices.to_vec<double>(),
              PrintOptions{.notation = Notation::Fixed, .precision = 2,
                           .grouping = ',', .multiline = true, .indent = "   "});

  // C11 -- bool column extracted -> std::vector<bool>, YesNo style
  out.writeln("C11 sold flags:", homes.features("sold").as_series().to_vec<bool>(),
              PrintOptions{.bool_style = BoolStyle::YesNo});

  // C12 -- a genuine "value_counts" Slate rendered, then its labels extracted
  Slate freq = cities.empty() ? Slate{} : Slate(cities).freq();
  out.writeln("C12 city frequencies (Slate renders itself):");
  out.writeln(freq);
  out.writeln("C12b same counts as a C++ vector, indexed:",
              freq.as_series().to_vec<std::int64_t>(),
              PrintOptions{.show_indices = true});

  // C13 -- pluss::series::scalar_inf as an open bound in a formatted line
  const double lo = -pluss::series::scalar_inf;
  const double hi =  pluss::series::scalar_inf;
  out.writelnf("C13 numeric column bounds: lower={} upper={}", lo, hi,
               PrintOptions{.pos_inf_text = "+INF", .neg_inf_text = "-INF"});

  // ══════════════════════════════════════════════════════════════════════════
  //  D. A SMALL REPORT: render a summary frame, then a formatted numeric line
  // ══════════════════════════════════════════════════════════════════════════
  DataFrame summary = DataFrame({
      {"metric", Slate(pluss::make_note({"count", "min", "max", "mean"}))},
      {"price",  Slate(pluss::make_series({
                     static_cast<double>(homes.features("price").as_series().numel()),
                     homes.features("price").min().to_dbl(),
                     homes.features("price").max().to_dbl(),
                     homes.features("price").mean().to_dbl()}))},
  }).title("price summary");

  out.writeln("D1 summary frame:");
  out.writeln(summary, PrintOptions{.line_prefix = "  "});

  out.writelnf("D2 price spans {} -> {}  (mean {}, {} homes)",
               homes.features("price").min().to_dbl(),
               homes.features("price").max().to_dbl(),
               homes.features("price").mean().to_dbl(),
               homes.features("price").as_series().numel(),
               PrintOptions{}.fixed(0).grouped(','));

  // D3 -- engine baseline: give every subsequent numeric a house style, then a
  //       per-call override, then clear it.
  out.print_defaults(PrintOptions{}.grouped('_').fixed(1));
  out.writeln("D3 baseline house style ->", homes.features("price").mean().to_dbl());
  out.writeln("D3 per-call override    ->", homes.features("price").mean().to_dbl(),
              PrintOptions{}.grouped(',').fixed(2));
  out.print_defaults(PrintOptions{});

  out.writeln("── end of Pluss-types tour ──");
  return 0;
}
