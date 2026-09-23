#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {

std::vector<std::string> strings(const pluss::TokenBatch& batch) {
  std::vector<std::string> out;
  for (const auto value : batch.tokens().vector()) out.emplace_back(value);
  return out;
}

bool offsets_equal(const pluss::TokenBatch& batch,
                   std::initializer_list<std::int64_t> expected) {
  const auto actual = batch.offsets().integers();
  return actual.size() == expected.size() &&
         std::equal(actual.begin(), actual.end(), expected.begin());
}

} // namespace

TEST_CASE("CodeN TeX structural tail extractors preserve authored structure",
          "[coden][tex]") {
  const auto headings = pluss::make_note({
      R"(\subsection[Short]{Nested {title}} \subsubsection*{Deep {title}})",
      R"(\\subsection{escaped} \subsectional{long} \subsection{broken)",
      R"(\subsubsection{D1}\subsection{S1}\subsubsection{D2})"});

  const auto subsections = headings.tex().subsections();
  REQUIRE(strings(subsections) ==
          std::vector<std::string>{"Nested {title}", "S1"});
  REQUIRE(offsets_equal(subsections, {0, 1, 1, 2}));

  const auto deep = headings.tex().subsubsections();
  REQUIRE(strings(deep) ==
          std::vector<std::string>{"Deep {title}", "D1", "D2"});
  REQUIRE(offsets_equal(deep, {0, 1, 1, 3}));

  const auto table_source = pluss::make_note(
      R"(\begin{table*}star\end{table*} x \begin{table}plain\end{table})");
  const auto tables = strings(table_source.tex().tables());
  REQUIRE(tables.size() == 2);
  REQUIRE(tables[0].find("table*") != std::string::npos);
  REQUIRE(tables[1].find("table}") != std::string::npos);

  const auto tabular_source = pluss::make_note(
      R"(\begin{tabular*}wide\end{tabular*} x \begin{tabular}plain\end{tabular})");
  const auto tabular = strings(tabular_source.tex().tabular_blocks());
  REQUIRE(tabular.size() == 2);
  REQUIRE(tabular[0].find("tabular*") != std::string::npos);
  REQUIRE(tabular[1].find("tabular}") != std::string::npos);
}

TEST_CASE("CodeN TeX tail projections preserve shape and validate modes",
          "[coden][tex]") {
  const auto source = pluss::make_note(
      std::vector<std::string>{R"(Before $x^2$ after)",
                               R"(\section{Intro}\textbf{Bold})",
                               "x_1 & 50%", "alpha + beta <= gamma"},
      std::array<std::int64_t, 2>{2, 2});

  const auto prose = source.tex().text_only();
  REQUIRE(prose.sizes() == source.sizes());
  REQUIRE(prose.at(0) == "Before after");

  const auto html = source.tex().to_html();
  REQUIRE(html.sizes() == source.sizes());
  REQUIRE(html.at(1) == "<h1>Intro</h1><strong>Bold</strong>");

  REQUIRE(source.tex().to_latex(0).at(2) == "x\\_1 \\& 50\\%");
  REQUIRE(source.tex().to_latex(1).at(3).find(R"(\alpha)") !=
          std::string_view::npos);
  const auto latex_math = source.tex().to_latex(2);
  const auto math = latex_math.at(3);
  REQUIRE(math.front() == '$');
  REQUIRE(math.back() == '$');

  for (const auto mode : {-1, 3}) {
    try {
      (void)(mode < 0 ? source : pluss::Note::undefined())
          .tex().to_latex(mode);
      FAIL("invalid LaTeX mode was accepted");
    } catch (const pluss::Error& error) {
      REQUIRE(std::string(error.what()) ==
              "Note::tex().to_latex: mode must be 0, 1, or 2");
    }
  }
}

TEST_CASE("CodeN TeX tail methods propagate state and reject non-text",
          "[coden][tex]") {
  const auto absent = pluss::Note::undefined();
  REQUIRE_FALSE(absent.tex().subsections().tokens().defined());
  REQUIRE_FALSE(absent.tex().subsubsections().tokens().defined());
  REQUIRE_FALSE(absent.tex().tables().tokens().defined());
  REQUIRE_FALSE(absent.tex().tabular_blocks().tokens().defined());
  REQUIRE_FALSE(absent.tex().text_only().defined());
  REQUIRE_FALSE(absent.tex().to_html().defined());
  REQUIRE_FALSE(absent.tex().to_latex().defined());

  const auto ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  REQUIRE_THROWS_AS(ints.tex().subsections(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().subsubsections(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().tables(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().tabular_blocks(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().text_only(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().to_html(), pluss::Error);
  REQUIRE_THROWS_AS(ints.tex().to_latex(), pluss::Error);
}
