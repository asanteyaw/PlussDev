#include <pluss/coden.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void require(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string(message));
}

void show_offsets(const pluss::TokenBatch& batch) {
  std::cout << "offsets=[";
  const auto offsets = batch.offsets().integers();
  for (std::size_t i = 0; i < offsets.size(); ++i)
    std::cout << (i ? ", " : "") << offsets[i];
  std::cout << "]\n";
}

bool same(const pluss::Note& lhs, const pluss::Note& rhs) {
  return (lhs == rhs).all().bool_item();
}

} // namespace

int main() {
  std::cout << "=== add math delimiters ===\n";
  const auto expressions = pluss::make_note(
      {" x+y ", "$z$", R"(\(a+b\))", R"(\[c=d\])", "", "$u", R"(v\))"});
  const auto wrapped = expressions.tex().add_math_delimiters();
  std::cout << wrapped << '\n';
  require(wrapped.sizes() == expressions.sizes(), "add delimiters changed shape");
  require(same(wrapped.tex().add_math_delimiters(), wrapped),
          "add delimiters is not idempotent");

  const auto grid = pluss::make_note(
      std::vector<std::string>{"a", "b", "$c$", ""},
      std::array<std::int64_t, 2>{2, 2});
  std::cout << grid.tex().add_math_delimiters() << '\n';
  std::cout << std::boolalpha << "undefined="
            << pluss::Note::undefined().tex().add_math_delimiters().defined()
            << '\n';
  try {
    (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
        .tex().add_math_delimiters();
  } catch (const pluss::Error&) {
    std::cout << "Int64 add delimiters: rejected\n";
  }

  std::cout << "\n=== align blocks ===\n";
  const auto documents = pluss::make_note({
      R"(before \begin{align*}a&=1\end{align*} middle \begin{align}b&=2\end{align})",
      "no alignment here",
      R"(\begin{align}c&=3\end{align})"});
  const auto align = documents.tex().align_blocks();
  std::cout << align.tokens() << '\n';
  show_offsets(align);
  require(align.tokens().at(0).find("align*") != std::string_view::npos,
          "align blocks lost document order");
  const auto align_offsets = align.offsets().integers();
  require(align_offsets.size() == 4 && align_offsets[0] == 0 &&
              align_offsets[1] == 2 && align_offsets[2] == 2 &&
              align_offsets[3] == 3,
          "align offsets differ");

  std::cout << "\n=== balance braces ===\n";
  const auto braces = pluss::make_note(
      {"{a{b}", "a}}b", R"(\{literal\})", "already {balanced}"});
  const auto braces_balanced = braces.tex().balance_braces();
  std::cout << braces_balanced << '\n';
  const bool braces_idempotent =
      same(braces_balanced.tex().balance_braces(), braces_balanced);
  std::cout << "idempotent=" << braces_idempotent << " undefined="
            << pluss::Note::undefined().tex().balance_braces().defined() << '\n';
  require(braces_idempotent,
          "brace balancing is not idempotent");

  std::cout << "\n=== balance math delimiters ===\n";
  const auto delimiters = pluss::make_note(
      {"$x+y", R"(\(a+b)", R"(c+d\))", R"(\[e=f)", R"(g=h\])", R"(\$5)"});
  const auto balanced = delimiters.tex().balance_math_delims();
  std::cout << balanced << '\n';
  const bool math_idempotent =
      same(balanced.tex().balance_math_delims(), balanced);
  std::cout << "idempotent=" << math_idempotent << " undefined="
            << pluss::Note::undefined().tex().balance_math_delims().defined()
            << '\n';
  require(math_idempotent,
          "math delimiter balancing is not idempotent");

  std::cout << "\n=== bibliography keys ===\n";
  const auto sources = pluss::make_note({
      R"(See \cite{knuth84, lamport94}; \bibitem{turing36}; \citep[sec. 2]{knuth84}.)",
      "no references",
      R"(\bibitem[Church]{church36})"});
  const auto keys = sources.tex().bibliography_keys();
  std::cout << keys.tokens() << '\n';
  show_offsets(keys);
  require(keys.tokens().numel() == 5, "bibliography key count differs");
  require(keys.tokens().at(1) == "lamport94", "citation list was not split");

  const auto absent = pluss::Note::undefined().tex().bibliography_keys();
  std::cout << "undefined tokens=" << absent.tokens().defined() << ' ';
  show_offsets(absent);

  std::cout << "\n=== canonicalize macros ===\n";
  const auto macro_spellings = pluss::make_note({
      R"(\dfrac{1}{2} + \tfrac{a}{b} + \cfrac{x}{y})",
      R"(\operatorname{rank} \rm old \bf b \it i \cal C \Bbb R)",
      R"(\mathrm{kept} \mathbf{v} \item \ltd \leqslant \lt \gt)"});
  const auto canonical = macro_spellings.tex().canonicalize_macros();
  std::cout << canonical << '\n';
  const bool canonical_idempotent =
      same(canonical.tex().canonicalize_macros(), canonical);
  std::cout << "idempotent=" << canonical_idempotent << '\n';
  require(canonical.at(2).find(R"(\mathrm{kept})") == 0,
          "canonicalization corrupted an existing canonical command");
  require(canonical.at(2).find(R"(\item)") != std::string_view::npos,
          "canonicalization rewrote a command-name prefix");
  require(canonical_idempotent, "macro canonicalization is not idempotent");

  std::cout << "\n=== captions ===\n";
  const auto caption_source = pluss::make_note(
      std::vector<std::string>{
          R"(\caption{Long title} \caption*[Short]{Starred long})",
          "no caption", R"(\caption[TOC title]{Displayed title})",
          R"(\caption{Nested \textbf{title}})"},
      std::array<std::int64_t, 2>{2, 2});
  const auto captions = caption_source.tex().captions();
  std::cout << captions.tokens() << '\n';
  show_offsets(captions);
  require(captions.tokens().numel() == 3, "caption count differs");
  require(captions.tokens().at(2) == "Displayed title",
          "caption optional argument selected instead of long caption");

  std::cout << "\n=== citations ===\n";
  const auto citation_source = pluss::make_note({
      R"(\cite{a,b} \citep[see][p. 4]{c,d})", R"(\citet*{e})",
      "no citation"});
  const auto citations = citation_source.tex().citations();
  std::cout << citations.tokens() << '\n';
  show_offsets(citations);
  require(citations.tokens().at(0) == "a,b",
          "citations unexpectedly split a command argument");
  require(citations.tokens().at(1) == "c,d",
          "citation with two optional notes was not extracted");

  std::cout << "\n=== collapse macros ===\n";
  const auto macro_documents = pluss::make_note({
      "  \\newcommand{\\RR}{\\mathbb{R}}\n"
      "  \\dfrac{1}{2}   in   \\RR  ",
      R"(\def\pair#1#2{{#1,#2}}
          \tfrac{x}{y})"});
  const auto collapsed = macro_documents.tex().collapse_macros();
  std::cout << collapsed << '\n';
  require(collapsed.at(0) == R"(\frac{1}{2} in \RR)",
          "collapse macros changed use sites or retained a declaration");
  require(collapsed.at(1) == R"(\frac{x}{y})",
          "collapse macros did not remove a nested def body");

  std::cout << "\n=== command names ===\n";
  const auto command_source = pluss::make_note({
      R"(\alpha + \section*{Intro} + \foo@bar)",
      R"(line break: \\alpha; symbols: \$ and \%; then \beta)",
      "plain text"});
  const auto names = command_source.tex().command_names();
  std::cout << names.tokens() << '\n';
  show_offsets(names);
  require(names.tokens().numel() == 4, "command name count differs");
  require(names.tokens().at(3) == "beta",
          "escaped linebreak was parsed as a named command");

  const auto no_tex = pluss::Note::undefined().tex().command_names();
  std::cout << "undefined names=" << no_tex.tokens().defined() << ' ';
  show_offsets(no_tex);

  std::cout << "\n=== commands ===\n";
  const auto command_calls = pluss::make_note({
      R"(\section* [Short] {A {nested} title} and \alpha)",
      R"(line break: \\beta; escaped: \$; malformed: \frac{one)",
      "plain text"});
  const auto bare_commands = command_calls.tex().commands();
  const auto explicit_bare_commands = command_calls.tex().commands(false);
  const auto complete_commands = command_calls.tex().commands(true);
  std::cout << "bare=" << bare_commands.tokens() << '\n';
  show_offsets(bare_commands);
  std::cout << "with args=" << complete_commands.tokens() << '\n';
  show_offsets(complete_commands);
  require(same(bare_commands.tokens(), explicit_bare_commands.tokens()),
          "commands(false) differs from commands()");
  require(bare_commands.tokens().numel() == 3,
          "commands parsed a control symbol as a control word");
  require(complete_commands.tokens().at(0) ==
              R"(\section* [Short] {A {nested} title})",
          "commands(true) lost a balanced attached group");

  std::cout << "\n=== convert fractions ===\n";
  const auto fraction_forms = pluss::make_note(
      R"(\frac{1}{2} + \dfrac{a+b}{c} + \tfrac{m}{n} + \cfrac{p}{q})");
  std::cout << fraction_forms.tex().frac_slash() << '\n';
  const auto fractions = pluss::make_note({
      R"(\frac{1}{2} + \dfrac{a+b}{c} + \cfrac{\frac{x}{y}}{z})",
      R"(\fraction{1}{2} + \\frac{3}{4} + \frac{broken})"});
  const auto slash_fractions = fractions.tex().frac_slash();
  std::cout << slash_fractions << '\n';
  require(slash_fractions.at(0) ==
              "(1)/(2) + (a+b)/(c) + ((x)/(y))/(z)",
          "nested fraction conversion differs");
  require(slash_fractions.at(1) == fractions.at(1),
          "fraction conversion changed a prefix, linebreak, or malformed call");
  const auto fraction_grid = pluss::make_note(
      std::vector<std::string>{R"(\frac{a}{b})", "plain",
                               R"(\tfrac{x}{y})", ""},
      std::array<std::int64_t, 2>{2, 2});
  std::cout << fraction_grid.tex().frac_slash() << '\n';

  std::cout << "\n=== convert plus-minus ===\n";
  const auto signs = pluss::make_note(
      {R"(x \pm e, y \mp d)", R"(\pmatrix and \\pm stay TeX)"});
  const auto plain_signs = signs.tex().pm_signs();
  std::cout << plain_signs << '\n';
  require(plain_signs.at(0) == "x +/- e, y -/+ d",
          "plus-minus conversion differs");
  require(plain_signs.at(1) == signs.at(1),
          "plus-minus conversion changed a command prefix or linebreak");
  const auto sign_grid = pluss::make_note(
      std::vector<std::string>{R"(a\pm b)", "plain", R"(c\mp d)", ""},
      std::array<std::int64_t, 2>{2, 2});
  std::cout << sign_grid.tex().pm_signs() << '\n';

  std::cout << "\n=== convert roots ===\n";
  std::cout << pluss::make_note({R"(\sqrt{x})", R"(\sqrt[3]{a+b})"})
                   .tex().sqrt_pow()
            << '\n';
  const auto roots = pluss::make_note({
      R"(\sqrt{x} + \sqrt[3]{a+\sqrt{b}})",
      R"(\sqrted{x} + \\sqrt{x} + \sqrt[3]oops)"});
  const auto powers = roots.tex().sqrt_pow();
  std::cout << powers << '\n';
  require(powers.at(0) == "(x)^(1/2) + (a+(b)^(1/2))^(1/3)",
          "nested root conversion differs");
  require(powers.at(1) == roots.at(1),
          "root conversion changed a prefix, linebreak, or malformed call");
  const auto root_grid = pluss::make_note(
      std::vector<std::string>{R"(\sqrt{a})", "plain",
                               R"(\sqrt[4]{b})", ""},
      std::array<std::int64_t, 2>{2, 2});
  std::cout << root_grid.tex().sqrt_pow() << '\n';

  std::cout << "\n=== dedup blank lines ===\n";
  std::cout << pluss::make_note("alpha\n\n\n\nbeta")
                   .tex().dedup_blank_lines()
            << '\n';
  std::cout << pluss::make_note("one\n  \n\t\n\n\ntwo")
                   .tex().dedup_blank_lines()
            << '\n';
  std::cout << pluss::make_note("one\r\n \r\n\t\r\ntwo")
                   .tex().dedup_blank_lines()
            << '\n';
  const auto spaced = pluss::make_note(
      std::vector<std::string>{"alpha\n\n\n\nbeta", "one\r\n \r\n\t\r\ntwo",
                               "already\n\ncompact", "single line"},
      std::array<std::int64_t, 2>{2, 2});
  const auto compact = spaced.tex().dedup_blank_lines();
  std::cout << compact << '\n';
  std::cout << "shape=[" << compact.sizes()[0] << ", "
            << compact.sizes()[1] << "]\n";
  require(compact.at(0) == "alpha\n\nbeta",
          "LF blank lines were not collapsed");
  require(compact.at(1) == "one\r\n\r\ntwo",
          "CRLF blank lines were not preserved and collapsed");
  std::cout << pluss::make_note("one\r\n \r\n\t\r\ntwo")
                   .tex().normalize_newlines().tex().dedup_blank_lines()
            << '\n';

  const auto undefined_tex = pluss::Note::undefined();
  std::cout << "undefined transforms="
            << undefined_tex.tex().frac_slash().defined() << ','
            << undefined_tex.tex().pm_signs().defined() << ','
            << undefined_tex.tex().sqrt_pow().defined() << ','
            << undefined_tex.tex().dedup_blank_lines().defined() << '\n';

  std::cout << "\n=== display math ===\n";
  const auto math_source = pluss::make_note({
      R"(inline $a$ then $$b+c$$ and \[d=e\])",
      R"($$first$$ between $$second$$)",
      R"(plain text and \(inline\))"});
  const auto display = math_source.tex().display_math();
  std::cout << display.tokens() << '\n';
  show_offsets(display);
  require(display.tokens().numel() == 4, "display math count differs");

  std::cout << "\n=== environment names ===\n";
  const auto env_source = pluss::make_note({
      R"(\begin{figure}A\end{figure} \begin{align*}x&=1\end{align*})",
      R"(\begin{table}B\end{table})", "no environment"});
  const auto env_names_result = env_source.tex().env_names();
  std::cout << env_names_result.tokens() << '\n';
  show_offsets(env_names_result);
  const auto partial_names = pluss::make_note(
      {R"(\begin{proof}unfinished)", R"(\end{proof})"})
      .tex().env_names();
  std::cout << partial_names.tokens() << '\n';

  std::cout << "\n=== environments ===\n";
  const auto envs = env_source.tex().envs();
  std::cout << envs.tokens() << '\n';
  show_offsets(envs);
  require(envs.tokens().numel() == env_names_result.tokens().numel(),
          "environment and name counts differ");
  const auto partial_envs = pluss::make_note(
      {R"(\begin{proof}unfinished)", R"(\end{proof})"})
      .tex().envs();
  std::cout << partial_envs.tokens() << '\n';
  show_offsets(partial_envs);

  std::cout << "\n=== input tags ===\n";
  const auto input_source = pluss::make_note(
      std::vector<std::string>{R"(Before \input{chapters/one} after)",
                               R"(\include {appendix} and \\input{literal})",
                               R"(\input{nested/{part}})", R"(\input broken)"},
      std::array<std::int64_t, 2>{2, 2});
  const auto tagged = input_source.tex().input_tags();
  std::cout << tagged << '\n';
  std::cout << "shape=[" << tagged.sizes()[0] << ", " << tagged.sizes()[1]
            << "]\n";
  require(tagged.at(1).find(R"(\\input{literal})") != std::string_view::npos,
          "escaped input command was rewritten");
  std::cout << pluss::make_note(
                   {R"(Before \input{chapters/one} after)",
                    R"(\include {appendix})"})
                   .tex().input_tags()
            << '\n';
  std::cout << pluss::make_note(
                   {R"(\\input{literal})", R"(\input broken)",
                    R"(\input{nested/{part}})"})
                   .tex().input_tags()
            << '\n';

  std::cout << "\n=== macro substitutions ===\n";
  const auto macro_source = pluss::make_note(
      R"(\newcommand{\A}{\B}\newcommand{\B}{done}Value: \A; prefix: \Alpha.)");
  const auto one_pass = macro_source.tex().macro_subs();
  const auto two_passes = macro_source.tex().macro_subs(2);
  std::cout << "one pass: " << one_pass << '\n';
  std::cout << "two passes: " << two_passes << '\n';
  require(one_pass.at(0).find(R"(\B)") != std::string_view::npos,
          "one macro pass expanded more than one level");
  require(two_passes.at(0).find("done") != std::string_view::npos,
          "two macro passes did not expand the chain");
  std::cout << pluss::make_note({
                   R"(\newcommand{\unit}{kg}mass: \unit)",
                   R"(\newcommand{\unit}{m}length: \unit)"})
                   .tex().macro_subs()
            << '\n';

  const auto tex_absent = pluss::Note::undefined();
  std::cout << std::boolalpha << "undefined="
            << tex_absent.tex().display_math().tokens().defined() << ','
            << tex_absent.tex().env_names().tokens().defined() << ','
            << tex_absent.tex().envs().tokens().defined() << ','
            << tex_absent.tex().input_tags().defined() << ','
            << tex_absent.tex().macro_subs().defined() << '\n';
  try {
    (void)macro_source.tex().macro_subs(0);
  } catch (const pluss::Error&) {
    std::cout << "macro_subs(0): rejected\n";
  }

  const auto ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  for (const auto [label, operation] : std::array{
           std::pair{"align", 0}, std::pair{"braces", 1},
           std::pair{"math", 2}, std::pair{"bibliography", 3},
           std::pair{"canonical", 4}, std::pair{"captions", 5},
           std::pair{"citations", 6}, std::pair{"collapse", 7},
           std::pair{"command names", 8}, std::pair{"commands", 9},
           std::pair{"fractions", 10}, std::pair{"plus-minus", 11},
           std::pair{"roots", 12}, std::pair{"blank lines", 13},
           std::pair{"display math", 14}, std::pair{"environment names", 15},
           std::pair{"environments", 16}, std::pair{"input tags", 17},
           std::pair{"macro substitutions", 18}}) {
    try {
      if (operation == 0) (void)ints.tex().align_blocks();
      if (operation == 1) (void)ints.tex().balance_braces();
      if (operation == 2) (void)ints.tex().balance_math_delims();
      if (operation == 3) (void)ints.tex().bibliography_keys();
      if (operation == 4) (void)ints.tex().canonicalize_macros();
      if (operation == 5) (void)ints.tex().captions();
      if (operation == 6) (void)ints.tex().citations();
      if (operation == 7) (void)ints.tex().collapse_macros();
      if (operation == 8) (void)ints.tex().command_names();
      if (operation == 9) (void)ints.tex().commands(true);
      if (operation == 10) (void)ints.tex().frac_slash();
      if (operation == 11) (void)ints.tex().pm_signs();
      if (operation == 12) (void)ints.tex().sqrt_pow();
      if (operation == 13) (void)ints.tex().dedup_blank_lines();
      if (operation == 14) (void)ints.tex().display_math();
      if (operation == 15) (void)ints.tex().env_names();
      if (operation == 16) (void)ints.tex().envs();
      if (operation == 17) (void)ints.tex().input_tags();
      if (operation == 18) (void)ints.tex().macro_subs();
    } catch (const pluss::Error&) {
      std::cout << label << " Int64: rejected\n";
    }
  }
}
