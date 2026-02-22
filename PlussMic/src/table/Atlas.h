#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"
namespace pluss::kindle {
class Archive;
struct KindelizePolicy;
}

namespace pluss::table {

namespace detail {
class AtlasImpl;
}

class Atlas {
public:
  Atlas();
  explicit Atlas(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels = std::nullopt);
  Atlas(const Atlas& other);
  Atlas(Atlas&& other) noexcept;
  Atlas& operator=(const Atlas& other);
  Atlas& operator=(Atlas&& other) noexcept;
  ~Atlas();

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t depth() const;
  std::int64_t nrows() const;
  std::int64_t ncols() const;

  const std::vector<std::string>& column_names() const;
  const std::optional<Thread>& axis0_labels() const;

  DataFrame frame(std::int64_t index) const;
  DataFrame iloc(std::int64_t index) const;
  DataFramePlotAccessor plot(std::int64_t frame_index = 0) const;

  DataFrame loc(std::string_view label) const;
  DataFrame loc(std::int64_t label) const;
  DataFrame loc(const pluss::Timeline& label) const;

  Atlas head(std::int64_t n = 5) const;
  Atlas tail(std::int64_t n = 5) const;
  Atlas slice_frames(std::int64_t start, std::int64_t stop) const;
  Atlas sort_depth(bool ascending = true, bool nulls_last = true) const;
  Atlas relabel_depth(const Thread& labels) const;
  Atlas align_depth(const Thread& labels) const;

  Atlas map_frames(const std::function<DataFrame(const DataFrame&)>& fn) const;
  DataFrame aggregate_over_frames(const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) const;
  pluss::kindle::Archive kindelize() const;
  pluss::kindle::Archive kindelize(const pluss::kindle::KindelizePolicy& policy) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

private:
  explicit Atlas(pluss::msharp::intrusive_ptr<detail::AtlasImpl> impl);

  const detail::AtlasImpl& impl_ref_() const;
  pluss::msharp::intrusive_ptr<detail::AtlasImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Atlas& atlas);

} // namespace pluss::table
