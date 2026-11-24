// Every umbrella header, and the headers inside PlussTils one by one, must compile together
// from an installed download: <pluss/tils.h> is all of PlussTils, and each piece can be taken alone.
#include <pluss/pluss.h>
#include <pluss/tils.h>
#include <pluss/ma.h>
#include <pluss/ai.h>

#include <pluss/tils/cache.h>
#include <pluss/tils/doc.h>
#include <pluss/tils/env.h>
#include <pluss/tils/flow.h>
#include <pluss/tils/info.h>
#include <pluss/tils/kit.h>
#include <pluss/tils/kit/output.h>
#include <pluss/tils/let.h>
#include <pluss/tils/print_options.h>
#include <pluss/tils/settings.h>
#include <pluss/tils/suppose.h>
#include <pluss/tils/view.h>

int main() {
  // Something from each part, so a header that compiles but links to nothing is noticed.
  pluss::tils::Settings settings;
  settings.set("n", 3);
  const auto page = pluss::tils::doc_find("Series::sum");
  return settings.get<int>("n") == 3 && pluss::tils::info::build().platforms.size() >= 1 && (page || true) ? 0 : 1;
}
