#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {
bool has_token(const pluss::Note& toks, const std::string& needle) {
  for (std::int64_t i = 0; i < toks.numel(); ++i) {
    if (toks.at(i) == needle) return true;
  }
  return false;
}

std::vector<std::int64_t> offset_values(const pluss::TokenBatch& batch) {
  const auto lanes = batch.offsets().integers();
  return {lanes.begin(), lanes.end()};
}
} // namespace

TEST_CASE("CodeN TeX extract/token command family is concrete", "[coden][patch17]") {
  pluss::Note tex = pluss::make_note(
      "\\section{Intro}\n"
      "\\begin{equation} E = mc^2 \\end{equation}\n"
      "See \\ref{eq:1} and \\cite{einstein1905}.\n");

  auto cmds = tex.tex().commands();
  REQUIRE(has_token(cmds.tokens(), "\\section"));
  REQUIRE(has_token(cmds.tokens(), "\\begin"));
  REQUIRE(has_token(cmds.tokens(), "\\ref"));

  // env("equation") is the sole survivor of what used to be
  // two names (the removed tex_extract_env_equation() convenience method was
  // a pure alias): it now always returns the full closed TokenBatch, not a
  // bare flattened Note.
  auto env_eq = tex.tex().env("equation");
  REQUIRE(env_eq.tokens().numel() >= 1);
  REQUIRE(env_eq.tokens().at(0).find("\\begin{equation}") != std::string::npos);

  auto refs = tex.tex().refs();
  REQUIRE(has_token(refs.tokens(), "eq:1"));
}

TEST_CASE("CodeN TeX command and conversion parsing respects lexical boundaries",
          "[coden][tex]") {
  const auto source = pluss::make_note({
      R"(\section* [Short] {A {nested} title} \alpha)",
      R"(\\beta \$ \frac{one)"});
  const auto bare = source.tex().commands(false);
  const auto complete = source.tex().commands(true);

  REQUIRE(bare.tokens().numel() == 3);
  REQUIRE(bare.tokens().at(0) == R"(\section*)");
  REQUIRE(complete.tokens().at(0) ==
          R"(\section* [Short] {A {nested} title})");
  const auto offsets = complete.offsets().integers();
  REQUIRE(offsets.size() == 3);
  REQUIRE(offsets[0] == 0);
  REQUIRE(offsets[1] == 2);
  REQUIRE(offsets[2] == 3);

  const auto expressions = pluss::make_note({
      R"(\cfrac{\frac{x}{y}}{z})", R"(\sqrt[3]{a+\sqrt{b}})",
      R"(\pm x \mp y \pmatrix \\pm)",
      R"(\frac*{x}{y} \sqrt*{x} \pm*)"});
  REQUIRE(expressions.tex().frac_slash().at(0) ==
          "((x)/(y))/(z)");
  REQUIRE(expressions.tex().sqrt_pow().at(1) ==
          "(a+(b)^(1/2))^(1/3)");
  REQUIRE(expressions.tex().pm_signs().at(2) ==
          R"(+/- x -/+ y \pmatrix \\pm)");
  REQUIRE(expressions.tex().frac_slash().at(3) == expressions.at(3));
  REQUIRE(expressions.tex().sqrt_pow().at(3) == expressions.at(3));
  REQUIRE(expressions.tex().pm_signs().at(3) == expressions.at(3));

  const auto lines = pluss::make_note("a\r\n \r\n\t\r\nb");
  REQUIRE(lines.tex().dedup_blank_lines().scalar() == "a\r\n\r\nb");
  REQUIRE(pluss::make_note("a\n\n\n  b").tex().dedup_blank_lines().scalar() ==
          "a\n\n  b");
}

