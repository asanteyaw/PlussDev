#include <pluss/bi/bi.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace bi = pluss::bi;

  // Each constructor names a presentation. The view records which one it is.
  const bi::View kpi = bi::kpi("headline");
  const bi::View line = bi::line("trend");
  const bi::View bar = bi::bar("by_region");
  const bi::View table = bi::table("rows");

  std::cout << "kpi is a kpi: " << (kpi.bind().kind == bi::ViewKind::Kpi) << '\n';
  std::cout << "line is a line: " << (line.bind().kind == bi::ViewKind::Line) << '\n';
  std::cout << "bar is a bar: " << (bar.bind().kind == bi::ViewKind::Bar) << '\n';
  std::cout << "table is a detail table: " << (table.bind().kind == bi::ViewKind::DetailTable) << '\n';

  // The key identifies the view. It is what a board uses to refer to it.
  std::cout << "bar key: " << bar.key() << '\n';
}
