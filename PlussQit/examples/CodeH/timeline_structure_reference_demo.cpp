#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::timeline::print(value) << '\n';
}

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    std::cout << "\n=== " << label << " ===\nrejected\n";
  }
}
} // namespace

int main() {
  const auto jan12 = pluss::timeline::date({{"2024-01-01", "2024-01-02"}});
  const auto jan34 = pluss::timeline::date({{"2024-01-03", "2024-01-04"}});
  const auto jan56 = pluss::timeline::date({{"2024-01-05", "2024-01-06"}});
  show("concat_matrix", jan12.concat(jan34));
  show("concat_many", jan12.concat({jan34, jan56}));
  show("concat_scalars", pluss::timeline::date("2024-01-01")
                             .concat(pluss::timeline::date("2024-01-02")));
  show("concat_empty_list", jan12.concat(std::vector<pluss::Timeline>{}));

  std::array<std::int64_t, 4> backing{
      1'704'067'200'000'000'000LL, 11,
      1'704'153'600'000'000'000LL, 22};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  show("concat_strided", strided.concat(pluss::timeline::date("2024-01-03")));
  show("concat_absent", pluss::timeline::make({"2024-01-01", ""})
                            .concat(pluss::timeline::date("2024-01-03")));

  show_rejection("concat_trailing_shape", [&] {
    const std::vector<std::string> values{
        "2024-01-03", "2024-01-04", "2024-01-05"};
    (void)jan12.concat(pluss::timeline::date(values, {3, 1}));
  });
  show_rejection("concat_timezone", [&] {
    (void)jan12.concat(jan34.tz_convert("America/Edmonton"));
  });
  show_rejection("concat_rank", [&] {
    (void)jan12.concat(pluss::timeline::date("2024-01-03"));
  });

  const auto row = pluss::timeline::date({"2024-01-01", "2024-01-02"});
  show("repeat_vector", row.repeat(2));
  show("repeat_matrix", jan12.repeat(2));
  show("repeat_strided", strided.repeat(2));
  show("repeat_scalar_once", pluss::timeline::date("2024-01-01").repeat(1));
  show("repeat_zero", jan12.repeat(0));
  show_rejection("repeat_negative", [&] { (void)row.repeat(-1); });
  show_rejection("repeat_overflow", [&] {
    (void)row.repeat(std::numeric_limits<std::int64_t>::max());
  });

  show("empty_like_scalar", pluss::timeline::date("2024-01-01").empty_like());
  const auto cube = pluss::timeline::date({
      {{"2024-01-01", "2024-01-02"}},
      {{"2024-01-03", "2024-01-04"}},
  });
  show("empty_like_cube", cube.empty_like());

  const auto like = jan12.tz_convert("America/Edmonton");
  show("fill_like_fields", like.fill_like(2035, 7, 8));
  show("fill_like_text", like.fill_like("2036-03-04"));
  show("fill_like_slash_text", like.fill_like("2036/03/04"));
  show("fill_like_scalar", pluss::timeline::date("2024-01-01").fill_like(2040, 2, 29));
  show("fill_like_empty", cube.empty_like().fill_like(2035, 7, 8));
  show_rejection("fill_like_invalid_date", [&] { (void)like.fill_like(2023, 2, 29); });
  show_rejection("fill_like_datetime_text", [&] {
    (void)like.fill_like("2036-03-04T12:00:00");
  });
}
