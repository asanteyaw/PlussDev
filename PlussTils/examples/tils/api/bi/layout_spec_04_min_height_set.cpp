#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace bi = pluss::bi;

int main() {
  // A view or a control can set a minimum height in pixels. The tile records it in its layout.
  const bi::View tall = bi::table("rows").query(bi::from("sales")).place(0, 0).min_height(320.0);
  const bi::Control picker = bi::category("pick", "region", "Region").place(0, 1).min_height(180.0);
  std::cout << "view minimum height: " << *tall.layout().min_height << '\n';
  std::cout << "control minimum height: " << *picker.layout().min_height << '\n';
  return 0;
}
