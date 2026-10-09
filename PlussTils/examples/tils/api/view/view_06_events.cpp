#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/renderer.h>
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>
#include <vector>

namespace view = pluss::view::window;

int main() {
  std::cout << std::boolalpha;
  auto renderer = view::make_software_renderer();

  // A normal frame reports that it started and that it finished, in that order.
  view::PixelCanvas canvas(view::SizeI{4, 4});
  view::Scene scene;
  scene.add(view::ClearCommand{view::Color::rgba(0, 0, 0, 255)});
  std::vector<view::RenderEvent> events;
  const bool ok = renderer->render(scene, canvas, events);
  std::cout << "rendered ok: " << ok << '\n';
  std::cout << "event count: " << events.size() << '\n';
  std::cout << "first event started: " << (events.front().kind == view::RenderEventKind::FrameStarted) << '\n';
  std::cout << "last event finished: " << (events.back().kind == view::RenderEventKind::FrameFinished) << '\n';

  // Events accumulate in the vector you pass. Clear it between frames to read each frame separately.
  events.clear();
  renderer->render(scene, canvas, events);
  std::cout << "second frame event count: " << events.size() << '\n';
}
