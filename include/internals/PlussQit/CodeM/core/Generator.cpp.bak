#include <PlussQit/core/Generator.h>
#include <PlussQit/core/Series.h>
#include <MSharp/util/Exception.h>

namespace pluss {

void Generator::set_state(const pluss::Series& new_state) {
  TORCH_CHECK(new_state.defined(), "Undefined series is not allowed");
  this->impl_->set_state(*new_state.unsafeGetTensorImpl());
}

pluss::Series Generator::get_state() const {
  return pluss::Series::wrap_tensor_impl(this->impl_->get_state());
}

void Generator::graphsafe_set_state(const Generator& new_state) {
  this->impl_->graphsafe_set_state(new_state.getIntrusivePtr());
}

Generator Generator::graphsafe_get_state() const {
  return Generator(this->impl_->graphsafe_get_state());
}

} // namespace pluss
