#include <pluss/pluss.h>
#include "../../PlussQit/src/CodeM/core/StackTrace.h"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace {

// Regressions of the renderer and the trace setters; they run before the larger smoke scenario.
void run_regressions() {
  using namespace pluss::pl;
  // The field of view moves the camera: a perspective view changes with view_angle, and the default of 30 degrees is unchanged.
  {
    const auto view_svg = [](double angle, bool set_angle) {
      Studio fig("view angle");
      auto& pane = fig.pane3("p");
      pane.path3(std::vector<double>{0.0, 1.0, 0.5}, std::vector<double>{0.0, 0.5, 1.0}, std::vector<double>{0.0, 1.0, 0.3});
      pane.lens().project(ProjectionKind::Perspective);
      if (set_angle) {
        pane.lens().view_angle(angle);
      }
      return fig.to_svg();
    };
    assert(view_svg(10.0, true) != view_svg(60.0, true));
    assert(view_svg(30.0, true) == view_svg(0.0, false));
  }

  // Trace style setters refuse invalid values instead of ignoring them.
  {
    Studio fig("trace validation");
    auto& pane = fig.pane2("p");
    Trace& trace = pane.trend(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});
    for (const float bad : {0.0F, -1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
      bool rejected = false;
      try {
        trace.width(bad);
      } catch (const pluss::Error&) {
        rejected = true;
      }
      assert(rejected);
    }
    assert(trace.stroke_width() == 1.5F);
    for (const Range1d bad : {Range1d{2.0, 2.0}, Range1d{3.0, 1.0}, Range1d{0.0, std::numeric_limits<double>::infinity()}}) {
      bool rejected = false;
      try {
        trace.color_limits(bad);
      } catch (const pluss::Error&) {
        rejected = true;
      }
      assert(rejected);
    }
    assert(!trace.color_limits().has_value());
  }

  // A lattice is a wireframe and draws all its edges at the default view; a spike's stem follows the trace width.
  {
    const auto paths = [](const std::string& svg) {
      std::size_t count = 0;
      for (std::size_t at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) {
        ++count;
      }
      return count;
    };
    constexpr std::int64_t n = 6;
    auto coord = pluss::series::linspace(-1.0, 1.0, n, pluss::ScalarType::Float64);
    std::vector<double> heights(static_cast<std::size_t>(n * n));
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        heights[static_cast<std::size_t>(i * n + j)] = 0.3 * static_cast<double>(i) + 0.1 * static_cast<double>(j);
      }
    }
    const auto z = pluss::make_series(heights, {n, n});
    const auto lattice_paths = [&](double azimuth) {
      Studio fig("lattice");
      auto& pane = fig.pane3("p");
      pane.lattice(coord, coord, z);
      pane.lens().look(azimuth, 30.0);
      return paths(fig.to_svg());
    };
    assert(lattice_paths(-37.5) == lattice_paths(140.0));

    const auto spike_svg = [](float width) {
      Studio fig("spike");
      fig.pane2("p").spike(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 3.0, 2.0}).width(width);
      return fig.to_svg();
    };
    assert(spike_svg(1.0F) != spike_svg(4.0F));
  }

  // The figure size is validated; save_png refuses a raster it could never produce.
  {
    Studio fig("sizes");
    fig.pane2("p").trend(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});
    const auto rejects = [](const std::function<void()>& call) {
      try {
        call();
      } catch (const pluss::Error&) {
        return true;
      }
      return false;
    };
    const double nan = std::numeric_limits<double>::quiet_NaN();
    assert(rejects([&] { fig.size(0.0, 100.0); }));
    assert(rejects([&] { fig.size(100.0, -1.0); }));
    assert(rejects([&] { fig.size(nan, 100.0); }));
    assert(fig.extent().width == Studio("x").extent().width);
    assert(rejects([&] { fig.save_png("sizes.png", 100000000); }));
  }

  // References to panes and traces stay valid while more are added.
  {
    Studio fig("stable");
    Pane& first = fig.pane2("first");
    Trace& line = first.trend(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});
    for (int i = 0; i < 64; ++i) {
      fig.pane2("more");
      first.trend(std::vector<double>{1.0, 2.0}, std::vector<double>{2.0, 1.0});
    }
    first.title("renamed");
    line.label("still the first trace");
    assert(&fig.panes()[0] == &first);
    assert(&first.traces()[0] == &line);
    assert(fig.panes()[0].title_text() == "renamed");
    assert(first.traces()[0].label_text() == "still the first trace");
  }

  // Captions and docked legends only claim layout space when something is drawn in it.
  {
    const std::vector<double> x{1.0, 2.0, 3.0};
    const std::vector<double> y{1.0, 3.0, 2.0};
    const auto plain = [&] {
      Studio fig("space");
      fig.pane2("p").trend(x, y);
      return fig.to_svg();
    }();
    {
      Studio fig("space");
      fig.pane2("p").trend(x, y);
      fig.x_caption("orphan", "never drawn").y_caption("orphan", "never drawn");
      assert(fig.to_svg() == plain);
    }
    {
      Studio fig("space");
      fig.pane2("p").trend(x, y);
      fig.legend(KeyDock::East);  // nothing is labelled, so nothing is docked
      assert(fig.to_svg() == plain);
    }
    {
      Studio fig("space");
      fig.pane2("p").trend(x, y).label("series");
      const auto without = fig.to_svg();
      fig.legend(KeyDock::East);
      assert(fig.to_svg() != without);
    }
    {
      // A shared y caption is turned to read upward and stays inside the figure, clear of the tick labels.
      Studio fig("space");
      auto& pane = fig.pane2("p");
      pane.trend(x, y);
      pane.align_y("g");
      fig.y_caption("g", "ycap");
      const std::string svg = fig.to_svg();
      const auto at = svg.find(">ycap<");
      assert(at != std::string::npos);
      assert(svg.rfind("rotate(-90", at) > svg.rfind("<text", at));
    }
    {
      Studio fig("space");
      auto& pane = fig.pane2("p");
      pane.trend(x, y).label("series");
      pane.align_x("g");
      fig.x_caption("g", "drawn");
      assert(fig.to_svg().find(">drawn<") != std::string::npos);
    }
  }

  // An error reads like a Python traceback: frames from main down to the failing line, then the message.
  {
    Studio fig("traceback");
    try {
      fig.size(-1.0, 100.0);  // the failing line the traceback must point at
      assert(false);
    } catch (const pluss::Error& e) {
      const std::string what = e.what();
      assert(what.rfind("Traceback (most recent call last):", 0) == 0);
      assert(what.find("studio.cpp") != std::string::npos);
      assert(what.find("pluss::Error: size: width and height must be finite and positive") != std::string::npos);
      assert(e.message() == "size: width and height must be finite and positive");
      assert(e.traceback().find("\n") != std::string::npos);
    }
  }

  // An error raised while another is being handled carries it, and prints it first, as Python does.
  {
    Studio fig("chained");
    try {
      try {
        fig.size(-1.0, 1.0);
      } catch (const pluss::Error&) {
        fig.size(1.0, -1.0);
      }
      assert(false);
    } catch (const pluss::Error& e) {
      assert(e.cause() != nullptr);
      const std::string what = e.what();
      const auto during = what.find("During handling of the above exception, another exception occurred:");
      assert(during != std::string::npos);
      assert(what.find("Traceback (most recent call last):") < during);
      assert(what.find("Traceback (most recent call last):", during) != std::string::npos);
    }
    try {
      fig.size(-1.0, 1.0);
    } catch (const pluss::Error& e) {
      assert(e.cause() == nullptr);
      assert(std::string(e.what()).find("During handling") == std::string::npos);
    }
  }

  // An error raised from another on purpose says so, and a long run of one frame is collapsed.
  {
    try {
      try {
        throw std::invalid_argument("negative input");
      } catch (...) {
        PLUSS_FAIL_FROM("could not load", std::current_exception());
      }
    } catch (const pluss::Error& e) {
      const std::string what = e.what();
      assert(what.rfind("std::invalid_argument: negative input", 0) == 0);
      assert(what.find("The above exception was the direct cause of the following exception:") != std::string::npos);
    }
    std::function<void(int)> dive = [&](int n) {
      if (n == 0) PLUSS_FAIL("bottom");
      dive(n - 1);
    };
    try {
      dive(30);
    } catch (const pluss::Error& e) {
      assert(std::string(e.what()).find("[Previous line repeated") != std::string::npos);
    }
  }

  // Alignment groups share one range; limits set by hand are the group's window.
  {
    auto ticks = [](bool grouped, bool manual) {
      Studio fig("align_regression");
      fig.mosaic(1, 2);
      auto& l = fig.pane2("l");
      l.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 1, 2, 3});
      auto& r = fig.pane2("r");
      r.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 10, 20, 30});
      if (grouped) {
        l.align_y("g");
        r.align_y("g");
      }
      if (manual) l.y_limits(Range1d{0.0, 5.0});
      return fig.to_svg();
    };
    assert(ticks(true, false) != ticks(false, false));
    assert(ticks(true, true).find(">30<") == std::string::npos);
    assert(ticks(true, true).find(">5<") != std::string::npos);
  }

  // The colour limit method shapes the colour scale; a guide with no colour scale to show reserves no room.
  {
    auto guide = [](LimitMethod method) {
      Studio fig("color_method");
      auto& pane = fig.pane2("p");
      pane.cloud(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 2, 3, 4}, std::vector<double>{5, 5, 5, 5},
                 std::vector<double>{0.13, 0.4, 0.62, 0.97});
      pane.scale_guide(true);
      pane.color_limit_method(method);
      return fig.to_svg();
    };
    assert(guide(LimitMethod::Tight) != guide(LimitMethod::TickAligned));
    auto trend = [](bool on) {
      Studio fig("guide_without_scale");
      auto& pane = fig.pane2("p");
      pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
      pane.scale_guide(on);
      return fig.to_svg();
    };
    assert(trend(true) == trend(false));
    auto manual = [](bool set) {
      Studio fig("cloud_color_limits");
      auto& pane = fig.pane2("p");
      pane.cloud(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 2, 3, 4}, std::vector<double>{5, 5, 5, 5},
                 std::vector<double>{0.13, 0.4, 0.62, 0.97});
      if (set) pane.color_limits(Range1d{0.0, 10.0});
      return fig.to_svg();
    };
    assert(manual(true) != manual(false));
  }

  // Auto puts the legend in the emptiest corner; a legend with nothing to list keeps no room.
  {
    auto build = [](bool falling, KeyPlace place) {
      Studio fig("auto_legend");
      auto& pane = fig.pane2("p");
      std::vector<double> x{1, 2, 3, 4, 5};
      pane.trend(x, falling ? std::vector<double>{6, 5, 4, 3, 2} : std::vector<double>{2, 3, 4, 5, 6}).label("a");
      pane.trend(x, falling ? std::vector<double>{5, 4, 3, 2, 1} : std::vector<double>{1, 2, 3, 4, 5}).label("b");
      pane.legend(place);
      return fig.to_svg();
    };
    assert(build(true, KeyPlace::Auto) == build(true, KeyPlace::NorthEast));
    assert(build(false, KeyPlace::Auto) == build(false, KeyPlace::NorthWest));
    auto bare = [](KeyPlace place) {
      Studio fig("bare_legend");
      auto& pane = fig.pane2("p");
      pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3});
      pane.legend(place);
      return fig.to_svg();
    };
    assert(bare(KeyPlace::OutsideEast) == bare(KeyPlace::None));
  }

  // The lens target moves the picture, in both projections.
  {
    auto build = [](ProjectionKind kind, bool targeted) {
      Studio fig("lens_target");
      auto& pane = fig.pane3("p");
      pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
      pane.lens().project(kind);
      if (targeted) pane.lens().target_at(Vec3d{0.4, 0.0, 0.0});
      return fig.to_svg();
    };
    assert(build(ProjectionKind::Orthographic, true) != build(ProjectionKind::Orthographic, false));
    assert(build(ProjectionKind::Perspective, true) != build(ProjectionKind::Perspective, false));
  }

  // Error messages name the method that was called; a whisker length cannot be negative.
  {
    auto message = [](auto&& call) {
      Studio fig("messages");
      try {
        call(fig);
      } catch (const pluss::Error& e) {
        return e.message();
      }
      return std::string("no error");
    };
    const auto field = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3});
    assert(message([&](Studio& f) { f.pane2("p").columns_h(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2}); }).rfind("columns_h:", 0) == 0);
    assert(message([&](Studio& f) { f.pane3("p").ridge3(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, field, 0); }).rfind("ridge3:", 0) == 0);
    assert(message([&](Studio& f) { f.pane2("p").isoband(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, field, 0); }).rfind("isoband:", 0) == 0);
    assert(message([&](Studio& f) { f.pane2("p").whisk(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{-0.1, 0.1, 0.1}); }).find("non-negative") != std::string::npos);
  }

  // A whisker length given as a one-element vector is one length for every point.
  {
    Studio fig("whisk_scalar");
    auto& pane = fig.pane2("p");
    pane.whisk(std::vector<double>{3, 5, 4}, std::vector<double>{0.7});
    assert(!fig.to_svg().empty());
  }

  // Bars are as wide as their spacing allows: the same picture at any scale of positions.
  {
    auto bars = [](double scale) {
      Studio fig("bar_width");
      auto& pane = fig.pane2("p");
      pane.columns(std::vector<double>{scale, 2 * scale, 5 * scale, 6 * scale}, std::vector<double>{3, 5, 2, 6});
      pane.x_tick_labels(false);
      pane.y_tick_labels(false);
      return fig.to_svg();
    };
    assert(bars(1.0) == bars(10.0));
  }

  // A figure draws with the style it is given; the style is checked and does not leak into other figures.
  {
    auto build = [](double tick_font) {
      Studio fig("style_settings");
      auto& pane = fig.pane2("p");
      pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
      if (tick_font > 0.0) {
        StyleOptions options = fig.style_options();
        options.tick_font_pt = tick_font;
        fig.style(options);
      }
      return fig.to_svg();
    };
    const std::string plain = build(0.0);
    assert(build(16.0) != plain);
    assert(build(0.0) == plain);
    assert(build(10.0) == plain);  // the default value given explicitly is the default
    Studio fig("bad_style");
    StyleOptions bad;
    bad.tick_font_pt = std::numeric_limits<double>::quiet_NaN();
    bool threw = false;
    try {
      fig.style(bad);
    } catch (const pluss::Error&) {
      threw = true;
    }
    assert(threw);
  }

  // Mathematics in text: scripts take the style's script scale, in SVG and PDF alike.
  {
    auto build = [](double script_scale) {
      Studio fig("math_scale");
      auto& pane = fig.pane2("$x^2$ and $y_i$");
      pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
      StyleOptions options = fig.style_options();
      options.math_script_scale = script_scale;
      fig.style(options);
      return std::make_pair(fig.to_svg(), fig.to_pdf());
    };
    const auto normal = build(0.72);
    const auto small = build(0.5);
    assert(normal.first.find("font-size=\"72%\"") != std::string::npos);
    assert(small.first.find("font-size=\"50%\"") != std::string::npos);
    assert(normal.second != small.second);
  }

  // Flash: a kind given as text is checked, and a summary table is as wide as its entries.
  {
    Studio fig("kinds");
    auto& pane = fig.pane2("p");
    std::string message;
    try {
      flash::relplot(pane, std::vector<double>{0, 1}, std::vector<double>{0, 1}, "scater");
    } catch (const pluss::Error& e) {
      message = e.message();
    }
    assert(message.find("scatter") != std::string::npos);
    bool joint_threw = false;
    try {
      Studio other("joint");
      flash::jointplot(other, std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, "hexagon");
    } catch (const pluss::Error&) {
      joint_threw = true;
    }
    assert(joint_threw);

    Studio table("summary");
    flash::summaryplot(table, std::vector<pluss::Series>{pluss::make_series(std::vector<double>{1.5, 2.5, 3.5})},
                       std::vector<std::string_view>{"a_rather_long_feature_name"});
    const std::string caption = table.panes().front().caption_text();
    std::size_t first = std::string::npos;
    std::size_t at = 0;
    while (at < caption.size()) {
      const std::size_t end = caption.find('\n', at);
      const std::size_t length = (end == std::string::npos ? caption.size() : end) - at;
      if (first == std::string::npos) first = length;
      assert(length == first || caption.compare(at, 3, "...") == 0);
      if (end == std::string::npos) break;
      at = end + 1;
    }
    assert(caption.find("a_rather_long_feature_name") != std::string::npos);
  }

  // The text parts of a traceback behave the same on every platform: paths of any machine are shown relative to the
  // repository, and Linux addr2line output is read frame by frame.
  {
    using pluss::codem::StackTrace;
    const std::string unix_this = "/home/builder/PlussDev/PlussQit/src/CodeM/core/StackTrace.cpp";
    assert(StackTrace::shorten_path("/home/builder/PlussDev/PlussPl/src/x.cpp", unix_this) == "PlussPl/src/x.cpp");
    assert(StackTrace::shorten_path("/usr/include/c++/v1/vector", unix_this) == "/usr/include/c++/v1/vector");
    assert(StackTrace::shorten_path("./local/file.cpp", unix_this) == "local/file.cpp");
    const std::string windows_this = "C:\\build\\PlussDev\\PlussQit\\src\\CodeM\\core\\StackTrace.cpp";
    assert(StackTrace::shorten_path("C:\\build\\PlussDev\\PlussPl\\src\\x.cpp", windows_this) == "PlussPl/src/x.cpp");
    assert(StackTrace::shorten_path("c:/BUILD/plussdev/PlussPl/src/x.cpp", windows_this) == "PlussPl/src/x.cpp");
    pluss::codem::Frame frame;
    assert(StackTrace::parse_addr2line("pluss::run(int)", "/a/b.cpp:42 (discriminator 3)", frame));
    assert(frame.function == "pluss::run(int)" && frame.file == "/a/b.cpp" && frame.line == 42);
    assert(StackTrace::parse_addr2line("main", "C:\\src\\main.cpp:7", frame));
    assert(frame.file == "C:\\src\\main.cpp" && frame.line == 7);
    assert(!StackTrace::parse_addr2line("??", "??:0", frame));
    assert(!StackTrace::parse_addr2line("f", "/a/b.cpp:0", frame));
  }

  // Flash: an invalid font scale is refused, not replaced by a default.
  {
    Studio fig("pair_scale");
    flash::PairPlotOptions options;
    options.tick_font_scale = -1.0;
    bool threw = false;
    try {
      flash::pairplot(fig, pluss::msharp::RangeView::Series(std::vector<pluss::Series>{pluss::make_series(std::vector<double>{1, 2, 3}), pluss::make_series(std::vector<double>{3, 1, 2})}), options);
    } catch (const pluss::Error& e) {
      threw = std::string(e.message()).find("tick_font_scale") != std::string::npos;
    }
    assert(threw);
  }

  // Flash: a count below its minimum is refused, naming the function and the option.
  {
    std::string message;
    try {
      (void)flash::dark_palette(Rgba::from_u8(10, 20, 200), 1);
    } catch (const pluss::Error& e) {
      message = e.message();
    }
    assert(message.find("flash::dark_palette") != std::string::npos && message.find("at least 2") != std::string::npos);
    message.clear();
    try {
      Studio fig("fit_points");
      flash::RegPlotOptions options;
      options.fit_points = 1;
      flash::regplot(fig.pane2("p"), std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 3, 2, 4}, options);
    } catch (const pluss::Error& e) {
      message = e.message();
    }
    assert(message.find("fit_points") != std::string::npos);
  }

  // A lens view angle outside (0, 180) is refused, not clamped.
  {
    Pane* none = nullptr;
    (void)none;
    Studio fig("lens_angle");
    auto& pane = fig.pane3("p");
    bool threw = false;
    try {
      pane.lens().view_angle(0.0);
    } catch (const pluss::Error&) {
      threw = true;
    }
    assert(threw);
    threw = false;
    try {
      pane.lens().view_angle(180.0);
    } catch (const pluss::Error&) {
      threw = true;
    }
    assert(threw);
    pane.lens().view_angle(1.0);
    assert(pane.lens().view_angle_deg() == 1.0);
  }

  // Notes ride along when an error is rethrown, and a no-context error leaves the handled one out.
  {
    Studio fig("notes");
    try {
      try {
        fig.size(-1.0, 1.0);
      } catch (pluss::Error& e) {
        e.add_note("while sizing the figure");
        throw;
      }
    } catch (const pluss::Error& e) {
      assert(e.notes().size() == 1);
      const std::string what = e.what();
      assert(what.find("pluss::Error: size: width and height must be finite and positive\nwhile sizing the figure") != std::string::npos);
    }
    try {
      try {
        fig.size(-1.0, 1.0);
      } catch (const pluss::Error&) {
        PLUSS_FAIL_NO_CONTEXT("a new problem");
      }
    } catch (const pluss::Error& e) {
      assert(e.cause() == nullptr);
      assert(std::string(e.what()).find("During handling") == std::string::npos);
    }
  }

  // Grouped errors keep each member's traceback; unknown names get a "Did you mean" suggestion.
  {
    std::vector<std::exception_ptr> errors;
    try {
      Studio("group").size(-1.0, 1.0);
    } catch (...) {
      errors.push_back(std::current_exception());
    }
    errors.push_back(std::make_exception_ptr(std::invalid_argument("second")));
    try {
      PLUSS_FAIL_GROUP("two things failed", errors);
    } catch (const pluss::ErrorGroup& e) {
      assert(e.errors().size() == 2);
      const std::string what = e.what();
      assert(what.find("+ Exception Group Traceback (most recent call last):") != std::string::npos);
      assert(what.find("pluss::ErrorGroup: two things failed (2 sub-exceptions)") != std::string::npos);
      assert(what.find("+-+---------------- 1 ----------------") != std::string::npos);
      assert(what.find("std::invalid_argument: second") != std::string::npos);
    }
    const std::vector<std::string> palettes{"thermal", "ocean", "forest"};
    assert(pluss::exception::closest_match("thermel", palettes) == "thermal");
    assert(pluss::exception::closest_match("zzzzzz", palettes).empty());
    assert(pluss::exception::unknown_name_message("palette", "oceam", palettes).find("Did you mean 'ocean'?") != std::string::npos);
  }

  // The error finds "Did you mean" candidates itself from the vocabularies that were declared for the kind
  // of thing the message names; raise sites pass nothing.
  {
    static const bool declared = pluss::exception::names::define("smoke palette", {"thermal", "ocean", "forest"});
    (void)declared;
    try {
      pluss::validate::check(false, "unknown smoke palette 'thermel'");
      assert(false);
    } catch (const pluss::Error& e) {
      assert(e.message() == "unknown smoke palette 'thermel' Did you mean 'thermal'?");
    }
    std::vector<std::string> columns{"price", "quantity"};
    {
      pluss::exception::names::Scope scope("smoke column", [&] { return columns; });
      try {
        PLUSS_FAIL("no smoke column 'quantitty'");
      } catch (const pluss::Error& e) {
        assert(e.message().find("Did you mean 'quantity'?") != std::string::npos);
      }
    }
    try {
      PLUSS_FAIL("no smoke column 'quantitty'");
    } catch (const pluss::Error& e) {
      assert(e.message().find("Did you mean") == std::string::npos);
    }
  }

  // The headline wraps to the figure width, honours line breaks, and a blank headline takes no room.
  {
    const auto build = [](const std::string& headline) {
      Studio fig("headline");
      fig.size(700.0, 300.0);
      fig.headline(headline);
      fig.pane2("p").trend(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});
      return fig.to_svg();
    };
    const std::string none = build("");
    assert(build("   \n  ") == none);
    const std::string wrapped = build(std::string("a very long headline that goes on and on past the width of the figure and must wrap ").append(60, 'x') + " end");
    assert(wrapped.find("long headline that goes on and on past the width of the figure and must wrap ") == std::string::npos);
    assert(wrapped.find("end<") != std::string::npos);
    const std::string lines = build("first line\nsecond line");
    assert(lines.find(">first line<") != std::string::npos && lines.find(">second line<") != std::string::npos);
    assert(lines != build("first line"));
  }

  // Sub-panes that clash are refused when they are created.
  {
    Studio fig("subpanes");
    fig.pane2("parent");
    fig.subpane2(0, 1, 2, 0, 0, 1, 1, "left");
    const auto refused = [&](const std::function<void()>& call) {
      try {
        call();
      } catch (const pluss::Error&) {
        return true;
      }
      return false;
    };
    assert(refused([&] { fig.subpane2(0, 1, 2, 0, 0, 1, 1, "again"); }));
    assert(refused([&] { fig.subpane3(0, 2, 2, 1, 1, 1, 1, "other grid"); }));
    assert(!refused([&] { fig.subpane2(0, 1, 2, 0, 1, 1, 1, "right"); }));
    assert(fig.pane_count() == 3);
  }

  // Axis gaps are honoured on 3D panes as well as planar ones.
  {
    const auto svg = [](const std::function<void(Pane&)>& style) {
      Studio fig("gaps3d");
      auto& pane = fig.pane3("p");
      pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
      pane.x_label("x").y_label("y").z_label("z");
      style(pane);
      return fig.to_svg();
    };
    const std::string plain = svg([](Pane&) {});
    assert(svg([](Pane& p) { p.x_tick_label_gap(40.0); }) != plain);
    assert(svg([](Pane& p) { p.y_tick_label_gap(40.0); }) != plain);
    assert(svg([](Pane& p) { p.z_tick_label_gap(40.0); }) != plain);
    assert(svg([](Pane& p) { p.x_label_gap(70.0); }) != plain);
    assert(svg([](Pane& p) { p.y_label_gap(70.0); }) != plain);
    assert(svg([](Pane& p) { p.z_label_gap(70.0); }) != plain);
  }

  // Data outside a manual axis range is clipped to the plot area, in the SVG and in the PDF.
  {
    Studio fig("clip");
    fig.pane2("p").trend(std::vector<double>{0, 1, 2, 3, 4}, std::vector<double>{0, 1, 0, 1, 0}).label("a");
    const std::string automatic = fig.to_svg();
    assert(automatic.find("<clipPath") != std::string::npos);  // traces are always drawn inside the plot area
    assert(fig.to_pdf().find(" re W n") != std::string::npos);
    assert(fig.to_pdf().rfind("%PDF-1.4", 0) == 0);
  }

  // On a 3D pane the axis label stays clear of the tick labels: longer labels, and labels turned upright, push it out.
  {
    const auto label_position = [](const char* tick_label, double rotation) {
      Studio fig("reach");
      auto& pane = fig.pane3("p");
      pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
      pane.x_label("AXISLABEL");
      pane.x_ticks(std::vector<double>{0.0, 1.0, 2.0}, std::vector<std::string_view>{tick_label, tick_label, tick_label});
      pane.x_tick_rotation(rotation);
      const std::string svg = fig.to_svg();
      const auto at = svg.find(">AXISLABEL<");
      assert(at != std::string::npos);
      return svg.substr(svg.rfind("<text", at), at - svg.rfind("<text", at));
    };
    assert(label_position("a", 0.0) != label_position("a much longer tick label", 0.0));
    assert(label_position("tick", 0.0) != label_position("tick", 90.0));
  }

  // Cues outside a manual range are not emitted; a band that is partly inside is trimmed to the range.
  {
    const auto svg = [](const std::function<void(Pane&)>& cues) {
      Studio fig("cues");
      auto& pane = fig.pane2("p");
      pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
      pane.x_limits(Range1d{0.0, 5.0}).y_limits(Range1d{0.0, 6.0});
      cues(pane);
      return fig.to_svg();
    };
    const std::string plain = svg([](Pane&) {});
    assert(svg([](Pane& p) { p.rule_x(40.0, "far"); }) == plain);
    assert(svg([](Pane& p) { p.rule_y(40.0, "far"); }) == plain);
    assert(svg([](Pane& p) { p.band_x(Range1d{20.0, 30.0}, "far"); }) == plain);
    assert(svg([](Pane& p) { p.pin("far", 2.0, 40.0); }) == plain);
    assert(svg([](Pane& p) { p.rule_x(2.5, "near"); }) != plain);
    assert(svg([](Pane& p) { p.band_x(Range1d{4.0, 9.0}, "edge"); }) != plain);
  }

  // On a planar pane the room reserved below the axis is the measured reach of the tick labels: longer labels and
  // labels turned upright take more of it, so the plot area shrinks and the axis label never meets them.
  {
    const auto plot_height = [](const char* tick_label, double rotation) {
      Studio fig("planar reach");
      auto& pane = fig.pane2("p");
      pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
      pane.x_label("AXISLABEL");
      pane.x_ticks(std::vector<double>{0.0, 1.0, 2.0}, std::vector<std::string_view>{tick_label, tick_label, tick_label});
      pane.x_tick_rotation(rotation);
      const std::string svg = fig.to_svg();
      const auto group = svg.find("<g clip-path");
      assert(group != std::string::npos);
      const auto path = svg.find("<path d=\"", group) + 9;
      const auto end = svg.find('"', path);
      std::istringstream numbers(svg.substr(path, end - path));
      double lowest = 1e300, highest = -1e300;
      std::string token;
      std::vector<double> values;
      while (numbers >> token) {
        if (token == "M" || token == "L" || token == "Z") continue;
        values.push_back(std::stod(token));
      }
      for (std::size_t k = 1; k < values.size(); k += 2) {
        lowest = std::min(lowest, values[k]);
        highest = std::max(highest, values[k]);
      }
      return highest - lowest;
    };
    assert(plot_height("a much longer tick label", 60.0) < plot_height("a", 60.0));
    assert(plot_height("tick label", 90.0) < plot_height("tick label", 60.0));
    assert(plot_height("tick label", 60.0) < plot_height("tick label", 0.0));
  }

  // No silent cuts: a long headline shows every line, and a legend label is not truncated while the key could be wider.
  {
    Studio headline_fig("lines");
    headline_fig.headline("one\ntwo\nthree\nfour\nfive\nsix");
    headline_fig.pane2("p").trend(std::vector<double>{1, 2}, std::vector<double>{1, 2});
    const std::string svg = headline_fig.to_svg();
    for (const char* word : {">one<", ">two<", ">three<", ">four<", ">five<", ">six<"}) assert(svg.find(word) != std::string::npos);

    Studio key_fig("key");
    key_fig.size(1500.0, 420.0);
    key_fig.legend(KeyDock::East);
    key_fig.pane2("p").trend(std::vector<double>{0, 1}, std::vector<double>{0, 1})
        .label("seasonally adjusted quarterly revenue per region over the whole trailing period");
    assert(key_fig.to_svg().find("...") == std::string::npos);
  }

  // Margins and the default mosaic gutter are shares of the figure, so a figure scaled up is laid out in proportion.
  {
    const auto second_title_center = [](double width) {
      Studio fig("proportions");
      fig.size(width, width * 0.5);
      fig.mosaic(1, 2);
      fig.pane2("FIRSTPANE").trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
      fig.pane2("SECONDPANE").trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
      const std::string svg = fig.to_svg();
      const auto at = svg.find(">SECONDPANE<");
      assert(at != std::string::npos);
      const auto start = svg.rfind("<text", at);
      const auto x_at = svg.find(" x=\"", start) + 4;
      return std::stod(svg.substr(x_at, svg.find('"', x_at) - x_at)) / width;
    };
    assert(std::abs(second_title_center(800.0) - second_title_center(1600.0)) < 0.03);
    Studio fig("gutters");
    fig.mosaic(1, 2);
    assert(fig.mosaic_layout().gutter_x == auto_gutter && fig.mosaic_layout().gutter_y == auto_gutter);
    fig.mosaic(1, 2, 30.0, 12.0);
    assert(fig.mosaic_layout().gutter_x == 30.0 && fig.mosaic_layout().gutter_y == 12.0);
    bool refused = false;
    try {
      fig.mosaic(1, 2, -5.0, 10.0);
    } catch (const pluss::Error&) {
      refused = true;
    }
    assert(refused);
  }

  // Legend shape changes the layout of a many-entry legend; a local light and an infinite one shade a surface differently;
  // cue labels anchored at the same point are placed apart instead of on top of each other.
  {
    const auto legend_svg = [](KeyShape shape) {
      Studio fig("legend");
      auto& pane = fig.pane2("p");
      for (int i = 0; i < 6; ++i) {
        pane.trend(std::vector<double>{0.0, 1.0}, std::vector<double>{static_cast<double>(i), static_cast<double>(i + 1)})
            .label("a rather long series label number " + std::to_string(i));
      }
      pane.legend(KeyPlace::NorthEast).legend_shape(shape);
      return fig.to_svg();
    };
    assert(legend_svg(KeyShape::Horizontal) != legend_svg(KeyShape::Vertical));

    constexpr std::int64_t n = 8;
    auto coord = pluss::series::linspace(-1.0, 1.0, n, pluss::ScalarType::Float64);
    std::vector<double> heights(static_cast<std::size_t>(n * n));
    for (std::int64_t i = 0; i < n * n; ++i) {
      heights[static_cast<std::size_t>(i)] = std::sin(0.3 * static_cast<double>(i));
    }
    const auto z = pluss::make_series(heights, {n, n});
    const auto lit_svg = [&](LightStyle style) {
      Studio fig("light");
      auto& pane = fig.pane3("p");
      pane.sheet(coord, coord, z).lighting(LightingMode::Gouraud);
      pane.lights().clear();
      pane.lights().add(LightSpec{30.0, 30.0, LightSpace::Camera, style});
      return fig.to_svg();
    };
    assert(lit_svg(LightStyle::Local) != lit_svg(LightStyle::Infinite));

    Studio fig("cues");
    auto& pane = fig.pane2("p");
    pane.trend(std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 1.0});
    pane.pin("first-cue", 0.5, 0.5);
    pane.pin("second-cue", 0.5, 0.5);
    const std::string svg = fig.to_svg();
    const auto position = [&](const std::string& label) {
      const std::size_t at = svg.find(label);
      assert(at != std::string::npos);
      return svg.substr(svg.rfind("<text", at), at - svg.rfind("<text", at));
    };
    assert(position("first-cue") != position("second-cue"));
  }

  // A filled kind is filled from the palette until its colour is set, and then takes it; an explicit width sets its outline.
  {
    const auto changes = [](auto make, auto style) {
      std::string svg[2];
      for (int i = 0; i < 2; ++i) {
        Studio fig("filled");
        Trace& trace = make(fig.pane2("p"));
        if (i == 1) {
          style(trace);
        }
        svg[i] = fig.to_svg();
      }
      return svg[0] != svg[1];
    };
    const std::vector<double> x{1.0, 2.0, 3.0, 4.0};
    const std::vector<double> y{2.0, 4.0, 3.0, 5.0};
    const auto stroke = [](Trace& trace) { trace.stroke(Rgba::from_u8(200, 30, 30)); };
    const auto width = [](Trace& trace) { trace.width(4.0F); };
    for (const auto& style : {std::function<void(Trace&)>(stroke), std::function<void(Trace&)>(width)}) {
      assert(changes([&](Pane& p) -> Trace& { return p.columns(x, y); }, style));
      assert(changes([&](Pane& p) -> Trace& { return p.bucket(y); }, style));
      assert(changes([&](Pane& p) -> Trace& { return p.ribbon(x, y); }, style));
      assert(changes([&](Pane& p) -> Trace& { return p.wedge(std::vector<double>{1.0, 2.0, 3.0}); }, style));
    }
    // Creating a trace does not count as setting its colour: the theme accent is only a default.
    Studio fig("defaults");
    Trace& trace = fig.pane2("p").columns(x, y);
    assert(!trace.stroke_explicit() && !trace.width_explicit());
  }
}

} // namespace

