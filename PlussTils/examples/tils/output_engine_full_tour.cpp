// ─────────────────────────────────────────────────────────────────────────────
//  output_engine_full_tour.cpp
//
//  An exhaustive, run-it-yourself tour of pluss::tils::OutputEngine and
//  pluss::tils::PrintOptions. Every constructor, every method, every value
//  category the renderer understands, and every PrintOptions knob is exercised
//  below with raw API calls -- no helper functions.
//
//  Build (fmt not required):
//    c++ -std=c++20 -O2 \
//        -I <repo>/PlussTils/include -I <repo>/PlussQit/include \
//        output_engine_full_tour.cpp -o tour && ./tour
// ─────────────────────────────────────────────────────────────────────────────
#include "pluss/tils/auxiliary/output.h"

#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace pluss::tils;
using Lvl = OutputLevel;

// ── user types, one per renderer hook ───────────────────────────────────────
struct Point {                          // rendered via operator<<
  int x{}, y{};
};
std::ostream& operator<<(std::ostream& os, const Point& p) {
  return os << '(' << p.x << ',' << p.y << ')';
}

struct Money {                          // rendered via .to_string()
  long cents{};
  std::string to_string() const {
    return "$" + std::to_string(cents / 100) + "." +
           (cents % 100 < 10 ? "0" : "") + std::to_string(cents % 100);
  }
};

struct Widget {                         // rendered via member .show(os)
  int id{};
  void show(std::ostream& os) const { os << "Widget#" << id; }
};

struct Tag {                            // rendered via ADL output_render()
  std::string name;
};
void output_render(std::ostream& os, const Tag& t) { os << '<' << t.name << '>'; }

