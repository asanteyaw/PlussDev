#include "SeriesBody.h"
#include <PlussQit/core/SeriesBase.h>

// Broadcasting utilities for working with SeriesBase
namespace pluss {
namespace internal {
PLUSS_API SeriesBase expand_slow_path(const SeriesBase& self, IntArrayRef size);
} // namespace internal

inline MSharp::MaybeOwned<SeriesBase> expand_size(
    const SeriesBase& self,
    IntArrayRef size) {
  if (size.equals(self.sizes())) {
    return MSharp::MaybeOwned<SeriesBase>::borrowed(self);
  }
  return MSharp::MaybeOwned<SeriesBase>::owned(
      pluss::internal::expand_slow_path(self, size));
}
MSharp::MaybeOwned<SeriesBase> expand_size(SeriesBase&& self, IntArrayRef size) =
    delete;

inline MSharp::MaybeOwned<SeriesBase> expand_inplace(
    const SeriesBase& series,
    const SeriesBase& to_expand) {
  return expand_size(to_expand, series.sizes());
}
MSharp::MaybeOwned<SeriesBase> expand_inplace(
    const SeriesBase& series,
    SeriesBase&& to_expand) = delete;

} // namespace pluss