TEST_CASE("CodeN TeX semantic extractors preserve structure and lane ownership",
          "[coden][tex]") {
  const auto source = pluss::make_note({
      R"(\caption[short]{A {nested} title} \caption{Second} \captioned{no})",
      R"(\begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof})",
      R"(plain \$5; $x_1 + \sin(\alpha) = 2.5$ and \[y \leq 10\])"});

  const auto args = source.tex().args("caption");
  REQUIRE(args.tokens().numel() == 2);
  REQUIRE(args.tokens().at(0) == "A {nested} title");
  REQUIRE(args.tokens().at(1) == "Second");
  const auto arg_offsets = args.offsets().integers();
  REQUIRE(arg_offsets.size() == 4);
  REQUIRE(arg_offsets[0] == 0);
  REQUIRE(arg_offsets[1] == 2);
  REQUIRE(arg_offsets[2] == 2);
  REQUIRE(arg_offsets[3] == 2);

  const auto proof = source.tex().env("proof");
  REQUIRE(proof.tokens().numel() == 1);
  REQUIRE(proof.tokens().at(0) ==
          R"(\begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof})");

  REQUIRE(source.tex().math_funcs().tokens().at(0) == "sin");
  REQUIRE(source.tex().greek().tokens().at(0) == "alpha");
  REQUIRE(has_token(source.tex().ids().tokens(), "x_1"));
  REQUIRE(has_token(source.tex().nums().tokens(), "2.5"));
  REQUIRE(has_token(source.tex().ops().tokens(), R"(\leq)"));
  REQUIRE(has_token(source.tex().symbols().tokens(), R"(\alpha)"));

  REQUIRE_THROWS_AS(source.tex().args(""), pluss::Error);
  REQUIRE_THROWS_AS(source.tex().env("bad name"), pluss::Error);
}

TEST_CASE("CodeN TeX document extractors honor structure and source order",
          "[coden][tex]") {
  const auto source = pluss::make_note({
      R"(\begin{figure*}wide\end{figure*} then \begin{figure}plain \begin{figure}nested\end{figure}\end{figure})",
      R"(\footnote[7]{outer {nested} note} \footnoted{no} \\footnote{escaped})",
      R"(\section[Short]{Top {title}} \subsection*{Detail} \subsubsection{Deep} \paragraph{Leaf})",
      R"(\input{chapters/{intro}} \includegraphics[width={.5\textwidth}]{plot.pdf} \include{appendix} \\input{escaped})"});

  const auto figures = source.tex().figures();
  REQUIRE(figures.tokens().numel() == 2);
  REQUIRE(figures.tokens().at(0) == R"(\begin{figure*}wide\end{figure*})");
  REQUIRE(figures.tokens().at(1) ==
          R"(\begin{figure}plain \begin{figure}nested\end{figure}\end{figure})");
  REQUIRE(offset_values(figures) ==
          std::vector<std::int64_t>({0, 2, 2, 2, 2}));

  const auto footnotes = source.tex().footnotes();
  REQUIRE(footnotes.tokens().numel() == 1);
  REQUIRE(footnotes.tokens().at(0) == "outer {nested} note");
  REQUIRE(offset_values(footnotes) ==
          std::vector<std::int64_t>({0, 0, 1, 1, 1}));

  const auto shallow = source.tex().headings(2);
  REQUIRE(shallow.tokens().numel() == 2);
  REQUIRE(shallow.tokens().at(0) == "Top {title}");
  REQUIRE(shallow.tokens().at(1) == "Detail");
  REQUIRE(offset_values(shallow) ==
          std::vector<std::int64_t>({0, 0, 0, 2, 2}));
  REQUIRE(source.tex().headings(4).tokens().numel() == 4);
  REQUIRE_THROWS_AS(source.tex().headings(0), pluss::Error);
  REQUIRE_THROWS_AS(source.tex().headings(5), pluss::Error);

  const auto includes = source.tex().includes();
  REQUIRE(includes.tokens().numel() == 3);
  REQUIRE(includes.tokens().at(0) == "chapters/{intro}");
  REQUIRE(includes.tokens().at(1) == "plot.pdf");
  REQUIRE(includes.tokens().at(2) == "appendix");
  REQUIRE(offset_values(includes) ==
          std::vector<std::int64_t>({0, 0, 0, 0, 3}));
}