int main() {
  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 1 — CONSTRUCTING AN OutputEngine
  // ═════════════════════════════════════════════════════════════════════════
  OutputEngine out;                                        //  1. default sink = std::cout
  OutputEngine err{std::cerr};                             //  2. explicit ostream sink
  std::ostringstream capture;
  OutputEngine cap{capture};                               //  3. capture into a std::ostringstream

  OutputOptions opts;                                      //  4. bulk config object
  opts.separator = " · ";
  opts.levels_enabled = true;
  opts.level_prefixes = {"trc", "dbg", "inf", "wrn", "err"};
  opts.min_level = Lvl::Debug;
  opts.auto_flush = true;
  OutputEngine cfg{opts};                                  //  5. engine from OutputOptions
  OutputEngine cfg2{std::cerr, opts};                      //  6. sink + OutputOptions

  out.writeln("1-6: constructed default / cerr / capture / configured engines");

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 2 — ENGINE-LEVEL CONFIG METHODS (all chainable, return OutputEngine&)
  // ═════════════════════════════════════════════════════════════════════════
  out.sep(" | ");                                          //  7. set the join separator
  out.writeln("7:", "a", "b", "c");                        //     -> 7: | a | b | c
  out.sep(" ");                                            //     restore

  out.prefix(Lvl::Warn, "[!!]");                           //  8. override one level's prefix
  out.warn("8: custom warn prefix");
  out.prefix(Lvl::Warn, "[WARNING]");                      //     restore

  out.enable_levels(false);                                //  9. suppress level prefixes
  out.info("9: no [INFO] tag now");
  out.enable_levels(true);

  OutputOptions snapshot = out.options();                  // 10. read current OutputOptions
  out.writeln("10: current separator = [", snapshot.separator, "], auto_flush =",
              snapshot.auto_flush);

  out.set_options(snapshot);                               // 11. push a whole OutputOptions back

  out.to(capture);                                         // 12. redirect the sink...
  out.writeln("12: this line went to the capture buffer");
  out.to(std::cout);                                       //     ...and back
  out.writeln("12b: capture now holds:", capture.str());
  capture.str({});

  out.print_defaults(PrintOptions{}.grouped('_'));         // 13. engine-wide PrintOptions baseline
  out.writeln("13: baseline grouping ->", 1000000);        //     -> 1_000_000
  PrintOptions current_baseline = out.print_defaults();    // 14. read the baseline back
  out.print_defaults(PrintOptions{});                      //     clear it (empty = no baseline)
  out.writeln("14: baseline cleared ->", 1000000);         //     -> 1000000

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 3 — THE WRITE FAMILY (plain form, engine defaults only)
  // ═════════════════════════════════════════════════════════════════════════
  out.write("15: write() ");                               // 15. no trailing newline
  out.write("continues on the same line");
  out.endln();                                             // 16. endln() emits just '\n'

  out.writeln("17: writeln() joins with the separator and adds '\\n'", 1, 2, 3);

  out.writef("18: writef() no newline -> {} + {} = {}", 2, 3, 5);
  out.endln();
  out.writelnf("19: writelnf() -> {:>6} | {:<6} | {:^6} |", "r", "l", "c");

  out.write("20: ").write("chaining ").write("write ").write("calls").endln();

  out.flush();                                             // 21. flush the sink

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 4 — THE LOG FAMILY (level-tagged; obeys OutputOptions::min_level)
  // ═════════════════════════════════════════════════════════════════════════
  out.log(Lvl::Info, "22: log(level, ...) ->", 42);       // 22. generic
  out.logf(Lvl::Warn, "23: logf(level, pattern, ...) -> code {}", 500);

  out.trace("24: trace()");                                // 24-28. level shortcuts
  out.debug("25: debug()");
  out.info("26: info()");
  out.warn("27: warn()");
  out.error("28: error()");

  out.tracef("29: tracef {}", 1);                          // 29-33. formatted level shortcuts
  out.debugf("30: debugf {}", 2);
  out.infof("31: infof {}", 3);
  out.warnf("32: warnf {}", 4);
  out.errorf("33: errorf {}", 5);

  cfg.trace("34: (min_level=Debug) this trace line is filtered out and never prints");
  cfg.debug("34: (min_level=Debug) this debug line prints");

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 5 — VALUE CATEGORIES THE RENDERER UNDERSTANDS
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("35 strings :", "c-string", std::string("std::string"),
              std::string_view("string_view"));
  out.writeln("36 integers:", 0, -7, 255, 9223372036854775807LL,
              static_cast<std::uint64_t>(18446744073709551615ULL));
  out.writeln("37 floats  :", 3.0, 3.14159, -0.5, 1e-9, 1.0e12);
  out.writeln("38 bool    :", true, false);
  out.writeln("39 char     :", 'A', '\n' == 10 ? 'y' : 'n');   // char prints as a character
  out.writeln("40 optional :", std::optional<int>{7}, std::optional<int>{});
  out.writeln("41 variant  :", std::variant<int, std::string>{std::string("v")});
  out.writeln("42 pair     :", std::pair{1, std::string("one")});
  out.writeln("43 tuple    :", std::tuple{1, 2.5, std::string("x")});
  out.writeln("44 vector   :", std::vector<int>{1, 2, 3});
  out.writeln("45 list     :", std::list<std::string>{"a", "b"});
  out.writeln("46 deque    :", std::deque<int>{9, 8, 7});
  out.writeln("47 set      :", std::set<int>{3, 1, 2});
  out.writeln("48 map      :", std::map<std::string, int>{{"a", 1}, {"b", 2}});
  out.writeln("49 nested   :",
              std::vector<std::vector<int>>{{1, 2}, {3, 4}},
              std::map<std::string, std::vector<int>>{{"k", {5, 6}}});
  out.writeln("50 operator<<:", Point{3, 4});
  out.writeln("51 .to_string():", Money{129900});
  out.writeln("52 .show(os) :", Widget{7});
  out.writeln("53 output_render():", Tag{"span"});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 6 — PrintOptions: JOINING knobs  (write(a, b, c, PrintOptions))
  // ═════════════════════════════════════════════════════════════════════════
  PrintOptions j;
  j.sep = std::string(", ");                               // 54. between top-level args
  j.end = std::string(" ;");                               // 55. after the last arg (before '\n')
  j.prefix = std::string(">>> ");                          // 56. before everything
  j.suffix = std::string(" <<<");                          // 57. after content, before `end`
  j.line_prefix = std::string("    ");                     // 58. prepended to every physical line
  j.flush = true;                                          // 59. force flush after this call
  out.writeln("54-59:", "one", "two", "three", j);

  out.writeln("60 line_prefix over a multi-line value:",
              std::vector<int>{1, 2, 3},
              PrintOptions{.line_prefix = "  | ", .multiline = true});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 7 — PrintOptions: FIELD LAYOUT  (applied to each top-level value)
  //  NOTE: a per-call PrintOptions must be the LAST argument. To *see* the
  //  field width, either put boundary markers in the format pattern (below) or
  //  bracket the line with .prefix / .suffix.
  // ═════════════════════════════════════════════════════════════════════════
  out.writelnf("61 width + Left   : [{}]", "hi", PrintOptions{.align = Align::Left,   .width = 8});
  out.writelnf("62 width + Right  : [{}]", "hi", PrintOptions{.align = Align::Right,  .width = 8});
  out.writelnf("63 width + Center : [{}]", "hi", PrintOptions{.align = Align::Center, .width = 8});
  out.writelnf("64 Numeric (=Right): [{}]", -12, PrintOptions{.align = Align::Numeric, .width = 8});
  out.writelnf("65 fill char '.'  : [{}]", "hi",
               PrintOptions{.align = Align::Right, .width = 8, .fill = '.'});
  out.writelnf("66 truncate        : {}", "a-very-long-name", PrintOptions{.truncate = 10});
  out.writelnf("67 truncate+ellipsis: {}", "another-long-name",
               PrintOptions{.truncate = 12, .ellipsis = "..."});
  out.writelnf("68 width + truncate: [{}]", "supercalifragilistic",
               PrintOptions{.align = Align::Left, .width = 15, .truncate = 15});
  out.writeln("68b .prefix/.suffix bracket the whole line:", "hi",
              PrintOptions{.prefix = "[", .suffix = "]", .align = Align::Right, .width = 6});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 8 — PrintOptions: NUMBER FORMATTING
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("69 Auto        :", 3.14159265358979, PrintOptions{.notation = Notation::Auto});
  out.writeln("70 Fixed .3    :", 3.14159265358979,
              PrintOptions{.notation = Notation::Fixed, .precision = 3});
  out.writeln("71 Scientific  :", 123456.789,
              PrintOptions{.notation = Notation::Scientific, .precision = 2});
  out.writeln("72 General .4  :", 123456.789,
              PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("73 Hex         :", 255, PrintOptions{.notation = Notation::Hex});
  out.writeln("74 Hex + alt   :", 255, PrintOptions{.notation = Notation::Hex, .alt = true});
  out.writeln("75 Hex + upper :", 3735928559U,
              PrintOptions{.notation = Notation::Hex, .alt = true, .uppercase = true});
  out.writeln("76 Octal       :", 64,  PrintOptions{.notation = Notation::Octal,  .alt = true});
  out.writeln("77 Binary      :", 10,  PrintOptions{.notation = Notation::Binary, .alt = true});
  out.writeln("78 Percent .1  :", 0.1275,
              PrintOptions{.notation = Notation::Percent, .precision = 1});
  out.writeln("79 precision (Fixed default 6):",
              1.0 / 3.0, PrintOptions{.notation = Notation::Fixed});
  out.writeln("80 sign NegativeOnly:", 5, -5, PrintOptions{.sign = Sign::NegativeOnly});
  out.writeln("81 sign Always     :", 5, -5, PrintOptions{.sign = Sign::Always});
  out.writeln("82 sign Space      :", 5, -5, PrintOptions{.sign = Sign::SpaceForPositive});
  out.writeln("83 grouping ','    :", 1234567, PrintOptions{.grouping = ','});
  out.writeln("84 grouping '_'    :", 1234567, PrintOptions{.grouping = '_'});
  out.writeln("85 grouping ' '    :", 1234567, PrintOptions{.grouping = ' '});
  out.writeln("86 group_size 4    :", 123456789,
              PrintOptions{.grouping = ',', .group_size = 4});
  out.writeln("87 grouping + fraction:", 1234567.891,
              PrintOptions{.notation = Notation::Fixed, .precision = 2, .grouping = ','});
  out.writeln("88 decimal_point ','  :", 3.5,
              PrintOptions{.notation = Notation::Fixed, .precision = 2, .decimal_point = ','});
  out.writeln("89 trim_trailing_zeros:", 1.2000,
              PrintOptions{.notation = Notation::Fixed, .precision = 5,
                           .trim_trailing_zeros = true});
  out.writeln("90 humanize BytesIEC  :", 1536, 3221225472LL,
              PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("91 humanize BytesSI   :", 1500, 2500000LL,
              PrintOptions{.humanize = Humanize::BytesSI});
  out.writeln("92 humanize Metric    :", 12345, 6700000LL,
              PrintOptions{.humanize = Humanize::Metric});
  out.writeln("93 humanize DurationNs:", 2500, 2500000LL, 90000000000LL,
              PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("94 humanize_precision 2:", 1536,
              PrintOptions{.humanize = Humanize::BytesIEC, .humanize_precision = 2});
  out.writeln("95 nan / inf text     :",
              std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::infinity(),
              -std::numeric_limits<double>::infinity(),
              PrintOptions{.nan_text = "NaN", .pos_inf_text = "+Inf", .neg_inf_text = "-Inf"});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 9 — PrintOptions: BOOLEANS / NULL / OPTIONAL
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("96  bool_style TrueFalse:", true, false,
              PrintOptions{.bool_style = BoolStyle::TrueFalse});
  out.writeln("97  bool_style OneZero  :", true, false,
              PrintOptions{.bool_style = BoolStyle::OneZero});
  out.writeln("98  bool_style YesNo    :", true, false,
              PrintOptions{.bool_style = BoolStyle::YesNo});
  out.writeln("99  bool_style OnOff    :", true, false,
              PrintOptions{.bool_style = BoolStyle::OnOff});
  out.writeln("100 bool_style Custom   :", true, false,
              PrintOptions{.bool_style = BoolStyle::Custom,
                           .bool_true = "OUI", .bool_false = "NON"});
  out.writeln("101 null_text           :", std::optional<int>{},
              PrintOptions{.null_text = "<none>"});
  out.writeln("102 optional_tag        :", std::optional<int>{7}, std::optional<int>{},
              PrintOptions{.optional_tag = true});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 10 — PrintOptions: STRINGS
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("103 quote_strings (inside a container):",
              std::vector<std::string>{"a", "b"},
              PrintOptions{.quote_strings = true});
  out.writeln("104 quote_char '\\'':",
              std::vector<std::string>{"x", "y"},
              PrintOptions{.quote_strings = true, .quote_char = '\''});
  out.writelnf("105 case_transform Upper   : {}", "hello world",
               PrintOptions{.case_transform = CaseXf::Upper});
  out.writelnf("106 case_transform Lower   : {}", "HELLO",
               PrintOptions{.case_transform = CaseXf::Lower});
  out.writelnf("107 case_transform Title   : {}", "the quick brown fox",
               PrintOptions{.case_transform = CaseXf::Title});
  out.writelnf("108 case_transform Sentence: {}", "the quick BROWN fox",
               PrintOptions{.case_transform = CaseXf::Sentence});
  // escape_non_printable is accepted; byte-level escaping is reserved for a later pass.
  out.writeln("109 escape_non_printable (reserved, currently a no-op):", "tab\there",
              PrintOptions{.escape_non_printable = true});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 11 — PrintOptions: CONTAINERS / MAPS / TUPLES
  // ═════════════════════════════════════════════════════════════════════════
  std::vector<int> vv{1, 2, 3, 4, 5, 6, 7, 8};
  std::map<std::string, int> mm{{"c", 3}, {"a", 1}, {"b", 2}};

  out.writeln("110 item_sep       :", vv, PrintOptions{.item_sep = " ; "});
  out.writeln("111 kv_sep         :", mm, PrintOptions{.kv_sep = " -> "});
  out.writeln("112 list_brackets  :", vv,
              PrintOptions{.list_brackets = std::array<std::string, 2>{"<< ", " >>"}});
  out.writeln("113 map_brackets   :", mm,
              PrintOptions{.map_brackets = std::array<std::string, 2>{"{ ", " }"}});
  out.writeln("114 tuple_brackets :", std::tuple{1, 2, 3},
              PrintOptions{.tuple_brackets = std::array<std::string, 2>{"[|", "|]"}});
  out.writeln("115 pair_style Tuple :", std::pair{1, 2},
              PrintOptions{.pair_style = PairStyle::Tuple});
  out.writeln("116 pair_style Colon :", std::pair{1, 2},
              PrintOptions{.pair_style = PairStyle::Colon});
  out.writeln("117 pair_style Arrow :", std::pair{1, 2},
              PrintOptions{.pair_style = PairStyle::Arrow});
  out.writeln("118 pair_style Equals:", std::pair{1, 2},
              PrintOptions{.pair_style = PairStyle::Equals});
  out.writeln("119 max_items        :", vv, PrintOptions{.max_items = 3});
  out.writeln("120 max_items+verbose:", vv,
              PrintOptions{.max_items = 3, .more_verbose = true});
  out.writeln("121 max_depth 1      :",
              std::vector<std::vector<int>>{{1, 2}, {3, 4}},
              PrintOptions{.max_depth = 1});
  out.writeln("122 sort_keys        :", mm, PrintOptions{.sort_keys = true});
  out.writeln("123 show_indices     :",
              std::vector<std::string>{"zero", "one", "two"},
              PrintOptions{.show_indices = true});
  out.writeln("124 multiline + indent:",
              std::vector<int>{1, 2, 3},
              PrintOptions{.multiline = true, .indent = "  "});
  out.writeln("125 multiline map     :", mm,
              PrintOptions{.sort_keys = true, .multiline = true, .indent = "    "});
  out.writeln("126 multiline + trailing_comma:",
              std::vector<int>{1, 2},
              PrintOptions{.multiline = true, .trailing_comma = true});
  out.writeln("127 nested multiline  :",
              std::map<std::string, std::vector<int>>{{"a", {1, 2}}, {"b", {3, 4}}},
              PrintOptions{.sort_keys = true, .multiline = true});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 12 — PrintOptions: ANSI STYLING
  //  color_mode Auto only emits escapes on a real TTY; Always forces them so
  //  you can see the raw \x1b[...m sequences even in a pipe / capture.
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("128 color_mode Never (default off-tty):", "quiet",
              PrintOptions{.color_mode = ColorMode::Never,
                           .style = Style{.fg = Color::Red}});

  PrintOptions styled;
  styled.color_mode = ColorMode::Always;                   // force escapes on
  styled.style.fg = Color::Green;                          // 129. named foreground
  styled.style.bg = Color::Black;                          // 130. named background
  styled.style.bold = true;                                // 131. bold
  styled.style.dim = false;                                // 132. dim
  styled.style.italic = true;                              // 133. italic
  styled.style.underline = true;                           // 134. underline
  styled.style.blink = false;                              // 135. blink
  styled.style.reverse = false;                            // 136. reverse video
  styled.style.strike = true;                              // 137. strikethrough
  out.writeln("129-137 combined SGR:", "styled text", styled);

  PrintOptions rgb;
  rgb.color_mode = ColorMode::Always;
  rgb.style.fg_rgb = std::array<std::uint8_t, 3>{255, 128, 0};   // 138. 24-bit fg
  rgb.style.bg_rgb = std::array<std::uint8_t, 3>{16, 16, 64};    // 139. 24-bit bg
  out.writeln("138-139 truecolor:", "orange on navy", rgb);

  out.writeln("140 bright color:", "bright cyan",
              PrintOptions{.color_mode = ColorMode::Always,
                           .style = Style{.fg = Color::BrightCyan}});

  // SyntaxTheme is accepted; per-token colouring of structures is reserved.
  SyntaxTheme theme;
  theme.number = Style{.fg = Color::Cyan};
  theme.string = Style{.fg = Color::Yellow};
  theme.key = Style{.fg = Color::Magenta};
  out.writeln("141 syntax theme (reserved; struct is accepted today):",
              mm, PrintOptions{.color_mode = ColorMode::Always, .syntax = theme});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 13 — PrintOptions: FLUENT BUILDERS
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("142 with_sep :", 1, 2, 3, PrintOptions{}.with_sep(" / "));
  out.writeln("143 with_end :", "x", PrintOptions{}.with_end("!"));
  out.writeln("144 fixed(2) :", 3.14159, PrintOptions{}.fixed(2));
  out.writeln("145 sci(1)   :", 98765.0, PrintOptions{}.sci(1));
  out.writeln("146 hex()    :", 4095, PrintOptions{}.hex());
  out.writeln("147 hex(false):", 4095, PrintOptions{}.hex(false));
  out.writeln("148 percent(0):", 0.5, PrintOptions{}.percent(0));
  out.writelnf("149 pad(6,Right,'*'): [{}]", "hi", PrintOptions{}.pad(6, Align::Right, '*'));
  out.writeln("150 grouped():", 1234567, PrintOptions{}.grouped());
  out.writeln("151 grouped('_',2):", 12345678, PrintOptions{}.grouped('_', 2));
  out.writeln("152 colored():", "green",
              PrintOptions{}.colored(Style{.fg = Color::Green}));  // .style set; still needs a
                                                                  // TTY or color_mode=Always to show
  out.writeln("153 quoted():", std::vector<std::string>{"q"}, PrintOptions{}.quoted());
  out.writeln("154 upcase():", "shout", PrintOptions{}.upcase());
  out.writeln("155 pretty():",
              std::vector<int>{1, 2, 3}, PrintOptions{}.pretty("  "));
  out.writeln("156 capped(2):", vv, PrintOptions{}.capped(2));
  out.writeln("157 builder chain:", 1234567.5,
              PrintOptions{}.fixed(2).grouped(',').with_sep(" "));

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 14 — PrintOptions: PRESETS
  // ═════════════════════════════════════════════════════════════════════════
  out.writeln("158 plain()      :",
              std::vector<int>{1, 2, 3}, std::map<std::string, int>{{"a", 1}},
              PrintOptions::plain());
  out.writeln("159 pretty_json():",
              std::map<std::string, std::vector<int>>{{"b", {3, 4}}, {"a", {1, 2}}},
              PrintOptions::pretty_json());
  out.writeln("160 repr()       :", std::optional<std::string>{"hi"}, 255,
              PrintOptions::repr());
  out.writelnf("161 table_cell(10): [{}]", "name", PrintOptions::table_cell(10));
  out.writelnf("162 table_cell(10): [{}]", "an-overlong-name", PrintOptions::table_cell(10));

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 15 — THE FORMAT PATH: our own brace mini-language (no fmt)
  //  {}  auto-index      {N}  explicit index      {{ }}  literal braces
  //  {:[fill]align[sign][#][0][width][.prec][group][type]}
  //  types: d x X o b f F e E g G % s ?
  // ═════════════════════════════════════════════════════════════════════════
  out.writelnf("163 auto index      : {} {} {}", 'a', 'b', 'c');
  out.writelnf("164 explicit index  : {2} {1} {0}", 'a', 'b', 'c');
  out.writelnf("165 repeat an index : {0} {0} {1}", "x", "y");
  out.writelnf("166 literal braces  : {{ {} }}", "inside");
  out.writelnf("167 align <  ^  >   : [{:<8}][{:^8}][{:>8}]", "L", "C", "R");
  out.writelnf("168 fill char       : [{:*<8}][{:.^8}][{:0>8}]", "L", "C", "R");
  out.writelnf("169 sign + and space: {:+} {: } {:+}", 5, 7, -3);
  out.writelnf("170 base prefix #   : {:#x} {:#o} {:#b}", 255, 64, 10);
  out.writelnf("171 hex case        : {:x} vs {:X}", 3735928559U, 3735928559U);
  out.writelnf("172 zero-pad + prec : {:08.2f}", 3.14159);
  out.writelnf("173 fixed / sci / g : {:.3f} | {:.2e} | {:.4g}", 12345.678, 12345.678, 12345.678);
  out.writelnf("174 percent         : {:.1%}", 0.8137);
  out.writelnf("175 grouping in spec: {:,} | {:_}", 1234567, 89012345);
  out.writelnf("176 integer type d  : {:d}", 1000);
  out.writelnf("177 quoted ?        : {:?} {:?}", "text", std::string("std"));
  out.writelnf("178 spec width+trunc: [{:>6}]", "toolongvalue");   // width honoured; truncate needs PrintOptions

  // ── format + trailing PrintOptions ──────────────────────────────────────
  out.writelnf("179 bare {} uses the call spec, {:.0f} keeps its own:",
               3.14159, 2.71828,
               PrintOptions{}.fixed(2));                   // -> 3.14 and 3 (spec wins for the 2nd)
  out.writelnf("180 envelope on a formatted line -> {} of {}",
               3, 10, PrintOptions{.prefix = "  * ", .suffix = " done"});

  // ── pack(...) : one bundle, so a trailing PrintOptions is never ambiguous ─
  out.writelnf("181 pack + no opts   : {} @ {}", pack(std::string("widget"), 19.99));
  out.writelnf("182 pack + opts      : {} @ {}", pack(std::string("widget"), 19.99),
               PrintOptions{}.fixed(2).grouped(','));
  auto reusable = pack(7, std::string("seven"), 7.0);      // 183. build once...
  out.writelnf("183a reuse pack -> [{} {} {}]", reusable);  //      ...use across patterns
  out.writelnf("183b reuse pack -> {2}/{1}/{0}", reusable);

  // ── missing_arg + strict_args ──────────────────────────────────────────
  out.writelnf("184 missing arg -> {} {} {}", "only", "two",
               PrintOptions{.missing_arg = "<?>"});
  try {
    out.writelnf("185 strict_args -> {} {}", 1, 2, 3,       // 3 args, 2 placeholders
                 PrintOptions{.strict_args = true});
  } catch (const std::exception& e) {
    out.writeln("185 strict_args threw:", e.what());
  }

  // ── logf with a trailing PrintOptions ─────────────────────────────────
  out.warnf("186 warnf + spec -> disk at {} on {}", 0.92, std::string("/data"),
            PrintOptions{.notation = Notation::Percent, .precision = 0});

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 16 — SINKS: cerr, capture-to-string, and mixing engines
  // ═════════════════════════════════════════════════════════════════════════
  err.writeln("187 this line is on std::cerr");
  cap.writeln("188 captured:", std::vector<int>{1, 2, 3},
              PrintOptions{.multiline = true});
  out.writeln("189 the capture buffer contains:\n" + capture.str());

  // ═════════════════════════════════════════════════════════════════════════
  //  SECTION 17 — PRECEDENCE: engine baseline  <  per-call PrintOptions
  // ═════════════════════════════════════════════════════════════════════════
  out.print_defaults(PrintOptions{}.grouped('_').fixed(1));  // house style
  out.writeln("190 baseline only        :", 1234567.25);     // -> 1_234_567.2
  out.writeln("191 per-call overrides it :", 1234567.25,
              PrintOptions{}.grouped(',').fixed(3));          // -> 1,234,567.250
  out.writeln("192 per-call partial merge:", 1234567.25,
              PrintOptions{}.fixed(0));                       // grouping still '_' from baseline
  out.print_defaults(PrintOptions{});                         // clear house style

  out.writeln("── end of tour ──");
  return 0;
}
