// The palette generators: each block between the "### name n" markers is one documented example.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::pl;

static void describe(const PaletteRamp &ramp) {
  const auto c = [](const Rgba &x) {
    return "(" + std::to_string(static_cast<int>(x.red * 255 + 0.5)) + ", " + std::to_string(static_cast<int>(x.green * 255 + 0.5)) + ", " +
           std::to_string(static_cast<int>(x.blue * 255 + 0.5)) + ")";
  };
  const auto &s = ramp.stops();
  std::cout << s.size() << " stops; first " << c(s.front()) << ", last " << c(s.back()) << "\n";
}

template <class F> static void refused(F &&call) {
  try {
    call();
    std::cout << "no error\n";
  } catch (const pluss::Error &e) {
    std::cout << e.message() << "\n";
  }
}

int main() {
  const auto blue = Rgba::from_u8(30, 80, 200);
  const auto red = Rgba::from_u8(200, 40, 40);

  std::cout << "### light 1\n"; describe(flash::light_palette(blue));
  std::cout << "### light 2\n"; describe(flash::light_palette(blue, 3));
  std::cout << "### light 3\n"; describe(flash::light_palette(red, 10));
  std::cout << "### light 4\n"; refused([&] { flash::light_palette(blue, 1); });

  std::cout << "### dark 1\n"; describe(flash::dark_palette(blue));
  std::cout << "### dark 2\n"; describe(flash::dark_palette(blue, 3));
  std::cout << "### dark 3\n"; describe(flash::dark_palette(red, 10));
  std::cout << "### dark 4\n"; refused([&] { flash::dark_palette(blue, 1); });

  std::cout << "### diverging 1\n"; describe(flash::diverging_palette(blue, red));
  std::cout << "### diverging 2\n"; describe(flash::diverging_palette(blue, red, 3));
  std::cout << "### diverging 3\n"; describe(flash::diverging_palette(red, blue, 11));
  std::cout << "### diverging 4\n"; refused([&] { flash::diverging_palette(blue, red, 2); });

  std::cout << "### husl 1\n"; describe(flash::husl_palette());
  std::cout << "### husl 2\n"; describe(flash::husl_palette(3));
  std::cout << "### husl 3\n"; describe(flash::husl_palette(12));
  std::cout << "### husl 4\n"; refused([&] { flash::husl_palette(0); });

  std::cout << "### cubehelix 1\n"; describe(flash::cubehelix_palette());
  std::cout << "### cubehelix 2\n"; describe(flash::cubehelix_palette(4));
  std::cout << "### cubehelix 3\n"; describe(flash::cubehelix_palette(9));
  std::cout << "### cubehelix 4\n"; refused([&] { flash::cubehelix_palette(0); });

  std::cout << "### crayon 1\n"; describe(flash::crayon_palette());
  std::cout << "### crayon 2\n"; { const auto r = flash::crayon_palette(); std::cout << "name \"" << r.name() << "\"\n"; }
  std::cout << "### xkcd 1\n"; describe(flash::xkcd_palette());
  std::cout << "### xkcd 2\n"; { const auto r = flash::xkcd_palette(); std::cout << "name \"" << r.name() << "\"\n"; }
}