TEST_CASE("CodeN TeX substitution rendering and structural cleanup are exact",
          "[coden][tex]") {
  const auto commands = pluss::make_note({
      R"(\alpha + \alpha* + \alphabet + \\alpha)",
      R"(\alpha* + \alpha)",
      R"(plain)"});
  const auto plain = commands.tex().cmd_sub("alpha", "[$&]");
  REQUIRE(plain.at(0) == R"([$&] + \alpha* + \alphabet + \\alpha)");
  REQUIRE(plain.at(1) == R"(\alpha* + [$&])");
  REQUIRE(commands.tex().cmd_sub(R"(\alpha)", "A").at(0) ==
          R"(A + \alpha* + \alphabet + \\alpha)");
  REQUIRE(commands.tex().cmd_sub("alpha*", "STAR").at(1) ==
          R"(STAR + \alpha)");
  REQUIRE_THROWS_AS(commands.tex().cmd_sub("bad name", "x"), pluss::Error);
  REQUIRE_THROWS_AS(pluss::Note::undefined().tex().cmd_sub("", "x"),
                    pluss::Error);

  const auto graphics = pluss::make_note({
      R"(A \includegraphics[width={.5\textwidth}]{plots/{loss}.pdf} B)",
      R"(\\includegraphics{escaped} \includegraphicsx{long})",
      R"(\includegraphics[width=2cm]{unfinished)"});
  const auto tagged = graphics.tex().graphics_tags();
  REQUIRE(tagged.at(0) == "A [graphic:plots/{loss}.pdf] B");
  REQUIRE(tagged.at(1) == graphics.at(1));
  REQUIRE(tagged.at(2) == graphics.at(2));

  const auto spacing = pluss::make_note({
      R"(a\,b\;c\:d\!e\ f\/g\quad h\qquad i\hspace*{1{e}m}j\vspace{2pt}k\vskip 3pt l)",
      R"(\\quad \quadratic \hspace{unfinished)"});
  const auto compact = spacing.tex().strip_space();
  REQUIRE(compact.at(0) == "a b c d e f g h i j k l");
  REQUIRE(compact.at(1) == spacing.at(1));
  REQUIRE((compact == compact.tex().strip_space()).all().bool_item());

  const auto tables = pluss::make_note({
      R"(before \begin{table}outer \begin{tabular}x\end{tabular} tail\end{table} after)",
      R"(x \begin{array}a\end{array} y \begin{longtable}b\end{longtable} z)",
      R"(\\begin{table}escaped\\end{table} \begin{table}unfinished)"});
  const auto no_tables = tables.tex().strip_tables();
  REQUIRE(no_tables.at(0) == "before after");
  REQUIRE(no_tables.at(1) == "x y z");
  REQUIRE(no_tables.at(2) == tables.at(2));
  REQUIRE((no_tables == no_tables.tex().strip_tables()).all().bool_item());

  const auto section_source = pluss::make_note({
      R"(\section[Short]{A {nested} title} \section*{Second})",
      R"(\\section{escaped} \sectional{long} \section{unfinished)",
      "plain"});
  const auto sections = section_source.tex().sections();
  REQUIRE(sections.tokens().numel() == 2);
  REQUIRE(sections.tokens().at(0) == "A {nested} title");
  REQUIRE(sections.tokens().at(1) == "Second");
  REQUIRE(offset_values(sections) ==
          std::vector<std::int64_t>({0, 2, 2, 2}));

  const auto rendered = pluss::make_note(
      R"(\newcommand{\A}{\B}\newcommand{\B}{done}\A + \frac{1}{2} + x^{2})");
  REQUIRE(rendered.tex().render(1).scalar().find("done") == std::string::npos);
  REQUIRE(rendered.tex().render(2).scalar().find("done") != std::string::npos);
  REQUIRE_THROWS_AS(rendered.tex().render(0), pluss::Error);

  const auto ints = pluss::note::make(std::vector<std::int64_t>{1});
  REQUIRE_THROWS_AS(ints.tex().cmd_sub("alpha", "a"), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().graphics_tags(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().render(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().sections(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_space(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_tables(), pluss::Error);
}

TEST_CASE("CodeN TeX math labels and macro inventories are lexical and lane aware",
          "[coden][tex]") {
  const auto math = pluss::make_note(
      std::vector<std::string>{
          R"(price \$5; $x+\$y$ then $$display$$ and \(z\))",
          "\\[block\\] and $line\nbreak$", R"(\(unfinished)", "plain"},
      std::array<std::int64_t, 2>{2, 2});

  const auto inline_only = math.tex().inline_math();
  REQUIRE(inline_only.tokens().numel() == 2);
  REQUIRE(inline_only.tokens().at(0) == R"($x+\$y$)");
  REQUIRE(inline_only.tokens().at(1) == R"(\(z\))");
  REQUIRE(offset_values(inline_only) ==
          std::vector<std::int64_t>({0, 2, 2, 2, 2}));

  const auto all = math.tex().math_blocks(-1);
  REQUIRE(all.tokens().numel() == 4);
  REQUIRE(all.tokens().at(0) == R"($x+\$y$)");
  REQUIRE(all.tokens().at(1) == R"($$display$$)");
  REQUIRE(all.tokens().at(2) == R"(\(z\))");
  REQUIRE(all.tokens().at(3) == R"(\[block\])");
  REQUIRE(math.tex().math_blocks(0).tokens().numel() == 0);
  REQUIRE(math.tex().math_blocks(1).tokens().numel() == 2);
  REQUIRE_THROWS_AS(math.tex().math_blocks(-2), pluss::Error);

  const auto labels = pluss::make_note({
      R"(\label{eq:{energy}} \labeled{no})",
      R"(\\label{escaped} \label {sec:intro})", R"(\label{unfinished)"})
                          .tex().labels();
  REQUIRE(labels.tokens().numel() == 2);
  REQUIRE(labels.tokens().at(0) == "eq:{energy}");
  REQUIRE(labels.tokens().at(1) == "sec:intro");
  REQUIRE(offset_values(labels) ==
          std::vector<std::int64_t>({0, 1, 2, 2}));

  const auto declarations = pluss::make_note({
      R"(\newcommand{\RR}{\mathbb{R}} \renewcommand*\vecx[1]{\mathbf{#1}})",
      R"(\providecommand{\unit}[1][kg]{#1} \def\pair#1#2{#1,#2})",
      R"(\\newcommand{\escaped}{x} \newcommand{\broken})"});
  const auto defined = declarations.tex().macro_defs();
  REQUIRE(defined.tokens().numel() == 4);
  REQUIRE(defined.tokens().at(0) == R"(\RR)");
  REQUIRE(defined.tokens().at(1) == R"(\vecx)");
  REQUIRE(defined.tokens().at(2) == R"(\unit)");
  REQUIRE(defined.tokens().at(3) == R"(\pair)");
  REQUIRE(offset_values(defined) ==
          std::vector<std::int64_t>({0, 2, 4, 4}));

  const auto uses = pluss::make_note({
      R"(\alpha \beta \alpha \\gamma \section* \section)",
      R"(\beta \delta \beta)"})
                        .tex().macro_use();
  REQUIRE(uses.tokens().numel() == 6);
  REQUIRE(uses.tokens().at(0) == R"(\alpha)");
  REQUIRE(uses.tokens().at(1) == R"(\beta)");
  REQUIRE(uses.tokens().at(2) == R"(\section*)");
  REQUIRE(uses.tokens().at(3) == R"(\section)");
  REQUIRE(uses.tokens().at(4) == R"(\beta)");
  REQUIRE(uses.tokens().at(5) == R"(\delta)");
  REQUIRE(offset_values(uses) ==
          std::vector<std::int64_t>({0, 4, 6}));
}

TEST_CASE("CodeN TeX projection inventories and deaccenting honor lexical boundaries",
          "[coden][tex]") {
  const auto source = pluss::make_note(
      std::vector<std::string>{
          R"(price \$5, $x+\$y$, $$z$$, and \(q\))",
          "\\[a+b\\] then $line\nbreak$", R"(\(unfinished)", "plain"},
      std::array<std::int64_t, 2>{2, 2});
  const auto projected = source.tex().math_only();
  REQUIRE(projected.sizes() == source.sizes());
  REQUIRE(projected.at(0) == R"(x+\$y z q)");
  REQUIRE(projected.at(1) == "a+b");
  REQUIRE(projected.at(2).empty());
  REQUIRE(projected.at(3).empty());

  const auto refs = pluss::make_note({
      R"(\ref{eq:{energy}} \eqref* {eq:two} \cref{sec:a})",
      R"(\Cref{sec:b} \vref{fig:a} \Vref{fig:b} \nameref{name:a})",
      R"(\\ref{escaped} \reference{long} \ref{unfinished)"})
                        .tex().refs();
  REQUIRE(refs.tokens().numel() == 7);
  REQUIRE(refs.tokens().at(0) == "eq:{energy}");
  REQUIRE(refs.tokens().at(1) == "eq:two");
  REQUIRE(refs.tokens().at(2) == "sec:a");
  REQUIRE(refs.tokens().at(6) == "name:a");
  REQUIRE(offset_values(refs) ==
          std::vector<std::int64_t>({0, 3, 7, 7}));

  const auto packages = pluss::make_note({
      R"(\usepackage[margin={1in,2in}]{geometry, xcolor})",
      R"(\RequirePackage {amsmath} \RequirePackageWithOptions{hyperref})",
      R"(\\usepackage{escaped} \usepackages{long} \usepackage{, })"})
                            .tex().packages();
  REQUIRE(packages.tokens().numel() == 4);
  REQUIRE(packages.tokens().at(0) == "geometry");
  REQUIRE(packages.tokens().at(1) == "xcolor");
  REQUIRE(packages.tokens().at(2) == "amsmath");
  REQUIRE(packages.tokens().at(3) == "hyperref");
  REQUIRE(offset_values(packages) ==
          std::vector<std::int64_t>({0, 2, 4, 4}));

  const auto accents = pluss::make_note(
      std::vector<std::string>{R"(\'{e} \v S \c{c} \t{oo} \text{keep})",
                               "caf\xC3\xA9 d\xC3\xA9j\xC3\xA0",
                               R"(\\'{e} \unknown{x})", ""},
      std::array<std::int64_t, 2>{2, 2});
  const auto plain = accents.tex().deaccent();
  REQUIRE(plain.sizes() == accents.sizes());
  REQUIRE(plain.at(0) == R"(e S c oo \text{keep})");
  REQUIRE(plain.at(1) == "cafe deja");
  REQUIRE(plain.at(2) == R"(\\'{e} \unknown{x})");
  REQUIRE(plain.at(3).empty());

  const auto lines = pluss::make_note(
      std::vector<std::string>{"a\r\nb\rc\n", "stable", "", "x\r"},
      std::array<std::int64_t, 2>{2, 2});
  const auto normalized = lines.tex().line_norm();
  REQUIRE(normalized.sizes() == lines.sizes());
  REQUIRE(normalized.at(0) == "a\nb\nc\n");
  REQUIRE(normalized.at(1) == "stable");
  REQUIRE(normalized.at(2).empty());
  REQUIRE(normalized.at(3) == "x\n");

  REQUIRE_FALSE(pluss::Note::undefined().tex().math_only().defined());
  REQUIRE_FALSE(pluss::Note::undefined().tex().line_norm().defined());
  REQUIRE_FALSE(pluss::Note::undefined().tex().deaccent().defined());
  const auto ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  REQUIRE_THROWS_AS(ints.tex().math_only(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().line_norm(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().packages(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().refs(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().deaccent(), pluss::Error);
}

TEST_CASE("CodeN TeX cleanup preserves authored content across balanced boundaries",
          "[coden][tex]") {
  const auto bib = pluss::make_note({
      R"(Before \bibliography{refs/{main}} middle \printbibliography[heading={none}] after)",
      R"(A \begin{thebibliography}{9} one \begin{thebibliography}{1} nested \end{thebibliography} two \end{thebibliography} Z)",
      R"(\\bibliography{escaped} \bibliography{unfinished)"});
  const auto no_bib = bib.tex().strip_bib();
  REQUIRE(no_bib.at(0) == "Before middle after");
  REQUIRE(no_bib.at(1) == "A Z");
  REQUIRE(no_bib.at(2) == bib.at(2));

  const auto sized = pluss::make_note({
      R"(\Bigl( x \Bigr) + \bigg[y\bigg])",
      R"(\\Bigl literal \BiglExtra keep)", "plain"});
  const auto plain_size = sized.tex().strip_size();
  REQUIRE(plain_size.at(0) == "( x ) + [y]");
  REQUIRE(plain_size.at(1) == sized.at(1));

  const auto cites = pluss::make_note({
      R"(A \citep[see][p.~2]{alpha,{beta}} B \cite*{gamma} C)",
      R"(\\cite{escaped} \cite{unfinished)"});
  const auto no_cites = cites.tex().strip_cites();
  REQUIRE(no_cites.at(0) == "A B C");
  REQUIRE(no_cites.at(1) == cites.at(1));

  const auto colors = pluss::make_note({
      R"(A \textcolor{red}{bold {nested}} B \colorbox{blue}{box} C)",
      R"(\fcolorbox[rgb]{0,0,0}{1,1,1}{framed} \color{green} tail)",
      R"(\\textcolor{red}{escaped} \textcolor{red}{unfinished)"});
  const auto plain_color = colors.tex().strip_color();
  REQUIRE(plain_color.at(0) == "A bold {nested} B box C");
  REQUIRE(plain_color.at(1) == "framed tail");
  REQUIRE(plain_color.at(2) == colors.at(2));

  const auto commands = pluss::make_note({
      R"(A \frac{one {nested}}{two} B \alpha C)",
      R"(price \$5 and \\alpha literal)", R"(\broken{unfinished)"});
  const auto no_commands = commands.tex().strip_cmds();
  REQUIRE(no_commands.at(0) == "A B C");
  REQUIRE(no_commands.at(1) == commands.at(1));
  REQUIRE(no_commands.at(2) == "{unfinished");

  const auto environments = pluss::make_note({
      R"(A \begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof} B)",
      R"(\\begin{proof}escaped\end{proof})",
      R"(\begin{proof}unfinished)"});
  const auto no_proof = environments.tex().strip_env("proof");
  REQUIRE(no_proof.at(0) == "A B");
  REQUIRE(no_proof.at(1) == environments.at(1));
  REQUIRE(no_proof.at(2) == environments.at(2));
  REQUIRE_THROWS_AS(environments.tex().strip_env(""), pluss::Error);
  REQUIRE_THROWS_AS(environments.tex().strip_env("bad name"), pluss::Error);

  REQUIRE_FALSE(pluss::Note::undefined().tex().strip_bib().defined());
  REQUIRE_FALSE(pluss::Note::undefined().tex().strip_env("proof").defined());
  const auto ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  REQUIRE_THROWS_AS(ints.tex().strip_bib(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_size(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_cites(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_color(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_cmds(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_env("proof"), pluss::Error);
}

TEST_CASE("CodeN TeX structural cleanup is balanced and shape preserving",
          "[coden][tex]") {
  const auto source = pluss::make_note(
      std::vector<std::string>{
          R"(A \begin{quote}outer \begin{emph}inner\end{emph}\end{quote} B)",
          R"(\begin{figure}one \begin{figure}two\end{figure}\end{figure})",
          R"(\textbf{bold \emph{nested}} \bfseries tail)",
          R"(\href{https://x/{y}}{visible {label}} \url{gone})"},
      std::array<std::int64_t, 2>{2, 2});

  const auto no_envs = source.tex().strip_envs();
  REQUIRE(no_envs.sizes() == source.sizes());
  REQUIRE(no_envs.at(0) == "A B");
  REQUIRE(no_envs.at(1).empty());
  REQUIRE((no_envs == no_envs.tex().strip_envs()).all().bool_item());

  const auto no_figs = source.tex().strip_figs();
  REQUIRE(no_figs.at(1).empty());
  REQUIRE(no_figs.at(0) == source.at(0));

  const auto no_fmt = source.tex().strip_fmt();
  REQUIRE(no_fmt.at(2) == R"(bold nested \bfseries tail)");
  const auto no_fonts = source.tex().strip_fonts();
  REQUIRE(no_fonts.at(2) == "bold nested tail");

  const auto graphics = pluss::make_note({
      R"(A \includegraphics[width={2cm}]{img/{plot}.pdf} B)",
      R"(C \begin{figure*}wide\end{figure*} D)",
      R"(\\includegraphics{escaped} \includegraphics{unfinished)"});
  const auto no_graphics = graphics.tex().strip_graphics();
  REQUIRE(no_graphics.at(0) == "A B");
  REQUIRE(no_graphics.at(1) == "C D");
  REQUIRE(no_graphics.at(2) == graphics.at(2));

  const auto links = source.tex().strip_links();
  REQUIRE(links.at(3) == "visible {label}");
  REQUIRE((links == links.tex().strip_links()).all().bool_item());

  const auto malformed = pluss::make_note({
      R"(\begin{proof}unfinished)", R"(\textbf{unfinished)",
      R"(\href{target}{unfinished)", R"(\\href{x}{escaped})"});
  REQUIRE(malformed.tex().strip_envs().at(0) == malformed.at(0));
  REQUIRE(malformed.tex().strip_fmt().at(1) == malformed.at(1));
  REQUIRE(malformed.tex().strip_links().at(2) == malformed.at(2));
  REQUIRE(malformed.tex().strip_links().at(3) == malformed.at(3));

  REQUIRE_FALSE(pluss::Note::undefined().tex().strip_links().defined());
  const auto ints = pluss::note::make(std::vector<std::int64_t>{1});
  REQUIRE_THROWS_AS(ints.tex().strip_envs(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_figs(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_fmt(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_fonts(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_graphics(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_links(), pluss::Error);
}

TEST_CASE("CodeN TeX declaration and marker cleanup preserves boundaries",
          "[coden][tex]") {
  const auto source = pluss::make_note(
      std::vector<std::string>{
          R"(A \label{sec:{nested}} B \ref*{eq:{one}} C)",
          R"(\left( x \right) + \leftarrow + \rightarrow)",
          R"(\newcommand{\RR}[1]{\mathbf{R}_{#1}} \RR{x})",
          R"(\documentclass[paper={a4}]{article} \usepackage{o})"},
      std::array<std::int64_t, 2>{2, 2});

  const auto labels = source.tex().strip_labels();
  REQUIRE(labels.sizes() == source.sizes());
  REQUIRE(labels.at(0) == R"(A B \ref*{eq:{one}} C)");
  REQUIRE((labels == labels.tex().strip_labels()).all().bool_item());

  const auto refs = source.tex().strip_refs();
  REQUIRE(refs.at(0) == R"(A \label{sec:{nested}} B C)");
  REQUIRE((refs == refs.tex().strip_refs()).all().bool_item());

  const auto lr = source.tex().strip_lr();
  REQUIRE(lr.at(1) == R"(( x ) + \leftarrow + \rightarrow)");

  const auto macro = source.tex().strip_macro("RR");
  REQUIRE(macro.at(2) == R"(\RR{x})");
  REQUIRE(source.tex().strip_macro(R"(\RR)").at(2) == macro.at(2));
  REQUIRE_THROWS_AS(source.tex().strip_macro(""), pluss::Error);
  REQUIRE_THROWS_AS(source.tex().strip_macro("bad name"), pluss::Error);

  const auto pkgs = source.tex().strip_pkgs();
  REQUIRE(pkgs.at(3).empty());

  const auto delims = pluss::make_note(
      {R"($x$ $$y$$ \(z\) \[q\])", R"(price \$5; \\(literal\\); $tail)"})
      .tex().strip_delims();
  REQUIRE(delims.at(0) == "x y z q");
  REQUIRE(delims.at(1) == R"(price \$5; \\(literal\\); tail)");

  const auto malformed = pluss::make_note({
      R"(\label{unfinished)", R"(\ref{unfinished)",
      R"(\usepackage{unfinished)", R"(\\label{escaped})"});
  REQUIRE(malformed.tex().strip_labels().at(0) == malformed.at(0));
  REQUIRE(malformed.tex().strip_refs().at(1) == malformed.at(1));
  REQUIRE(malformed.tex().strip_pkgs().at(2) == malformed.at(2));
  REQUIRE(malformed.tex().strip_labels().at(3) == malformed.at(3));

  REQUIRE_FALSE(pluss::Note::undefined().tex().strip_refs().defined());
  const auto ints = pluss::note::make(std::vector<std::int64_t>{1});
  REQUIRE_THROWS_AS(ints.tex().strip_labels(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_lr(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_macro("RR"), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_delims(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_pkgs(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().strip_refs(), pluss::Error);
}

TEST_CASE("CodeN NLP/tokenization extract family is concrete", "[coden][patch17]") {
  pluss::Note n = pluss::make_note(
      "OpenAI shipped GPT in 2025. Visit https://openai.com and email team@openai.com.\n"
      "NASA builds rockets.");

  auto urls = n.nlp().urls();
  REQUIRE(has_token(urls.tokens(), "https://openai.com"));

  auto acr = n.nlp().acronyms();
  REQUIRE(has_token(acr.tokens(), "GPT"));
  REQUIRE(has_token(acr.tokens(), "NASA"));

  auto grams = n.nlp().word_ngrams(2);
  REQUIRE(grams.tokens().numel() > 0);

  auto sents = n.nlp().sentences();
  REQUIRE(sents.tokens().numel() >= 2);

  auto found = n.regex().findall("[A-Z]{2,}");
  REQUIRE(found.tokens().numel() >= 2);

  auto joined = pluss::make_note({"OpenAI", "shipped", "GPT"}).join(" ");
  REQUIRE(joined.defined());
  REQUIRE(joined.is_scalar());
  REQUIRE(joined.scalar() == "OpenAI shipped GPT");
}
