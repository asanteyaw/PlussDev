// Check a LaTeX manuscript before submission: list its sections, find citations without a bibliography entry
// and references to labels that do not exist.
#include <pluss/pluss.h>

#include <iostream>
#include <set>
#include <string>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto source = pluss::make_note(
      "\\section{Introduction}\\label{sec:intro}\n"
      "Prior work \\cite{knuth84} and \\cite{lamport94,missing99} motivates this paper.\n"
      "\\section{Method}\\label{sec:method}\n"
      "As argued in Section~\\ref{sec:intro} and Section~\\ref{sec:results}, we proceed.\n"
      "\\begin{equation}\\label{eq:main} E = mc^2 \\end{equation}\n"
      "See Equation~\\ref{eq:main}.\n"
      "\\section{Results}\n");
  auto bibliography_keys = pluss::make_note({"knuth84", "lamport94", "unused2000"});

  // 1. The outline of the paper.
  show("sections", source.tex().sections().tokens());

  // 2. Every label defined and every reference used.
  auto labels = source.tex().labels().tokens();
  auto refs = source.tex().refs().tokens();
  show("labels defined", labels);
  show("labels referenced", refs);

  // 3. References to labels that are never defined: set difference on the host.
  const auto defined = labels.host();
  const auto used = refs.host();
  std::set<std::string> defined_set(defined.vector().begin(), defined.vector().end());
  for (const auto &r : used.vector())
    if (!defined_set.count(r)) std::cout << "undefined reference: " << r << "\n";

  // 4. Citations against the bibliography: cited but missing, and listed but never cited.
  // citations() returns one token per \cite command ("lamport94,missing99" is one); bibliography_keys() splits a command into its keys.
  show("per command", source.tex().citations().tokens());
  auto cited = source.tex().bibliography_keys().tokens();
  show("citation keys", cited);
  const auto cites = cited.host();
  const auto bib = bibliography_keys.host();
  std::set<std::string> bib_set(bib.vector().begin(), bib.vector().end()), cite_set(cites.vector().begin(), cites.vector().end());
  for (const auto &k : cite_set)
    if (!bib_set.count(k)) std::cout << "cited but not in the bibliography: " << k << "\n";
  for (const auto &k : bib_set)
    if (!cite_set.count(k)) std::cout << "in the bibliography but never cited: " << k << "\n";
}
