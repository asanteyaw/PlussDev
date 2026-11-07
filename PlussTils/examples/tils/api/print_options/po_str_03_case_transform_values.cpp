#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("Upper: {}", "hello world", PrintOptions{.case_transform = CaseXf::Upper});
  out.writelnf("Lower: {}", "HELLO WORLD", PrintOptions{.case_transform = CaseXf::Lower});
  out.writelnf("Title: {}", "the quick brown fox", PrintOptions{.case_transform = CaseXf::Title});
  out.writelnf("Sentence: {}", "the QUICK brown fox. a SECOND one.", PrintOptions{.case_transform = CaseXf::Sentence});
  return 0;
}
