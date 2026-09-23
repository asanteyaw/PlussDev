#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN TeX delimiter/spacing cleanup family is concrete", "[coden][patch15]") {
  pluss::Note tex = pluss::make_note("\\left( x \\right) + \\Bigl[ y \\Bigr] + a\\,b\\quad c");
  auto out = tex.tex().strip_lr()
                 .tex().strip_size()
                 .tex().strip_space();
  const std::string s = std::string(out.at(0));
  REQUIRE(s.find("\\left") == std::string::npos);
  REQUIRE(s.find("\\right") == std::string::npos);
  REQUIRE(s.find("\\Bigl") == std::string::npos);
  REQUIRE(s.find("\\Bigr") == std::string::npos);
  REQUIRE(s.find("\\,") == std::string::npos);
  REQUIRE(s.find("\\quad") == std::string::npos);
  REQUIRE(s.find("a b") != std::string::npos);
}

TEST_CASE("CodeN TeX cmd_sub handles every Greek symbol name", "[coden][patch15]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{"\\alpha + \\beta + \\omega", "\\gamma + \\varphi"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  // The 15 per-letter tex_replace_<name>() convenience methods were removed
  // as pure aliases: each forwarded, with zero unique logic, to
  // cmd_sub(name, repl). cmd_sub is the one surface now.
  auto replaced = n.tex().cmd_sub("alpha", "A")
                   .tex().cmd_sub("beta", "B")
                   .tex().cmd_sub("omega", "W")
                   .tex().cmd_sub("varphi", "VP")
                   .tex().cmd_sub("gamma", "G");

  REQUIRE(replaced.at(0).find("\\alpha") == std::string::npos);
  REQUIRE(replaced.at(0).find("\\beta") == std::string::npos);
  REQUIRE(replaced.at(0).find("\\omega") == std::string::npos);
  REQUIRE(replaced.at(0).find("A + B + W") != std::string::npos);
  REQUIRE(replaced.at(1).find("\\gamma") == std::string::npos);
  REQUIRE(replaced.at(1).find("\\varphi") == std::string::npos);
  REQUIRE(replaced.at(1).find("G + VP") != std::string::npos);
}

TEST_CASE("CodeN TeX source normalization is balanced and lexical",
          "[coden][tex]") {
  const auto fractions = pluss::make_note({
      R"(\dfrac { a + {b} } { \tfrac{ c }{ d } })",
      R"(\operatorname{x} \fraction{a}{b} \\dfrac{a}{b} \frac{broken})"});
  const auto simple_fractions = fractions.tex().simplify_fractions();
  REQUIRE(simple_fractions.at(0) ==
          R"(\frac{a + {b}}{\frac{c}{d}})");
  REQUIRE(simple_fractions.at(1) == fractions.at(1));
  REQUIRE(simple_fractions.tex().simplify_fractions().at(0) ==
          simple_fractions.at(0));

  const auto roots = pluss::make_note({
      R"(\sqrt [ 3 ] { a + \sqrt { {b} } })",
      R"(\sqrted{x} \\sqrt{x} \sqrt[3]oops \sqrt{broken)"});
  const auto simple_roots = roots.tex().simplify_roots();
  REQUIRE(simple_roots.at(0) == R"(\sqrt[3]{a + \sqrt{{b}}})");
  REQUIRE(simple_roots.at(1) == roots.at(1));
  REQUIRE(simple_roots.tex().simplify_roots().at(0) == simple_roots.at(0));

  const auto comments = pluss::make_note(
      "keep \\% percent % LF\nnext % CRLF\r\nlast % CR\rend");
  REQUIRE(comments.tex().strip_comments().at(0) ==
          "keep \\% percent \nnext \r\nlast \rend");

  const auto metadata = pluss::make_note(
      R"(A \title{Nested {title}} \author {Ada} \maketitle B \\date{x} \titlecase{y} \thanks{broken)");
  REQUIRE(metadata.tex().strip_metadata().at(0) ==
          R"(A B \\date{x} \titlecase{y} \thanks{broken)");

  const auto document = pluss::make_note(
      R"(\documentclass{article}
\begin { document }
Body \begin{figure}x\end{figure}
\end{document}
trailer)");
  REQUIRE(document.tex().strip_preamble().at(0) ==
          "Body \\begin{figure}x\\end{figure}");
  const auto incomplete = pluss::make_note(R"(head \begin{document}body)");
  REQUIRE(incomplete.tex().strip_preamble().at(0) == incomplete.at(0));
}

TEST_CASE("CodeN TeX set_macro validates and replaces one declaration",
          "[coden][tex]") {
  const auto source = pluss::make_note(
      R"(\renewcommand*{\unit}{old}
value=\unit)");
  const auto updated = source.tex().set_macro(R"(\unit)", R"(\mathrm{kg})");
  REQUIRE(updated.at(0) ==
          "\\newcommand{\\unit}{\\mathrm{kg}}\nvalue=\\unit");

  REQUIRE_THROWS_AS(pluss::Note::undefined().tex().set_macro("bad name", "x"),
                    pluss::Error);
  REQUIRE_THROWS_AS(pluss::Note::undefined().tex().set_macro("ok", "{x"),
                    pluss::Error);
  REQUIRE_THROWS_AS(pluss::Note::undefined().tex().set_macro("ok", "#1"),
                    pluss::Error);
}
