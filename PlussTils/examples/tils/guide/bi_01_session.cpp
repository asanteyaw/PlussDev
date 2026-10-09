#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <variant>

int main() {
  std::cout << std::boolalpha;
  pluss::bi::Session session;
  std::cout << "starts empty: " << session.empty() << '\n';
  std::cout << "starts at version: " << session.version() << '\n';

  // Each change bumps the version, so a view can tell that the session moved.
  session.set_param("region", std::string("north"));
  std::cout << "after one change, version: " << session.version() << '\n';

  // Parameters hold either a number or text. Read them back with their stored type.
  session.set_param("threshold", 2.5);
  const auto threshold = session.param("threshold");
  std::cout << "threshold is numeric: " << std::holds_alternative<double>(*threshold) << '\n';
  std::cout << "threshold value: " << std::get<double>(*threshold) << '\n';
  std::cout << "region is text: " << std::holds_alternative<std::string>(*session.param("region")) << '\n';

  // Clearing a key removes it. Clearing an absent key still counts as a change.
  session.clear_param("region");
  std::cout << "region still set: " << session.has_param("region") << '\n';
  std::cout << "version after clear: " << session.version() << '\n';
  session.clear_param("never_set");
  std::cout << "version after clearing an absent key: " << session.version() << '\n';
}
