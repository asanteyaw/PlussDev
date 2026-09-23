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

void show_shape(const pluss::Note& note) {
  std::cout << "shape=[";
  const auto sizes = note.sizes();
  for (std::size_t i = 0; i < sizes.size(); ++i)
    std::cout << (i ? ", " : "") << sizes[i];
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
                   .tex().line_norm().tex().dedup_blank_lines()
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

  std::cout << "\n=== command arguments ===\n";
  const auto arg_source = pluss::make_note(
      std::vector<std::string>{
          R"(\caption[Short]{A {nested} title} \caption{Second})",
          R"(\captioned{not a caption} \\caption{escaped})",
          R"(\caption {Third})", R"(\caption[broken{ignored})"},
      std::array<std::int64_t, 2>{2, 2});
  const auto args = arg_source.tex().args("caption");
  std::cout << args.tokens() << '\n';
  show_offsets(args);
  std::cout << "shape=[" << arg_source.sizes()[0] << ", "
            << arg_source.sizes()[1] << "]\n";
  require(args.tokens().numel() == 3, "argument count differs");

  std::cout << "\n=== named environment ===\n";
  const auto env_source_nested = pluss::make_note({
      R"(\begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof})",
      R"(\begin{lemma}other\end{lemma})", R"(\begin{proof}unfinished)",
      R"(\\begin{proof}escaped\end{proof})"});
  const auto proofs = env_source_nested.tex().env("proof");
  std::cout << proofs.tokens() << '\n';
  show_offsets(proofs);
  require(proofs.tokens().numel() == 1, "nested environment count differs");

  std::cout << "\n=== math extractors ===\n";
  const auto math_extract_source = pluss::make_note(
      std::vector<std::string>{
          R"(prose 99; $x_1 + \sin(\alpha) = 2.5$)",
          R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
          R"(escaped \$7 and plain theta)",
          R"($$\argmax_x f(x) \approx 10$$)"},
      std::array<std::int64_t, 2>{2, 2});
  const auto funcs = math_extract_source.tex().math_funcs();
  const auto greek = math_extract_source.tex().greek();
  const auto ids = math_extract_source.tex().ids();
  const auto nums = math_extract_source.tex().nums();
  const auto ops = math_extract_source.tex().ops();
  const auto symbols = math_extract_source.tex().symbols();
  std::cout << "math_funcs: " << funcs.tokens() << '\n';
  show_offsets(funcs);
  std::cout << "greek: " << greek.tokens() << '\n';
  show_offsets(greek);
  std::cout << "ids: " << ids.tokens() << '\n';
  show_offsets(ids);
  std::cout << "nums: " << nums.tokens() << '\n';
  show_offsets(nums);
  std::cout << "ops: " << ops.tokens() << '\n';
  show_offsets(ops);
  std::cout << "symbols: " << symbols.tokens() << '\n';
  show_offsets(symbols);
  require(nums.tokens().numel() == 3, "math-only number count differs");
  require(greek.tokens().numel() == 2, "Greek command count differs");

  std::cout << "\n=== figures ===\n";
  const auto figure_source = pluss::make_note({
      R"(\begin{figure*}wide\end{figure*} then \begin{figure}plain\end{figure})",
      R"(\begin{figure}outer \begin{figure}inner\end{figure} tail\end{figure})",
      R"(\begin{figure}unfinished)", R"(\\begin{figure}escaped\end{figure})"});
  const auto figures = figure_source.tex().figures();
  std::cout << figures.tokens() << '\n';
  show_offsets(figures);
  require(figures.tokens().numel() == 3, "figure count differs");
  require(figures.tokens().at(0).find("figure*") != std::string_view::npos,
          "figure variants lost source order");

  std::cout << "\n=== fix linebreaks ===\n";
  const auto rough_lines = pluss::make_note(
      std::vector<std::string>{"alpha  \r\n\r\n \r\n\r\nbeta\t \r",
                               "already\n\nclean", "tail   \n", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto fixed_lines = rough_lines.tex().fix_linebreaks();
  std::cout << fixed_lines << '\n';
  std::cout << "idempotent="
            << same(fixed_lines.tex().fix_linebreaks(), fixed_lines) << '\n';
  require(fixed_lines.sizes() == rough_lines.sizes(),
          "linebreak repair changed shape");
  require(same(fixed_lines.tex().fix_linebreaks(), fixed_lines),
          "linebreak repair is not idempotent");

  std::cout << "\n=== footnotes ===\n";
  const auto footnote_source = pluss::make_note({
      R"(Text\footnote{first} and \footnote[7]{outer {nested} note}.)",
      R"(\footnoted{not a footnote} \\footnote{escaped})",
      R"(\footnote{unfinished)"});
  const auto footnotes = footnote_source.tex().footnotes();
  std::cout << footnotes.tokens() << '\n';
  show_offsets(footnotes);
  require(footnotes.tokens().numel() == 2, "footnote count differs");

  std::cout << "\n=== headings ===\n";
  const auto heading_source = pluss::make_note(
      std::vector<std::string>{
          R"(\part{Front} \chapter{Start} \section[Short]{Top {title}})",
          R"(\subsection*{Detail} \subsubsection{Deep} \paragraph{Leaf})",
          R"(\sectioned{not a heading} \\section{escaped})", "plain"},
      std::array<std::int64_t, 2>{2, 2});
  for (const std::int64_t depth : {1, 2, 4}) {
    const auto headings = heading_source.tex().headings(depth);
    std::cout << "depth=" << depth << ' ' << headings.tokens() << '\n';
    show_offsets(headings);
  }
  try {
    (void)heading_source.tex().headings(0);
  } catch (const pluss::Error&) {
    std::cout << "headings(0): rejected\n";
  }
  try {
    (void)heading_source.tex().headings(5);
  } catch (const pluss::Error&) {
    std::cout << "headings(5): rejected\n";
  }

  std::cout << "\n=== includes ===\n";
  const auto include_source = pluss::make_note({
      R"(\input{chapters/{intro}} \includegraphics[width={.5\textwidth}]{plot.pdf})",
      R"(\include{appendix} \inputted{no} \\input{escaped})",
      R"(\input{unfinished)"});
  const auto includes = include_source.tex().includes();
  std::cout << includes.tokens() << '\n';
  show_offsets(includes);
  require(includes.tokens().numel() == 3, "include count differs");

  std::cout << "\n=== inline math ===\n";
  const auto inline_source = pluss::make_note(
      std::vector<std::string>{
          R"(price \$5; $x+\$y$ then $$display$$ and \(z\))",
          "\\[block\\] and $line\nbreak$", R"(\(unfinished)", "plain"},
      std::array<std::int64_t, 2>{2, 2});
  const auto inline_math = inline_source.tex().inline_math();
  std::cout << inline_math.tokens() << '\n';
  show_offsets(inline_math);
  require(inline_math.tokens().numel() == 2, "inline math count differs");

  std::cout << "\n=== labels ===\n";
  const auto label_source = pluss::make_note({
      R"(\label{eq:{energy}} \labeled{no})",
      R"(\\label{escaped} \label {sec:intro})", R"(\label{unfinished)"});
  const auto labels = label_source.tex().labels();
  std::cout << labels.tokens() << '\n';
  show_offsets(labels);
  require(labels.tokens().numel() == 2, "label count differs");

  std::cout << "\n=== macros defined ===\n";
  const auto declaration_source = pluss::make_note({
      R"(\newcommand{\RR}{\mathbb{R}} \renewcommand*\vecx[1]{\mathbf{#1}})",
      R"(\providecommand{\unit}[1][kg]{#1} \def\pair#1#2{#1,#2})",
      R"(\\newcommand{\escaped}{x} \newcommand{\broken})"});
  const auto defined_macros = declaration_source.tex().macro_defs();
  std::cout << defined_macros.tokens() << '\n';
  show_offsets(defined_macros);
  require(defined_macros.tokens().numel() == 4,
          "defined macro count differs");

  std::cout << "\n=== macros used ===\n";
  const auto use_source = pluss::make_note({
      R"(\alpha \beta \alpha \\gamma \section* \section)",
      R"(\beta \delta \beta)"});
  const auto used_macros = use_source.tex().macro_use();
  std::cout << used_macros.tokens() << '\n';
  show_offsets(used_macros);
  require(used_macros.tokens().numel() == 6, "used macro count differs");

  std::cout << "\n=== math blocks ===\n";
  const auto all_math = inline_source.tex().math_blocks(-1);
  const auto first_math = inline_source.tex().math_blocks(1);
  const auto no_math = inline_source.tex().math_blocks(0);
  std::cout << "all: " << all_math.tokens() << '\n';
  show_offsets(all_math);
  std::cout << "first: " << first_math.tokens() << '\n';
  show_offsets(first_math);
  std::cout << "zero: " << no_math.tokens() << '\n';
  show_offsets(no_math);
  require(all_math.tokens().numel() == 4, "all math count differs");
  require(first_math.tokens().numel() == 2, "per-cell math limit differs");
  require(no_math.tokens().numel() == 0, "zero math limit differs");
  try {
    (void)inline_source.tex().math_blocks(-2);
  } catch (const pluss::Error&) {
    std::cout << "math_blocks(-2): rejected\n";
  }

  std::cout << "\n=== math only ===\n";
  const auto projection_source = pluss::make_note(
      std::vector<std::string>{
          R"(price \$5, $x+\$y$, $$z$$, and \(q\))",
          "\\[a+b\\] then $line\nbreak$", R"(\(unfinished)", "plain"},
      std::array<std::int64_t, 2>{2, 2});
  const auto projected_math = projection_source.tex().math_only();
  std::cout << projected_math << '\n';
  std::cout << "projected shape=[" << projected_math.sizes()[0] << ", "
            << projected_math.sizes()[1] << "]\n";
  std::cout << projection_source.tex().math_blocks(-1).tokens() << '\n';
  require(projected_math.sizes() == projection_source.sizes(),
          "math-only projection changed shape");

  std::cout << "\n=== line norm ===\n";
  const auto mixed_lines = pluss::make_note(
      std::vector<std::string>{"a\r\nb\rc\n", "stable", "", "x\r"},
      std::array<std::int64_t, 2>{2, 2});
  const auto normalized_lines = mixed_lines.tex().line_norm();
  std::cout << normalized_lines << '\n';
  const bool line_shape = normalized_lines.sizes() == mixed_lines.sizes();
  const bool line_idempotent =
      same(normalized_lines.tex().line_norm(), normalized_lines);
  std::cout << "same_shape=" << line_shape
            << " idempotent=" << line_idempotent << '\n';
  require(line_shape,
          "line normalization changed shape");
  require(line_idempotent,
          "line normalization is not idempotent");

  std::cout << "\n=== packages ===\n";
  const auto package_source = pluss::make_note({
      R"(\usepackage[margin={1in,2in}]{geometry, xcolor})",
      R"(\RequirePackage {amsmath} \RequirePackageWithOptions{hyperref})",
      R"(\\usepackage{escaped} \usepackages{long} \usepackage{, })"});
  const auto package_names = package_source.tex().packages();
  std::cout << package_names.tokens() << '\n';
  show_offsets(package_names);
  require(package_names.tokens().numel() == 4, "package count differs");

  std::cout << "\n=== refs ===\n";
  const auto ref_source = pluss::make_note({
      R"(\ref{eq:{energy}} \eqref* {eq:two} \cref{sec:a})",
      R"(\Cref{sec:b} \vref{fig:a} \Vref{fig:b} \nameref{name:a})",
      R"(\\ref{escaped} \reference{long} \ref{unfinished)"});
  const auto ref_keys = ref_source.tex().refs();
  std::cout << ref_keys.tokens() << '\n';
  show_offsets(ref_keys);
  require(ref_keys.tokens().numel() == 7, "reference count differs");

  std::cout << "\n=== deaccent ===\n";
  const auto accented = pluss::make_note(
      std::vector<std::string>{R"(\'{e} \v S \c{c} \t{oo} \text{keep})",
                               "caf\xC3\xA9 d\xC3\xA9j\xC3\xA0",
                               R"(\\'{e} \unknown{x})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto deaccented = accented.tex().deaccent();
  std::cout << deaccented << '\n';
  std::cout << "shape=[" << deaccented.sizes()[0] << ", "
            << deaccented.sizes()[1] << "] dtype="
            << pluss::coden::dtype_name(deaccented.dtype()) << '\n';
  const auto accent_scalar = pluss::make_note(R"(\'{e})");
  std::cout << accent_scalar << '\n' << accent_scalar.tex().deaccent() << '\n';
  require(deaccented.sizes() == accented.sizes(),
          "deaccenting changed shape");

  std::cout << "\n=== strip bib ===\n";
  const auto bib_source = pluss::make_note({
      R"(Before \bibliography{refs/{main}} middle \printbibliography[heading={none}] after)",
      R"(A \begin{thebibliography}{9} one \begin{thebibliography}{1} nested \end{thebibliography} two \end{thebibliography} Z)",
      R"(\\bibliography{escaped} \bibliography{unfinished)"});
  std::cout << bib_source.tex().strip_bib() << '\n';
  const auto bib_grid = pluss::make_note(
      std::vector<std::string>{R"(A \addbibresource{refs/a.bib} B)",
                               R"(\printbibliography)", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto bib_grid_clean = bib_grid.tex().strip_bib();
  std::cout << bib_grid_clean << '\n';
  show_shape(bib_grid_clean);
  std::cout << "receiver unchanged=" << (bib_grid.at(0).find("addbibresource") != std::string_view::npos)
            << " idempotent=" << same(bib_grid_clean.tex().strip_bib(), bib_grid_clean) << '\n';

  std::cout << "\n=== strip size ===\n";
  const auto size_source = pluss::make_note({
      R"(\Bigl( x \Bigr) + \bigg[y\bigg])",
      R"(\\Bigl literal \BiglExtra keep)", "plain"});
  std::cout << size_source.tex().strip_size() << '\n';
  const auto size_grid = pluss::make_note(
      std::vector<std::string>{R"(\bigl\{a\bigr\})", R"(\Bigg|x\Bigg|)",
                               R"(\left(y\right))", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto size_grid_clean = size_grid.tex().strip_size();
  std::cout << size_grid_clean << '\n';
  show_shape(size_grid_clean);
  std::cout << "idempotent=" << same(size_grid_clean.tex().strip_size(), size_grid_clean) << '\n';

  std::cout << "\n=== strip cites ===\n";
  const auto cite_source = pluss::make_note({
      R"(A \citep[see][p.~2]{alpha,{beta}} B \cite*{gamma} C)",
      R"(\\cite{escaped} \cite{unfinished)"});
  std::cout << cite_source.tex().strip_cites() << '\n';
  const auto cite_grid = pluss::make_note(
      std::vector<std::string>{R"(Lead \citet{a} tail)", R"(\citeauthor*{b})",
                               "uncited prose", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto cite_grid_clean = cite_grid.tex().strip_cites();
  std::cout << cite_grid_clean << '\n';
  show_shape(cite_grid_clean);
  std::cout << "idempotent=" << same(cite_grid_clean.tex().strip_cites(), cite_grid_clean) << '\n';

  std::cout << "\n=== strip color ===\n";
  const auto color_source = pluss::make_note({
      R"(A \textcolor{red}{bold {nested}} B \colorbox{blue}{box} C)",
      R"(\fcolorbox[rgb]{0,0,0}{1,1,1}{framed} \color{green} tail)",
      R"(\\textcolor{red}{escaped} \textcolor{red}{unfinished)"});
  std::cout << color_source.tex().strip_color() << '\n';
  const auto color_grid = pluss::make_note(
      std::vector<std::string>{R"(\pagecolor{black} white)",
                               R"(\textcolor[rgb]{1,0,0}{red})", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto color_grid_clean = color_grid.tex().strip_color();
  std::cout << color_grid_clean << '\n';
  show_shape(color_grid_clean);
  std::cout << "receiver unchanged=" << (color_grid.at(0).find("pagecolor") != std::string_view::npos)
            << " idempotent=" << same(color_grid_clean.tex().strip_color(), color_grid_clean) << '\n';

  std::cout << "\n=== strip cmds ===\n";
  const auto cleanup_command_source = pluss::make_note({
      R"(A \frac{one {nested}}{two} B \alpha C)",
      R"(price \$5 and \\alpha literal)", R"(\broken{unfinished)"});
  std::cout << cleanup_command_source.tex().strip_cmds() << '\n';
  const auto command_grid = pluss::make_note(
      std::vector<std::string>{R"(A \section*[short]{Long} B)",
                               R"(x \unknown y)", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto command_grid_clean = command_grid.tex().strip_cmds();
  std::cout << command_grid_clean << '\n';
  show_shape(command_grid_clean);
  std::cout << "idempotent=" << same(command_grid_clean.tex().strip_cmds(), command_grid_clean) << '\n';

  std::cout << "\n=== strip env ===\n";
  const auto environment_source = pluss::make_note({
      R"(A \begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof} B)",
      R"(\\begin{proof}escaped\end{proof})",
      R"(\begin{proof}unfinished)"});
  std::cout << environment_source.tex().strip_env("proof") << '\n';
  const auto starred = pluss::make_note(
      {R"(A \begin{figure*}wide\end{figure*} B)",
       R"(C \begin{figure}plain\end{figure} D)"});
  std::cout << starred.tex().strip_env("figure*") << '\n';
  try {
    (void)environment_source.tex().strip_env("bad name");
  } catch (const pluss::Error&) {
    std::cout << "strip_env(bad name): rejected\n";
  }

  std::cout << "\n=== strip envs ===\n";
  const auto all_envs = pluss::make_note({
      R"(A \begin{quote}outer \begin{emph}inner\end{emph} tail\end{quote} B)",
      R"(C \begin{proof}one\end{proof} D \begin{lemma}two\end{lemma} E)",
      R"(\\begin{proof}escaped\end{proof} \begin{proof}unfinished)"});
  const auto no_envs = all_envs.tex().strip_envs();
  std::cout << no_envs << '\n';
  const auto env_grid = pluss::make_note(
      std::vector<std::string>{R"(x \begin{a}1\end{a} y)", "plain",
                               R"(\begin{b}2\end{b})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto env_grid_clean = env_grid.tex().strip_envs();
  std::cout << env_grid_clean << '\n';
  show_shape(env_grid_clean);
  std::cout << "idempotent=" << same(env_grid_clean.tex().strip_envs(), env_grid_clean) << '\n';

  std::cout << "\n=== strip figs ===\n";
  const auto figure_cleanup = pluss::make_note({
      R"(A \begin{figure}outer \begin{figure}inner\end{figure} tail\end{figure} B)",
      R"(C \begin{figure*}wide\end{figure*} D \begin{wrapfigure}wrap\end{wrapfigure} E)",
      R"(\includegraphics{kept.png} \begin{figure}unfinished)"});
  const auto no_figs = figure_cleanup.tex().strip_figs();
  std::cout << no_figs << '\n';
  std::cout << pluss::make_note(R"(A \begin{subfigure}part\end{subfigure} B)").tex().strip_figs() << '\n';
  std::cout << "idempotent=" << same(no_figs.tex().strip_figs(), no_figs) << '\n';

  std::cout << "\n=== strip fmt ===\n";
  const auto formatted = pluss::make_note({
      R"(A \textbf{bold \emph{and nested}} B)",
      R"(\underline{under} \mathrm{x} \texttt{code})",
      R"(\\textbf{escaped} \textbf{unfinished)"});
  const auto plain_fmt = formatted.tex().strip_fmt();
  std::cout << plain_fmt << '\n';
  const auto fmt_grid = pluss::make_note(
      std::vector<std::string>{R"(\mathbf{x})", "plain", R"(\mathsf{y})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto fmt_grid_clean = fmt_grid.tex().strip_fmt();
  std::cout << fmt_grid_clean << '\n';
  show_shape(fmt_grid_clean);
  std::cout << "idempotent=" << same(fmt_grid_clean.tex().strip_fmt(), fmt_grid_clean) << '\n';

  std::cout << "\n=== strip fonts ===\n";
  const auto fonts = pluss::make_note({
      R"(A {\bfseries bold} B {\itshape italic} C)",
      R"(\textbf{wrapped} \ttfamily mono)",
      R"(\\bfseries escaped \bfseriesExtra kept)"});
  const auto no_fonts = fonts.tex().strip_fonts();
  std::cout << no_fonts << '\n';
  std::cout << pluss::make_note(R"(\rmfamily roman \sffamily sans \scshape caps)").tex().strip_fonts() << '\n';
  std::cout << "idempotent=" << same(no_fonts.tex().strip_fonts(), no_fonts) << '\n';

  std::cout << "\n=== strip graphics ===\n";
  const auto graphics = pluss::make_note({
      R"(A \includegraphics[width={0.5\textwidth}]{img/{plot}.pdf} B)",
      R"(C \begin{figure}body\end{figure} D \begin{figure*}wide\end{figure*} E)",
      R"(\\includegraphics{escaped} \includegraphics{unfinished)"});
  const auto no_graphics = graphics.tex().strip_graphics();
  std::cout << no_graphics << '\n';
  const auto graphics_grid = pluss::make_note(
      std::vector<std::string>{R"(\includegraphics{a})", "plain",
                               R"(\begin{figure}b\end{figure})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto graphics_grid_clean = graphics_grid.tex().strip_graphics();
  std::cout << graphics_grid_clean << '\n';
  show_shape(graphics_grid_clean);
  std::cout << "idempotent=" << same(graphics_grid_clean.tex().strip_graphics(), graphics_grid_clean) << '\n';

  std::cout << "\n=== strip links ===\n";
  const auto links = pluss::make_note({
      R"(A \href{https://x.test/{a}}{visible {label}} B)",
      R"(C \hyperref[target]{jump \textbf{here}} D \url{https://y.test} E)",
      R"(\\href{escaped}{kept} \href{broken}{unfinished)"});
  const auto no_links = links.tex().strip_links();
  std::cout << no_links << '\n';
  const auto link_grid = pluss::make_note(
      std::vector<std::string>{R"(\href{u}{one})", R"(\url{u})", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto link_grid_clean = link_grid.tex().strip_links();
  std::cout << link_grid_clean << '\n';
  show_shape(link_grid_clean);
  std::cout << "receiver unchanged=" << (link_grid.at(0).find("href") != std::string_view::npos)
            << " idempotent=" << same(link_grid_clean.tex().strip_links(), link_grid_clean) << '\n';

  std::cout << "\n=== strip labels ===\n";
  const auto label_cleanup = pluss::make_note({
      R"(A \label{sec:{nested}} B \label*{starred} C)",
      R"(\\label{escaped} \labelled{long} \label{unfinished)"});
  const auto no_labels = label_cleanup.tex().strip_labels();
  std::cout << no_labels << '\n';
  const auto label_grid = pluss::make_note(
      std::vector<std::string>{R"(x \label{a} y)", R"(\label{b})", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto label_grid_clean = label_grid.tex().strip_labels();
  std::cout << label_grid_clean << '\n';
  show_shape(label_grid_clean);
  std::cout << "receiver unchanged=" << (label_grid.at(0).find("label") != std::string_view::npos)
            << " idempotent=" << same(label_grid_clean.tex().strip_labels(), label_grid_clean) << '\n';

  std::cout << "\n=== strip lr ===\n";
  const auto lr_source = pluss::make_note({
      R"(\left( x \right) + \left\{ y \right\})",
      R"(\left. f \right|_0^1 + \leftarrow + \rightarrow)",
      R"(\\left literal \leftover kept)"});
  const auto no_lr = lr_source.tex().strip_lr();
  std::cout << no_lr << '\n';
  const auto lr_grid = pluss::make_note(
      std::vector<std::string>{R"(\left[a\right])", "plain",
                               R"(\left\langle b\right\rangle)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto lr_grid_clean = lr_grid.tex().strip_lr();
  std::cout << lr_grid_clean << '\n';
  show_shape(lr_grid_clean);
  std::cout << "idempotent=" << same(lr_grid_clean.tex().strip_lr(), lr_grid_clean) << '\n';

  std::cout << "\n=== strip macro ===\n";
  const auto macro_cleanup = pluss::make_note({
      R"(A \newcommand{\RR}[1]{\mathbf{R}_{#1}} B \RR{x} C)",
      R"(\def\RR#1{{#1,#1}} \renewcommand{\ZZ}{Z} \RR{q})",
      R"(\\newcommand{\RR}{escaped} \newcommand{\RR}{unfinished)"});
  const auto no_rr = macro_cleanup.tex().strip_macro("RR");
  std::cout << no_rr << '\n';
  std::cout << macro_cleanup.tex().strip_macro(R"(\RR)") << '\n';
  std::cout << "idempotent=" << same(no_rr.tex().strip_macro("RR"), no_rr)
            << " other macro kept=" << (no_rr.at(1).find("renewcommand") != std::string_view::npos)
            << '\n';
  try {
    (void)macro_cleanup.tex().strip_macro("bad name");
  } catch (const pluss::Error&) {
    std::cout << "strip_macro(bad name): rejected\n";
  }

  std::cout << "\n=== strip delims ===\n";
  const auto delimiter_source = pluss::make_note({
      R"(A $x+1$ B $$y$$ C \(z\) D \[q\])",
      R"(price \$5; escaped \\(literal\\); unmatched $tail)",
      R"(plain text)"});
  const auto no_delims = delimiter_source.tex().strip_delims();
  std::cout << no_delims << '\n';
  const auto delim_grid = pluss::make_note(
      std::vector<std::string>{"$a$", R"(\(b\))", R"(\$3)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto delim_grid_clean = delim_grid.tex().strip_delims();
  std::cout << delim_grid_clean << '\n';
  show_shape(delim_grid_clean);
  std::cout << "idempotent=" << same(delim_grid_clean.tex().strip_delims(), delim_grid_clean) << '\n';

  std::cout << "\n=== strip pkgs ===\n";
  const auto package_cleanup = pluss::make_note({
      R"(A \documentclass[paper={a4}]{article} B)",
      R"(C \usepackage[opts={x,y}]{alpha,beta} D \RequirePackage{gamma} E)",
      R"(\\usepackage{escaped} \usepackages{long} \usepackage{unfinished)"});
  const auto no_packages = package_cleanup.tex().strip_pkgs();
  std::cout << no_packages << '\n';
  const auto package_grid = pluss::make_note(
      std::vector<std::string>{R"(\usepackage{a})", "plain",
                               R"(x \documentclass{book} y)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto package_grid_clean = package_grid.tex().strip_pkgs();
  std::cout << package_grid_clean << '\n';
  show_shape(package_grid_clean);
  std::cout << "idempotent=" << same(package_grid_clean.tex().strip_pkgs(), package_grid_clean) << '\n';

  std::cout << "\n=== strip refs ===\n";
  const auto refs_cleanup = pluss::make_note({
      R"(A \ref{eq:{one}} B \eqref*{eq:two} C \cref{sec:a} D)",
      R"(\Cref{x} \vref{y} \Vref{z} \autoref{a} \pageref{p} \nameref{n})",
      R"(\\ref{escaped} \reference{long} \ref{unfinished)"});
  const auto no_refs = refs_cleanup.tex().strip_refs();
  std::cout << no_refs << '\n';
  const auto refs_grid = pluss::make_note(
      std::vector<std::string>{R"(x \ref{a} y)", R"(\nameref{b})", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto refs_grid_clean = refs_grid.tex().strip_refs();
  std::cout << refs_grid_clean << '\n';
  show_shape(refs_grid_clean);
  std::cout << "idempotent=" << same(refs_grid_clean.tex().strip_refs(), refs_grid_clean) << '\n';

  std::cout << "\n=== cmd sub ===\n";
  const auto sub_source = pluss::make_note({
      R"(\alpha + \alpha* + \alphabet + \\alpha)",
      R"(\alpha* + \alpha)", "plain"});
  std::cout << sub_source.tex().cmd_sub("alpha", "[$&]") << '\n';
  std::cout << sub_source.tex().cmd_sub("alpha*", "STAR") << '\n';
  std::cout << "leading slash same="
            << same(sub_source.tex().cmd_sub("alpha", "A"),
                    sub_source.tex().cmd_sub(R"(\alpha)", "A")) << '\n';
  const auto sub_grid = pluss::make_note(
      std::vector<std::string>{R"(\alpha)", R"(\beta)", "",
                               R"(\alpha+\alpha)"},
      std::array<std::int64_t, 2>{2, 2});
  const auto mapped_grid = sub_grid.tex().cmd_sub("alpha", "A");
  std::cout << mapped_grid << '\n';
  show_shape(mapped_grid);
  try {
    (void)pluss::Note::undefined().tex().cmd_sub("bad name", "x");
  } catch (const pluss::Error&) {
    std::cout << "cmd_sub(bad name): rejected\n";
  }

  std::cout << "\n=== graphics tags ===\n";
  const auto graphics_source = pluss::make_note({
      R"(A \includegraphics[width={.5\textwidth}]{plots/{loss}.pdf} B)",
      R"(\\includegraphics{escaped} \includegraphicsx{long})",
      R"(\includegraphics[width=2cm]{unfinished)"});
  std::cout << graphics_source.tex().graphics_tags() << '\n';
  const auto tag_grid = pluss::make_note(
      std::vector<std::string>{R"(\includegraphics{a.png})", "plain",
                               R"(x \includegraphics{b.pdf} y)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto tagged_grid = tag_grid.tex().graphics_tags();
  std::cout << tagged_grid << '\n';
  show_shape(tagged_grid);

  std::cout << "\n=== render ===\n";
  const auto render_source = pluss::make_note({
      R"($\frac{1}{2}\alpha + x^{2} - y_1$)",
      R"(\textbf{Rate} \leq \infty; \unknown{kept})",
      R"(escaped \\alpha and \$5)"});
  std::cout << render_source.tex().render() << '\n';
  const auto render_grid = pluss::make_note(
      std::vector<std::string>{R"($x_1$)", R"(\alpha)", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto readable_grid = render_grid.tex().render();
  std::cout << readable_grid << '\n';
  show_shape(readable_grid);
  const auto recursive = pluss::make_note(
      R"(\newcommand{\A}{\B}\newcommand{\B}{done}\A)");
  std::cout << "one=" << recursive.tex().render(1).scalar()
            << " two=" << recursive.tex().render(2).scalar() << '\n';
  try {
    (void)render_source.tex().render(0);
  } catch (const pluss::Error&) {
    std::cout << "render(0): rejected\n";
  }

  std::cout << "\n=== sections ===\n";
  const auto section_source = pluss::make_note({
      R"(\section[Short]{A {nested} title} \section*{Second})",
      R"(\\section{escaped} \sectional{long} \section{unfinished)",
      "plain"});
  const auto section_batch = section_source.tex().sections();
  std::cout << section_batch.tokens() << '\n';
  show_offsets(section_batch);
  const auto section_grid = pluss::make_note(
      std::vector<std::string>{R"(\section{A})", "plain",
                               R"(\section{B}\section{C})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto grid_sections = section_grid.tex().sections();
  std::cout << grid_sections.tokens() << '\n';
  show_offsets(grid_sections);
  const auto empty_sections = pluss::make_note({"plain", ""}).tex().sections();
  std::cout << empty_sections.tokens() << '\n';
  show_offsets(empty_sections);

  std::cout << "\n=== strip space ===\n";
  const auto spacing_source = pluss::make_note({
      R"(a\,b\;c\:d\!e\ f\/g\quad h\qquad i)",
      R"(x\hspace*{1{e}m}y\vspace{2pt}z\vskip 3pt q)",
      R"(\\quad \quadratic \hspace{unfinished)"});
  const auto no_spacing = spacing_source.tex().strip_space();
  std::cout << no_spacing << '\n';
  const auto spacing_grid = pluss::make_note(
      std::vector<std::string>{R"(a\,b)", "plain", R"(c\quad d)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto compact_grid = spacing_grid.tex().strip_space();
  std::cout << compact_grid << '\n';
  show_shape(compact_grid);
  std::cout << "idempotent=" << same(no_spacing.tex().strip_space(), no_spacing) << '\n';

  std::cout << "\n=== strip tables ===\n";
  const auto table_source = pluss::make_note({
      R"(before \begin{table}outer \begin{tabular}x\end{tabular} tail\end{table} after)",
      R"(x \begin{array}a\end{array} y \begin{longtable}b\end{longtable} z)",
      R"(\\begin{table}escaped\\end{table} \begin{table}unfinished)"});
  const auto no_tables = table_source.tex().strip_tables();
  std::cout << no_tables << '\n';
  const auto table_grid = pluss::make_note(
      std::vector<std::string>{R"(\begin{tabular}a\end{tabular})", "plain",
                               R"(x \begin{table*}b\end{table*} y)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto clean_table_grid = table_grid.tex().strip_tables();
  std::cout << clean_table_grid << '\n';
  show_shape(clean_table_grid);
  std::cout << "idempotent=" << same(no_tables.tex().strip_tables(), no_tables) << '\n';

  std::cout << "\n=== set macro ===\n";
  const auto macro_update_source = pluss::make_note({
      R"(\renewcommand*{\unit}{old}
value=\unit)",
      "plain source",
      R"(\newcommand{\unit}{cm}\unit + \unitless)"});
  const auto macro_updated =
      macro_update_source.tex().set_macro("unit", R"(\mathrm{kg})");
  std::cout << macro_updated << '\n';
  std::cout << "leading slash same="
            << same(macro_update_source.tex().set_macro("unit", "m"),
                    macro_update_source.tex().set_macro(R"(\unit)", "m"))
            << '\n';
  const auto macro_grid = pluss::make_note(
      std::vector<std::string>{"a", "b", "", R"(\unit)"},
      std::array<std::int64_t, 2>{2, 2});
  const auto defined_grid = macro_grid.tex().set_macro("unit", "s");
  std::cout << defined_grid << '\n';
  show_shape(defined_grid);
  try {
    (void)pluss::Note::undefined().tex().set_macro("bad name", "x");
  } catch (const pluss::Error&) {
    std::cout << "set_macro(bad name): rejected\n";
  }
  try {
    (void)pluss::Note::undefined().tex().set_macro("unit", "{x");
  } catch (const pluss::Error&) {
    std::cout << "set_macro(unbalanced body): rejected\n";
  }

  std::cout << "\n=== simplify fractions ===\n";
  const auto fraction_source = pluss::make_note({
      R"(\dfrac { a + {b} } { \tfrac{ c }{ d } })",
      R"(\cfrac{x}{y} + \frac { 1 } { 2 })",
      R"(\operatorname{x} \fraction{a}{b} \\dfrac{a}{b} \frac{broken})"});
  const auto simple_fractions = fraction_source.tex().simplify_fractions();
  std::cout << simple_fractions << '\n';
  std::cout << "idempotent="
            << same(simple_fractions.tex().simplify_fractions(),
                    simple_fractions)
            << '\n';
  const auto fraction_norm_grid = pluss::make_note(
      std::vector<std::string>{R"(\tfrac{a}{b})", "plain",
                               R"(\dfrac {x}{y})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto simple_fraction_grid =
      fraction_norm_grid.tex().simplify_fractions();
  std::cout << simple_fraction_grid << '\n';
  show_shape(simple_fraction_grid);

  std::cout << "\n=== simplify roots ===\n";
  const auto root_source = pluss::make_note({
      R"(\sqrt [ 3 ] { a + \sqrt { {b} } })",
      R"(\sqrt { x } + \sqrt[ n + 1 ]{ y })",
      R"(\sqrted{x} \\sqrt{x} \sqrt[3]oops \sqrt{broken)"});
  const auto simple_roots = root_source.tex().simplify_roots();
  std::cout << simple_roots << '\n';
  std::cout << "idempotent="
            << same(simple_roots.tex().simplify_roots(), simple_roots) << '\n';
  const auto root_norm_grid = pluss::make_note(
      std::vector<std::string>{R"(\sqrt {a})", "plain",
                               R"(\sqrt [4] {b})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto simple_root_grid = root_norm_grid.tex().simplify_roots();
  std::cout << simple_root_grid << '\n';
  show_shape(simple_root_grid);

  std::cout << "\n=== strip comments ===\n";
  const auto comment_source = pluss::make_note({
      "keep \\% percent % LF\nnext",
      "alpha % CRLF\r\nbeta",
      "one % CR\rtwo",
      "100\\% visible"});
  const auto no_comments = comment_source.tex().strip_comments();
  std::cout << no_comments << '\n';
  std::cout << "idempotent="
            << same(no_comments.tex().strip_comments(), no_comments) << '\n';
  const auto comment_grid = pluss::make_note(
      std::vector<std::string>{"a % x\nb", "plain", "% all\nnext", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto clean_comment_grid = comment_grid.tex().strip_comments();
  std::cout << clean_comment_grid << '\n';
  show_shape(clean_comment_grid);

  std::cout << "\n=== strip metadata ===\n";
  const auto metadata_source = pluss::make_note({
      R"(A \title{Nested {title}} \author {Ada} \maketitle B)",
      R"(\date{2026} x \thanks{Supported by {Lab}} y)",
      R"(\\date{x} \titlecase{y} \thanks{broken})"});
  const auto no_metadata = metadata_source.tex().strip_metadata();
  std::cout << no_metadata << '\n';
  std::cout << "idempotent="
            << same(no_metadata.tex().strip_metadata(), no_metadata) << '\n';
  const auto metadata_grid = pluss::make_note(
      std::vector<std::string>{R"(\title{A})", "plain",
                               R"(x \author{B} y)", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto clean_metadata_grid = metadata_grid.tex().strip_metadata();
  std::cout << clean_metadata_grid << '\n';
  show_shape(clean_metadata_grid);

  std::cout << "\n=== strip preamble ===\n";
  const auto document_source = pluss::make_note({
      R"(\documentclass{article}
\begin { document }
Body \begin{figure}x\end{figure}
\end{document}
trailer)",
      R"(head \begin{document}body)",
      R"(\\begin{document}escaped\\end{document})",
      "plain body"});
  const auto document_bodies = document_source.tex().strip_preamble();
  std::cout << document_bodies << '\n';
  std::cout << "idempotent="
            << same(document_bodies.tex().strip_preamble(), document_bodies)
            << '\n';
  const auto document_grid = pluss::make_note(
      std::vector<std::string>{R"(h\begin{document}A\end{document}t)",
                               "plain", R"(\begin{document}\end{document})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto body_grid = document_grid.tex().strip_preamble();
  std::cout << body_grid << '\n';
  show_shape(body_grid);

  std::cout << "\n=== subsections ===\n";
  const auto subsection_source = pluss::make_note({
      R"(\subsection[Short]{Nested {title}} \subsection*{Second})",
      R"(\\subsection{escaped} \subsectional{long} \subsection{broken)",
      R"(\subsubsection{deeper} \subsection{Third})"});
  const auto subsection_batch = subsection_source.tex().subsections();
  std::cout << subsection_batch.tokens() << '\n';
  show_offsets(subsection_batch);
  const auto subsection_grid = pluss::make_note(
      std::vector<std::string>{R"(\subsection{A})", "plain",
                               R"(\subsection{B}\subsection{C})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto subsection_grid_batch = subsection_grid.tex().subsections();
  std::cout << subsection_grid_batch.tokens() << '\n';
  show_offsets(subsection_grid_batch);

  std::cout << "\n=== subsubsections ===\n";
  const auto deep_source = pluss::make_note({
      R"(\subsubsection[Short]{Deep {title}} \subsubsection*{Second})",
      R"(\subsection{shallower} \subsubsection{Third})", "plain"});
  const auto deep_batch = deep_source.tex().subsubsections();
  std::cout << deep_batch.tokens() << '\n';
  show_offsets(deep_batch);
  const auto empty_deep = pluss::make_note({"plain", ""}).tex().subsubsections();
  std::cout << empty_deep.tokens() << '\n';
  show_offsets(empty_deep);

  std::cout << "\n=== tables ===\n";
  const auto tables_source = pluss::make_note({
      R"(\begin{table*}star\end{table*} x \begin{table}plain\end{table})",
      R"(\begin{table}outer \begin{table}inner\end{table} tail\end{table})",
      R"(\\begin{table}escaped\\end{table} \begin{table}broken)"});
  const auto table_batch = tables_source.tex().tables();
  std::cout << table_batch.tokens() << '\n';
  show_offsets(table_batch);
  const auto tables_grid = pluss::make_note(
      std::vector<std::string>{R"(\begin{table}A\end{table})", "plain",
                               R"(\begin{table*}B\end{table*})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto table_grid_batch = tables_grid.tex().tables();
  std::cout << table_grid_batch.tokens() << '\n';
  show_offsets(table_grid_batch);

  std::cout << "\n=== tabular blocks ===\n";
  const auto tabular_source = pluss::make_note({
      R"(\begin{tabular*}wide\end{tabular*} x \begin{tabular}plain\end{tabular})",
      R"(\begin{table}\begin{tabular}inside\end{tabular}\end{table})",
      R"(\begin{array}not tabular\end{array} \begin{tabular}broken)"});
  const auto tabular_batch = tabular_source.tex().tabular_blocks();
  std::cout << tabular_batch.tokens() << '\n';
  show_offsets(tabular_batch);
  const auto no_tabular = pluss::make_note({"plain", ""}).tex().tabular_blocks();
  std::cout << no_tabular.tokens() << '\n';
  show_offsets(no_tabular);

  std::cout << "\n=== text only ===\n";
  const auto mixed_source = pluss::make_note(
      {R"(Before $x^2$ after)", R"(A \(y+1\) B \[z\] C)",
       R"(price \$5 and $unfinished)", ""});
  const auto prose_only = mixed_source.tex().text_only();
  std::cout << prose_only << '\n';
  std::cout << "idempotent=" << same(prose_only.tex().text_only(), prose_only)
            << '\n';
  const auto prose_grid = pluss::make_note(
      std::vector<std::string>{"left $x$", "$y$ right", "plain", "$$z$$"},
      std::array<std::int64_t, 2>{2, 2});
  const auto prose_grid_out = prose_grid.tex().text_only();
  std::cout << prose_grid_out << '\n';
  show_shape(prose_grid_out);

  std::cout << "\n=== to html ===\n";
  const auto html_source = pluss::make_note({
      R"(\section{Intro}\subsection{Detail}\subsubsection{Fine})",
      R"(\textbf{Bold} and \emph{emphasis}\\next \item first)",
      R"(See \href{https://example.test}{label} \label{x} \ref{x}.)",
      R"(\textcolor{red}{warn})"});
  const auto html = html_source.tex().to_html();
  std::cout << html << '\n';
  const auto html_grid = pluss::make_note(
      std::vector<std::string>{R"(\section{A})", R"(\textbf{B})",
                               "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto html_grid_out = html_grid.tex().to_html();
  std::cout << html_grid_out << '\n';
  show_shape(html_grid_out);

  std::cout << "\n=== to latex ===\n";
  const auto plain_source = pluss::make_note(
      {"x_1 & 50% complete", "alpha + beta <= gamma",
       "plain words", "sqrt(x) -> infty"});
  std::cout << plain_source.tex().to_latex(0) << '\n';
  std::cout << plain_source.tex().to_latex(1) << '\n';
  std::cout << plain_source.tex().to_latex(2) << '\n';
  const auto latex_grid = pluss::make_note(
      std::vector<std::string>{"a_b", "alpha", "x+y", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto latex_grid_out = latex_grid.tex().to_latex();
  std::cout << latex_grid_out << '\n';
  show_shape(latex_grid_out);
  for (const auto mode : {-1, 3}) {
    try {
      (void)plain_source.tex().to_latex(mode);
    } catch (const pluss::Error&) {
      std::cout << "to_latex(" << mode << "): rejected\n";
    }
  }

  std::cout << "\n=== tail boundaries ===\n";
  const auto undefined_tail = pluss::Note::undefined();
  std::cout << std::boolalpha << "undefined="
            << undefined_tail.tex().subsections().tokens().defined() << ','
            << undefined_tail.tex().subsubsections().tokens().defined() << ','
            << undefined_tail.tex().tables().tokens().defined() << ','
            << undefined_tail.tex().tabular_blocks().tokens().defined() << ','
            << undefined_tail.tex().text_only().defined() << ','
            << undefined_tail.tex().to_html().defined() << ','
            << undefined_tail.tex().to_latex().defined() << '\n';
  const auto tail_ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  for (const auto [label, operation] : std::array{
           std::pair{"subsections", 0}, std::pair{"subsubsections", 1},
           std::pair{"tables", 2}, std::pair{"tabular blocks", 3},
           std::pair{"text only", 4}, std::pair{"to html", 5},
           std::pair{"to latex", 6}}) {
    try {
      if (operation == 0) (void)tail_ints.tex().subsections();
      if (operation == 1) (void)tail_ints.tex().subsubsections();
      if (operation == 2) (void)tail_ints.tex().tables();
      if (operation == 3) (void)tail_ints.tex().tabular_blocks();
      if (operation == 4) (void)tail_ints.tex().text_only();
      if (operation == 5) (void)tail_ints.tex().to_html();
      if (operation == 6) (void)tail_ints.tex().to_latex();
    } catch (const pluss::Error&) {
      std::cout << label << " Int64: rejected\n";
    }
  }

  std::cout << "\n=== normalization boundaries ===\n";
  const auto undefined_norm = pluss::Note::undefined();
  std::cout << std::boolalpha << "undefined="
            << undefined_norm.tex().set_macro("unit", "m").defined() << ','
            << undefined_norm.tex().simplify_fractions().defined() << ','
            << undefined_norm.tex().simplify_roots().defined() << ','
            << undefined_norm.tex().strip_comments().defined() << ','
            << undefined_norm.tex().strip_metadata().defined() << ','
            << undefined_norm.tex().strip_preamble().defined() << '\n';
  const auto norm_ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  for (const auto [label, operation] : std::array{
           std::pair{"set macro", 0}, std::pair{"fractions", 1},
           std::pair{"roots", 2}, std::pair{"comments", 3},
           std::pair{"metadata", 4}, std::pair{"preamble", 5}}) {
    try {
      if (operation == 0) (void)norm_ints.tex().set_macro("unit", "m");
      if (operation == 1) (void)norm_ints.tex().simplify_fractions();
      if (operation == 2) (void)norm_ints.tex().simplify_roots();
      if (operation == 3) (void)norm_ints.tex().strip_comments();
      if (operation == 4) (void)norm_ints.tex().strip_metadata();
      if (operation == 5) (void)norm_ints.tex().strip_preamble();
    } catch (const pluss::Error&) {
      std::cout << label << " Int64: rejected\n";
    }
  }

  std::cout << "\n=== extractor boundaries ===\n";
  const auto undefined_extract = pluss::Note::undefined();
  std::cout << std::boolalpha << "undefined="
            << undefined_extract.tex().args("caption").tokens().defined() << ','
            << undefined_extract.tex().env("proof").tokens().defined() << ','
            << undefined_extract.tex().symbols().tokens().defined() << ','
            << undefined_extract.tex().figures().tokens().defined() << ','
            << undefined_extract.tex().fix_linebreaks().defined() << ','
            << undefined_extract.tex().footnotes().tokens().defined() << ','
            << undefined_extract.tex().headings(2).tokens().defined() << ','
            << undefined_extract.tex().includes().tokens().defined() << ','
            << undefined_extract.tex().inline_math().tokens().defined() << ','
            << undefined_extract.tex().labels().tokens().defined() << ','
            << undefined_extract.tex().macro_defs().tokens().defined() << ','
            << undefined_extract.tex().macro_use().tokens().defined() << ','
            << undefined_extract.tex().math_blocks(-1).tokens().defined() << ','
            << undefined_extract.tex().math_only().defined() << ','
            << undefined_extract.tex().line_norm().defined() << ','
            << undefined_extract.tex().packages().tokens().defined() << ','
            << undefined_extract.tex().refs().tokens().defined() << ','
            << undefined_extract.tex().deaccent().defined() << ','
            << undefined_extract.tex().strip_bib().defined() << ','
            << undefined_extract.tex().strip_size().defined() << ','
            << undefined_extract.tex().strip_cites().defined() << ','
            << undefined_extract.tex().strip_color().defined() << ','
            << undefined_extract.tex().strip_cmds().defined() << ','
            << undefined_extract.tex().strip_env("proof").defined() << ','
            << undefined_extract.tex().strip_envs().defined() << ','
            << undefined_extract.tex().strip_figs().defined() << ','
            << undefined_extract.tex().strip_fmt().defined() << ','
            << undefined_extract.tex().strip_fonts().defined() << ','
            << undefined_extract.tex().strip_graphics().defined() << ','
            << undefined_extract.tex().strip_links().defined() << ','
            << undefined_extract.tex().strip_labels().defined() << ','
            << undefined_extract.tex().strip_lr().defined() << ','
            << undefined_extract.tex().strip_macro("RR").defined() << ','
            << undefined_extract.tex().strip_delims().defined() << ','
            << undefined_extract.tex().strip_pkgs().defined() << ','
            << undefined_extract.tex().strip_refs().defined() << ','
            << undefined_extract.tex().cmd_sub("alpha", "a").defined() << ','
            << undefined_extract.tex().graphics_tags().defined() << ','
            << undefined_extract.tex().render().defined() << ','
            << undefined_extract.tex().sections().tokens().defined() << ','
            << undefined_extract.tex().strip_space().defined() << ','
            << undefined_extract.tex().strip_tables().defined() << '\n';
  try {
    (void)arg_source.tex().args("");
  } catch (const pluss::Error&) {
    std::cout << "args(empty): rejected\n";
  }
  try {
    (void)arg_source.tex().env("bad name");
  } catch (const pluss::Error&) {
    std::cout << "env(bad name): rejected\n";
  }

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
           std::pair{"macro substitutions", 18},
           std::pair{"command arguments", 19},
           std::pair{"named environment", 20}, std::pair{"math functions", 21},
           std::pair{"Greek commands", 22}, std::pair{"identifiers", 23},
           std::pair{"numbers", 24}, std::pair{"operators", 25},
           std::pair{"symbols", 26}, std::pair{"figures", 27},
           std::pair{"linebreaks", 28}, std::pair{"footnotes", 29},
           std::pair{"headings", 30}, std::pair{"includes", 31},
           std::pair{"inline math", 32}, std::pair{"labels", 33},
           std::pair{"macro defs", 34}, std::pair{"macro use", 35},
           std::pair{"math blocks", 36}, std::pair{"math only", 37},
           std::pair{"line norm", 38}, std::pair{"packages", 39},
           std::pair{"refs", 40}, std::pair{"deaccent", 41},
           std::pair{"strip bib", 42}, std::pair{"strip size", 43},
           std::pair{"strip cites", 44}, std::pair{"strip color", 45},
           std::pair{"strip cmds", 46}, std::pair{"strip env", 47},
           std::pair{"strip envs", 48}, std::pair{"strip figs", 49},
           std::pair{"strip fmt", 50}, std::pair{"strip fonts", 51},
           std::pair{"strip graphics", 52}, std::pair{"strip links", 53},
           std::pair{"strip labels", 54}, std::pair{"strip lr", 55},
           std::pair{"strip macro", 56}, std::pair{"strip delims", 57},
           std::pair{"strip pkgs", 58}, std::pair{"strip refs", 59},
           std::pair{"cmd sub", 60}, std::pair{"graphics tags", 61},
           std::pair{"render", 62}, std::pair{"sections", 63},
           std::pair{"strip space", 64}, std::pair{"strip tables", 65}}) {
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
      if (operation == 19) (void)ints.tex().args("caption");
      if (operation == 20) (void)ints.tex().env("proof");
      if (operation == 21) (void)ints.tex().math_funcs();
      if (operation == 22) (void)ints.tex().greek();
      if (operation == 23) (void)ints.tex().ids();
      if (operation == 24) (void)ints.tex().nums();
      if (operation == 25) (void)ints.tex().ops();
      if (operation == 26) (void)ints.tex().symbols();
      if (operation == 27) (void)ints.tex().figures();
      if (operation == 28) (void)ints.tex().fix_linebreaks();
      if (operation == 29) (void)ints.tex().footnotes();
      if (operation == 30) (void)ints.tex().headings(2);
      if (operation == 31) (void)ints.tex().includes();
      if (operation == 32) (void)ints.tex().inline_math();
      if (operation == 33) (void)ints.tex().labels();
      if (operation == 34) (void)ints.tex().macro_defs();
      if (operation == 35) (void)ints.tex().macro_use();
      if (operation == 36) (void)ints.tex().math_blocks(-1);
      if (operation == 37) (void)ints.tex().math_only();
      if (operation == 38) (void)ints.tex().line_norm();
      if (operation == 39) (void)ints.tex().packages();
      if (operation == 40) (void)ints.tex().refs();
      if (operation == 41) (void)ints.tex().deaccent();
      if (operation == 42) (void)ints.tex().strip_bib();
      if (operation == 43) (void)ints.tex().strip_size();
      if (operation == 44) (void)ints.tex().strip_cites();
      if (operation == 45) (void)ints.tex().strip_color();
      if (operation == 46) (void)ints.tex().strip_cmds();
      if (operation == 47) (void)ints.tex().strip_env("proof");
      if (operation == 48) (void)ints.tex().strip_envs();
      if (operation == 49) (void)ints.tex().strip_figs();
      if (operation == 50) (void)ints.tex().strip_fmt();
      if (operation == 51) (void)ints.tex().strip_fonts();
      if (operation == 52) (void)ints.tex().strip_graphics();
      if (operation == 53) (void)ints.tex().strip_links();
      if (operation == 54) (void)ints.tex().strip_labels();
      if (operation == 55) (void)ints.tex().strip_lr();
      if (operation == 56) (void)ints.tex().strip_macro("RR");
      if (operation == 57) (void)ints.tex().strip_delims();
      if (operation == 58) (void)ints.tex().strip_pkgs();
      if (operation == 59) (void)ints.tex().strip_refs();
      if (operation == 60) (void)ints.tex().cmd_sub("alpha", "a");
      if (operation == 61) (void)ints.tex().graphics_tags();
      if (operation == 62) (void)ints.tex().render();
      if (operation == 63) (void)ints.tex().sections();
      if (operation == 64) (void)ints.tex().strip_space();
      if (operation == 65) (void)ints.tex().strip_tables();
    } catch (const pluss::Error&) {
      std::cout << label << " Int64: rejected\n";
    }
  }

  std::cout << "\n=== to markdown ===\n";
  const auto markdown_source = pluss::make_note({
      R"(\section{Intro}\subsection{Detail}\subsubsection{Fine})",
      R"(\textbf{Bold} and \emph{emphasis} \item first)",
      R"(\textcolor{red}{warn} \href{https://example.test}{link} \label{x} \ref{x}.)"});
  const auto markdown = markdown_source.tex().to_markdown();
  std::cout << markdown << '\n';
  std::cout << markdown_source.tex().to_markdown(1) << '\n';
  require(!same(markdown_source.tex().to_markdown(0), markdown_source.tex().to_markdown(1)),
          "to_markdown mode >= 1 must case-fold, same contract as to_text's mode");
  require(same(markdown_source.tex().to_markdown(1), markdown_source.tex().to_markdown(5)),
          "to_markdown every mode >= 1 must fold identically");
  const auto markdown_grid = pluss::make_note(
      std::vector<std::string>{R"(\section{A})", R"(\textbf{B})", "plain", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto markdown_grid_out = markdown_grid.tex().to_markdown();
  std::cout << markdown_grid_out << '\n';
  show_shape(markdown_grid_out);

  std::cout << "\n=== todos ===\n";
  const auto todo_source = pluss::make_note({
      R"(\todo{fix this} text \todo{and this too})",
      "no todos here",
      R"(\todo{lone})"});
  const auto todo_batch = todo_source.tex().todos();
  std::cout << todo_batch.tokens() << '\n';
  show_offsets(todo_batch);
  const auto no_todos = pluss::make_note({"plain", ""}).tex().todos();
  std::cout << no_todos.tokens() << '\n';
  show_offsets(no_todos);

  std::cout << "\n=== warnings ===\n";
  const auto warning_source = pluss::make_note({
      R"(\PackageWarning{mypkg}{deprecated macro} body)",
      R"(Undefined control sequence at line 12)",
      R"(LaTeX Error: File not found)"
      "\ntrailing text on the next line is not part of the match",
      "clean, no diagnostics here"});
  const auto warning_batch = warning_source.tex().warnings();
  std::cout << warning_batch.tokens() << '\n';
  show_offsets(warning_batch);
  const auto error_source = pluss::make_note(
      {R"(\PackageError{mypkg}{bad option}{See the docs.})"});
  const auto error_batch = error_source.tex().warnings();
  std::cout << error_batch.tokens() << '\n';

  std::cout << "\n=== wrap inline math ===\n";
  const auto inline_targets = pluss::make_note({"x+y", "", "$already$", R"(\(also\))"});
  const auto inline_wrapped = inline_targets.tex().wrap_inline_math("");
  std::cout << inline_wrapped << '\n';
  require(same(inline_wrapped.tex().wrap_inline_math(""), inline_wrapped),
          "wrap_inline_math is not idempotent");
  const auto inline_custom = pluss::make_note({"a=b"}).tex().wrap_inline_math("~");
  std::cout << inline_custom << '\n';

  std::cout << "\n=== wrap display math ===\n";
  const auto display_targets = pluss::make_note({"x+y", "", "$already$"});
  const auto display_wrapped = display_targets.tex().wrap_display_math("");
  std::cout << display_wrapped << '\n';
  const auto display_dollars =
      pluss::make_note({"x+y"}).tex().wrap_display_math("$$");
  std::cout << display_dollars << '\n';
  const auto display_custom =
      pluss::make_note({"x+y"}).tex().wrap_display_math("~");
  std::cout << display_custom << '\n';

  std::cout << "\n=== unescape ===\n";
  const auto escaped_source = pluss::make_note(
      {R"(a\_b \$x\$ 50\% cost \#1 \& more \{y\} \~{} \^{} \textbackslash{})",
       "nothing special here"});
  const auto unescaped = escaped_source.tex().unescape();
  std::cout << unescaped << '\n';
  const auto round_trip = pluss::make_note({"a_b $x$ 50% cost #1 & more {y} ~ ^ \\"})
                               .tex().escape().tex().unescape();
  std::cout << round_trip << '\n';
  require(round_trip.at(0) == "a_b $x$ 50% cost #1 & more {y} ~ ^ \\",
          "escape/unescape round trip changed the text");

  std::cout << "\n=== final tranche boundaries ===\n";
  const auto undefined_final = pluss::Note::undefined();
  std::cout << std::boolalpha << "undefined="
            << undefined_final.tex().to_markdown().defined() << ','
            << undefined_final.tex().todos().tokens().defined() << ','
            << undefined_final.tex().warnings().tokens().defined() << ','
            << undefined_final.tex().wrap_inline_math("").defined() << ','
            << undefined_final.tex().wrap_display_math("").defined() << ','
            << undefined_final.tex().unescape().defined() << '\n';
  const auto final_ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  for (const auto [label, operation] : std::array{
           std::pair{"to markdown", 0}, std::pair{"todos", 1},
           std::pair{"warnings", 2}, std::pair{"wrap inline math", 3},
           std::pair{"wrap display math", 4}, std::pair{"unescape", 5}}) {
    try {
      if (operation == 0) (void)final_ints.tex().to_markdown();
      if (operation == 1) (void)final_ints.tex().todos();
      if (operation == 2) (void)final_ints.tex().warnings();
      if (operation == 3) (void)final_ints.tex().wrap_inline_math("");
      if (operation == 4) (void)final_ints.tex().wrap_display_math("");
      if (operation == 5) (void)final_ints.tex().unescape();
    } catch (const pluss::Error&) {
      std::cout << label << " Int64: rejected\n";
    }
  }
}
