// despine, save_png and show: the examples added to their reference pages. Each block between "### name n" markers is one example.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;
namespace fs = std::filesystem;

int main() {
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  const auto y = pluss::make_series({1.0, 4.0, 9.0});

  std::cout << "### despine 2\n";
  {
    auto fig = studio("flash_despine_explicit");
    auto &pane = fig.pane2("curve");
    pane.trend(x, y);
    const std::string before = fig.to_svg();
    flash::despine(pane, false);
    std::cout << "despine(pane, false), svg unchanged: " << (before == fig.to_svg()) << "\n";
  }
  std::cout << "### despine 3\n";
  {
    auto fig = studio("flash_despine_chain");
    fig.mosaic(1, 2);
    auto &left = fig.pane2("left");
    auto &right = fig.pane2("right");
    left.trend(x, y);
    right.trend(x, y);
    flash::despine(left);
    flash::despine(right);
    std::cout << "returns the pane it was given: " << (&flash::despine(left) == &left) << "\n";
    std::cout << "panes after despine: " << fig.pane_count() << "\n";
  }

  std::cout << "### save_png 2\n";
  {
    auto fig = studio("flash_save_png_dpi");
    fig.pane2("curve").trend(x, y);
    const auto low = (fs::temp_directory_path() / "flash_save_png_72.png").string();
    const auto high = (fs::temp_directory_path() / "flash_save_png_216.png").string();
    flash::save_png(fig, low, 72);
    flash::save_png(fig, high, 216);
    std::cout << "the 216 dpi file is larger than the 72 dpi file: " << (fs::file_size(high) > fs::file_size(low)) << "\n";
    fs::remove(low);
    fs::remove(high);
  }
  std::cout << "### save_png 3\n";
  {
    auto fig = studio("flash_save_png_default");
    fig.pane2("curve").trend(x, y);
    const auto path = (fs::temp_directory_path() / "flash_save_png_default.png").string();
    flash::save_png(fig, path);
    std::cout << "default dpi, file written: " << fs::exists(path) << "\n";
    fs::remove(path);
  }

  std::cout << "### show 1\n";
  {
    auto fig = studio("flash_show_nolaunch");
    fig.pane2("curve").trend(x, y);
    std::cout << "show(..., launch=false) returned: " << flash::show(fig, "My Figure", 1000, 700, false) << "\n";
  }
  std::cout << "### show 2\n";
  {
    // With PLUSSPL_CAPTURE_DIR set, show records the figure as <name>.svg in that directory instead of opening a window.
    const auto dir = fs::temp_directory_path() / "flash_show_capture";
    fs::create_directories(dir);
    setenv("PLUSSPL_CAPTURE_DIR", dir.string().c_str(), 1);
    auto fig = studio("flash_show_captured");
    fig.pane2("curve").trend(x, y);
    const bool shown = flash::show(fig);
    unsetenv("PLUSSPL_CAPTURE_DIR");
    std::cout << "show returned " << shown << "; captured file exists: " << fs::exists(dir / "flash_show_captured.svg") << "\n";
    fs::remove_all(dir);
  }
  std::cout << "### show 3\n";
  {
    auto fig = studio("flash_show_equivalence");
    fig.pane2("curve").trend(x, y);
    std::cout << "flash::show and Studio::show agree when not launching: " << (flash::show(fig, {}, 1280, 820, false) == fig.show({}, 1280, 820, false)) << "\n";
  }
}
