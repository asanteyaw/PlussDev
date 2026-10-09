// clear_commands empties a scene's command list, the same way a default-constructed scene starts.
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>

namespace view = pluss::view::window;

int main() {
  view::Scene scene;
  scene.add(view::ClearCommand{});
  scene.add(view::FillRectCommand{});
  std::cout << "commands before clearing: " << scene.commands().size() << '\n';
  scene.clear_commands();
  std::cout << "commands after clearing: " << scene.commands().size() << '\n';
  return 0;
}
