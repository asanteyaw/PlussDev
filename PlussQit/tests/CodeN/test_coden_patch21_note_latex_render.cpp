#include <string>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN Note text/LaTeX conversion helpers are concrete", "[coden][patch21]") {
  SECTION("text_to_latex escapes plain text and can map math words") {
    auto escaped = pluss::make_note("x_1 & 50% complete").text_to_latex();
    REQUIRE(escaped.at(0) == "x\\_1 \\& 50\\% complete");

    auto mapped = pluss::make_note("alpha + beta <= gamma").text_to_latex(2);
    const std::string latex = mapped.at(0);
    REQUIRE(latex.find("\\alpha") != std::string::npos);
    REQUIRE(latex.find("\\beta") != std::string::npos);
    REQUIRE(latex.find("\\leq") != std::string::npos);
    REQUIRE(latex.find("\\gamma") != std::string::npos);
    REQUIRE(!latex.empty());
    REQUIRE(latex.front() == '$');
    REQUIRE(latex.back() == '$');
  }

  SECTION("latex_to_text strips TeX control flow into plain text") {
    auto latex = pluss::make_note(
        "\\section{Intro}\n"
        "Body: value is $x^2$.\n"
        "\\textbf{Bold words}.\n");
    auto plain = latex.latex_to_text();
    const std::string out = plain.at(0);
    REQUIRE(out.find('\\') == std::string::npos);
    REQUIRE(out.find('$') == std::string::npos);
    REQUIRE(out.find("Body") != std::string::npos);
  }

  SECTION("render_latex renders common math commands to readable output") {
    auto latex = pluss::make_note("$\\frac{1}{2}\\alpha + x^{2} - y_1$");
    auto rendered = latex.render_latex();
    const std::string out = rendered.at(0);
    REQUIRE(out.find("\\frac") == std::string::npos);
    REQUIRE(out.find("\\alpha") == std::string::npos);
    REQUIRE(out.find("(1)/(2)") != std::string::npos);
    REQUIRE(out.find("\xCE\xB1") != std::string::npos);
  }
}
