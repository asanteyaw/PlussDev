#include <iostream>
#include <limits>
#include <string_view>

#include <pluss/pluss.h>

namespace {

void block(std::string_view name, const std::string& text) {
  std::cout << name << "\n" << text << "\n\n";
}

void block_bool(std::string_view name, bool value) {
  std::cout << name << "\n" << (value ? "true" : "false") << "\n\n";
}

} // namespace

int main() {
  using namespace pluss::table;
  using pluss::make_note;
  using pluss::make_series;

  auto base = DataFrame({
      {"A", make_series({1.0, 2.0, 3.0})},
      {"B", make_series({10.0, 20.0, 30.0})},
  }, make_note({"r1", "r2", "r3"})).title("base");

  block("DF_ALIGN_1",
        base.align(Thread(make_note({"r2", "missing", "r1"}))).to_string());

  block("DF_ALIGN_2",
        base.align(DataFrame::AlignSpec{
            .caselbl = Thread(make_note({"r2", "missing", "r1"})),
            .fill = 0.0,
        }).to_string());

  block("DF_ALIGN_3",
        base.align(DataFrame::AlignSpec{
            .features = {"B", "C", "A"},
        }).to_string());

  block("DF_ALIGN_4",
        base.align(DataFrame::AlignSpec{
            .caselbl = Thread(make_note({"r3", "r1"})),
            .features = {"B", "C", "A"},
        }).to_string());

  block("DF_ALIGN_5",
        base.align(DataFrame::AlignSpec{
            .caselbl = Thread(make_note({"r3", "r1"})),
            .features = {"B", "C", "A"},
            .fill = 0.0,
        }).to_string());

  auto case_driver = DataFrame({
      {"score", make_series({100.0, 200.0})},
  }, make_note({"r3", "r1"}));
  block("DF_ALIGN_6", base.align(case_driver.caselbl()).to_string());

  block("DF_ALIGN_7",
        base.align(DataFrame::AlignSpec{
            .features = {"B"},
        }).to_string());

  block("DF_ALIGN_8", base.align(base.caselbl()).to_string());

  auto labeled = Thread(make_series({10.0, 20.0, 30.0}))
                     .caselbl(Thread(make_note({"b", "a", "c"})));
  block("THREAD_ALIGN_1",
        labeled.align(
                   Thread(make_note({"a", "b", "d"})),
                   AlignOptions{.align = AlignBy::Caselbl})
            .to_string());

  block("THREAD_ALIGN_2",
        labeled.align(
                   Thread(make_note({"a", "b", "d"})),
                   AlignOptions{.align = AlignBy::Caselbl, .fill = 0.0})
            .to_string());

  auto feature_thread = Thread(make_series({1.5, 2.5}))
                            .caselbl(Thread(make_note({"A2", "A1"})));
  block("THREAD_ALIGN_3",
        feature_thread.align(
                          Thread(make_note({"A1", "A2"})),
                          AlignOptions{.align = AlignBy::Feature})
            .to_string());

  block("THREAD_ALIGN_4",
        labeled.align(
                   Thread(make_note({"x", "y", "z"})),
                   AlignOptions{.align = AlignBy::Pos})
            .to_string());

  auto scores = DataFrame({
      {"quiz", make_series({84.0, 90.0, 84.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  block("DF_EQ_1", scores.eq(84.0).to_string());
  block("DF_EQ_2",
        DataFrame({
            {"name", make_note({"Ada", "Bola", "Ada"})},
        }, make_note({"s1", "s2", "s3"})).eq("Ada").to_string());

  auto official = DataFrame({
      {"quiz", make_series({84.0, 90.0, 84.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  auto changed = DataFrame({
      {"quiz", make_series({84.0, 90.0, 83.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  block("DF_EQ_3", official.eq(changed).to_string());

  auto reordered = DataFrame({
      {"final", make_series({72.0, 84.0})},
      {"quiz", make_series({84.0, 84.0})},
  }, make_note({"s3", "s1"}));
  block("DF_EQ_4",
        official.eq(reordered, AlignOptions{.align = AlignBy::Both}).to_string());

  block_bool("DF_EQUALS_1", official.equals(official.copy()));
  block_bool("DF_EQUALS_2", official.equals(changed));

  auto left_extend = DataFrame({
      {"x", make_series({1.0, 2.0})},
      {"y", make_series({10.0, 20.0})},
  }, make_note({"A", "B"}));
  auto right_extend = DataFrame({
      {"y", make_series({30.0, 40.0})},
      {"z", make_series({300.0, 400.0})},
  }, make_note({"C", "D"}));

  block("DF_EXTEND_1", left_extend.extend_cases(right_extend).to_string());
  block("DF_EXTEND_2",
        left_extend.extend_cases(right_extend, FrameMatchRule::Common).to_string());
  block("DF_EXTEND_3",
        left_extend.extend_cases(right_extend, FrameMatchRule::Unite, true).to_string());

  auto thread_nums = Thread(make_series({10.0, 20.0, 30.0}));
  block("THREAD_DIV_1", thread_nums.div(10.0).to_string());
  block("THREAD_DIV_2",
        thread_nums.div(Thread(make_series({2.0, 4.0, 5.0}))).to_string());

  auto urls = Thread(make_note({"A B", "C&D", "email@example.com"}));
  block("THREAD_ENCODE_1", urls.encode("base64").to_string());
  block("THREAD_ENCODE_2", urls.encode("hex").to_string());
  block("THREAD_ENCODE_3", urls.encode("url").to_string());

  auto names = Thread(make_note({"Ada.csv", "Bola.txt", "Chen.csv"}));
  block("THREAD_ENDS_1", names.ends(".csv").to_string());

  block("THREAD_EQ_1", thread_nums.eq(20.0).to_string());
  block("THREAD_EQ_2", names.eq("Bola.txt").to_string());
  block("THREAD_EQ_3",
        thread_nums.eq(Thread(make_series({10.0, 0.0, 30.0}))).to_string());

  block("THREAD_EXP_1", Thread(make_series({0.0, 1.0, 2.0})).exp().to_string());

  auto parts = Thread(make_note({"A-100", "B-205", "C", "D-400"}));
  block("THREAD_EXTRACT_1",
        parts.extract("([A-Z])-([0-9]+)", {"code", "id"}).to_string());
  block("THREAD_EXTRACT_2",
        parts.extract("([A-Z])-([0-9]+)").to_string());

  return 0;
}