int main() {
  run_regressions();
  using namespace pluss::pl;

  Studio book("core-smoke", Theme::ocean());
  book.size(960.0, 720.0);
  book.headline("Core studio");
  book.legend(KeyDock::East).legend_frame(KeyFrame::Soft).x_caption("shared-signal", "shared signal axis");
  Pane& pane2 = book.pane2("signals");
  pane2.grid(GridStyle::MajorMinor)
      .scale_x(ScalePolicy::log10())
      .x_label("frequency")
      .y_label("$\\mu + \\sigma + \\gamma + \\omega + \\alpha + \\beta + \\lambda$")
      .legend(KeyPlace::NorthEast)
      .legend_frame(KeyFrame::Soft)
      .align_x("shared-signal")
      .caption("$\\Delta\\sigma_t \\leq \\omega$")
      .rule_x(10.0, "pivot")
      .scale_guide();

  Trace& trend = pane2.trend(std::vector<double>{1.0, 10.0, 100.0},
                             std::array<double, 3>{0.5, 0.8, 1.2});
  trend.label("response").width(2.0F);

  Trace& radial = pane2.radial(std::array<double, 3>{0.0, 1.0, 2.0},
                               std::array<double, 3>{1.0, 0.5, 1.5});
  radial.label("radial");
  pane2.cloud(std::array<double, 3>{1.0, 10.0, 100.0},
              std::array<double, 3>{0.6, 0.9, 1.1},
              std::array<double, 3>{18.0, 24.0, 30.0})
      .label("cloud");
  pane2.bucket(std::array<double, 6>{-1.0, -0.3, 0.0, 0.25, 0.9, 1.1}, 4)
      .label("bucket");
  pane2.pin("crest", 100.0, 1.2);

  Pane& pane3 = book.pane3("surface");
  pane3.grid(GridStyle::Major)
      .x_label("x")
      .y_label("y")
      .z_label("z")
      .caption("surface pane")
      .rule_z(1.0, "slice");
  pane3.lens().spin(12.0, 8.0);
  pane3.lights().sun_key();

  auto z = pluss::make_series(std::vector<double>{0.0, 1.0, 1.5, 3.0}, {2, 2});
  auto c = pluss::make_series(std::vector<double>{0.1, 0.4, 0.6, 0.9}, {2, 2});
  Trace& sheet = pane3.sheet(std::array<double, 2>{0.0, 1.0},
                             std::array<double, 2>{0.0, 1.0},
                             z,
                             c);
  sheet.label("sheet");

  pane3.lattice(std::array<double, 2>{0.0, 1.0},
                std::array<double, 2>{0.0, 1.0},
                z);

  assert(book.pane_count() == 2);
  // pane2 was created before pane3, and creating a pane can move the earlier ones, so read it again through the figure.
  const Pane& first = book.panes()[0];
  assert(first.trace_count() == 4);
  assert(pane3.trace_count() == 2);
  assert(pane3.lights().lights().size() == 2);
  assert(book.panes()[1].traces()[0].kind() == TraceKind::Sheet); // `sheet` itself is invalidated by the next trace
  assert(first.x_axis().scale.kind == ScaleKind::Log10);
  assert(first.x_axis().major_grid);
  assert(first.x_axis().minor_grid);
  assert(pane3.lens().azimuth_deg() == -25.5);
  assert(pane3.lens().elevation_deg() == 38.0);
  assert(book.legend_frame() == KeyFrame::Soft);
  assert(first.legend_frame() == KeyFrame::Soft);

  const std::string svg = book.to_svg();
  const std::string pdf = book.to_pdf();
  assert(svg.find("<svg") != std::string::npos);
  assert(svg.find("<path") != std::string::npos);
  assert(svg.find("Core studio") != std::string::npos);
  assert(svg.find("cloud") != std::string::npos);
  assert(svg.find("bucket") != std::string::npos);
  assert(svg.find("response") != std::string::npos);
  assert(svg.find("pivot") != std::string::npos);
  assert(pdf.rfind("%PDF-1.4", 0) == 0);
  // Greek letters in math text are written as single glyphs of the Symbol font,
  // so the PDF declares that font and the Latin operators stay as separate chunks.
  assert(pdf.find("/BaseFont /Symbol") != std::string::npos);
  assert(pdf.find("(m) Tj") != std::string::npos);  // mu is Symbol 'm'
  assert(pdf.find("(Ds) Tj") != std::string::npos);  // Delta sigma are Symbol 'D' 's'
  assert(pdf.find("( <= ) Tj") != std::string::npos);

  const std::filesystem::path out_dir = std::filesystem::temp_directory_path() / "plusspl_core_smoke";
  std::filesystem::create_directories(out_dir);
  const std::filesystem::path svg_path = out_dir / "smoke.svg";
  const std::filesystem::path pdf_path = out_dir / "smoke.pdf";
  book.save_svg(svg_path.string());
  book.save_pdf(pdf_path.string());
  assert(std::filesystem::exists(svg_path));
  assert(std::filesystem::exists(pdf_path));
  assert(std::filesystem::file_size(svg_path) > 0);
  assert(std::filesystem::file_size(pdf_path) > 0);

  bool threw = false;
  try {
    book.save_svg((out_dir / "wrong.pdf").string());
  } catch (const std::exception&) {
    threw = true;
  }
  assert(threw);

  threw = false;
  try {
    book.save_pdf((out_dir / "wrong.png").string());
  } catch (const std::exception&) {
    threw = true;
  }
  assert(threw);

  threw = false;
  try {
    book.save_png((out_dir / "wrong.svg").string(), 120);
  } catch (const std::exception&) {
    threw = true;
  }
  assert(threw);
  return 0;
}
