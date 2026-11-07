// A user profile record. Optional fields show a placeholder when they are empty, flags print as
// words that a reader understands, and a tag marks which values are present.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <optional>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::optional<std::string> nickname;          // not set
  const std::optional<std::string> city{"Accra"};     // set
  const bool subscribed = true;
  const bool two_factor = false;

  out.writeln("nickname:", nickname, PrintOptions{.null_text = "<none>"});
  out.writeln("city:", city, PrintOptions{.null_text = "<none>"});
  out.writeln("subscribed:", subscribed, two_factor, PrintOptions{.bool_style = BoolStyle::YesNo});
  out.writeln("two factor:", subscribed, two_factor, PrintOptions{.bool_style = BoolStyle::OnOff});
  out.writeln("tagged:", nickname, city, PrintOptions{.optional_tag = true});
  return 0;
}
