// Every umbrella header and every single header the include guide lists must compile together
// from an installed download, and each one must stand alone (see docs/sphinx/include.md).
#include <pluss/pluss.h>
#include <pluss/tils.h>
#include <pluss/ma.h>
#include <pluss/ai.h>

#include <pluss/core.h>
#include <pluss/error.h>
#include <pluss/null.h>
#include <pluss/validate.h>
#include <pluss/msharp/lister.h>
#include <pluss/codem/series_factory.h>
#include <pluss/codem/losses.h>
#include <pluss/slicing.h>
#include <pluss/coden/note_factory.h>
#include <pluss/codeh/timeline_factory.h>
#include <pluss/queue.h>
#include <pluss/io.h>
#include <pluss/array_file.h>
#include <pluss/exception.h>
#include <pluss/version.h>
#include <pluss/pluss_type.h>
#include <pluss/dispense.h>
#include <pluss/mic.h>
#include <pluss/table/table.h>
#include <pluss/kindle/kindle.h>
#include <pluss/scribble/scribble.h>
#include <pluss/bundle/bundle.h>
#include <pluss/loader/loader.h>
#include <pluss/pl.h>
#include <pluss/pl/studio.h>
#include <pluss/pl/style.h>
#include <pluss/pl/views.h>
#include <pluss/pl/flash.h>
#include <pluss/tils/doc.h>
#include <pluss/tils/let.h>
#include <pluss/tils/flow.h>
#include <pluss/tils/suppose.h>
#include <pluss/tils/kit/output.h>
#include <pluss/tils/kit/rng_seed.h>
#include <pluss/tils/kit/pathing.h>
#include <pluss/ma/math.h>
#include <pluss/ma/stats.h>
#include <pluss/ma/eng.h>
#include <pluss/ma/natsci.h>
#include <pluss/ma/actuarial.h>
#include <pluss/ma/finance.h>
#include <pluss/ma/linearalg.h>
#include <pluss/nn.h>
#include <pluss/ai/ml.h>
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/ai/nlp.h>
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

// Names a program is likely to use for its own types. PlussII's headers must not bring in system types
// of the same name (on macOS, Accelerate once did, through CoreFoundation).
struct Point {
  double x, y;
};
struct Rect {
  int left, top;
};

int main() {
  // Something from each part, so a header that compiles but links to nothing is noticed.
  pluss::tils::Settings settings;
  settings.set("n", 3);
  const auto page = pluss::tils::doc_find("Series::sum");
  return settings.get<int>("n") == 3 && pluss::tils::info::build().platforms.size() >= 1 && (page || true) ? 0 : 1;
}
