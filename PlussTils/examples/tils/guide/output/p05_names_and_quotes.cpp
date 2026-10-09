// A list of attendee names for a badge run. Names are title-cased, and inside a list each name is
// quoted so a name with a space or a comma is still clear.
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::vector<std::string> attendees{"ama mensah", "kofi, jr", "efua boateng"};

  out.writeln("badges:", attendees, PrintOptions{.quote_strings = true, .quote_char = '"'});
  out.writelnf("title case: {}", "ama mensah", PrintOptions{.case_transform = CaseXf::Title});
  out.writelnf("shout: {}", "room 4", PrintOptions{.case_transform = CaseXf::Upper});
  out.writelnf("sentence: {}", "the BADGE printer is out", PrintOptions{.case_transform = CaseXf::Sentence});
  return 0;
}
