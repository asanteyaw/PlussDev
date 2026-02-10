#pragma once

#include <PlussQit/core/List.h>
#include <PlussQit/core/Series.h>

namespace pluss {
class Series;
class OptionalTensorRef;
}


namespace MSharp::detail {

/*
 * Specializations of `IListRefTagImplBase` that implement the default
 * implementation for `IListRefTag::Unboxed`.
 */
template <typename T, typename ListElemT>
class IListRefTagImplBase<IListRefTag::Unboxed, T, ListElemT> {
 public:
  using elem_type = ListElemT;
  using list_type = ArrayRef<elem_type>;

  /*
   * These `unwrap` static methods unwraps the inner containers out
   * of `IListRef<T>` (and `IListRefIterator<T>`). They are required when
   * the macro `TORCH_ILISTREF_UNWRAP` is called.
   */
  static const list_type& unwrap(const IListRef<T>& ilist) {
    return ilist.payload_.unboxed;
  }

  static typename list_type::const_iterator& unwrap(IListRefIterator<T>& it) {
    return it.payload_.unboxed_iterator;
  }

  static const typename list_type::const_iterator& unwrap(
      const IListRefIterator<T>& it) {
    return it.payload_.unboxed_iterator;
  }

  /*
   * We have these function (besides the `unwrap`s above) because the
   * implementation for both `IListRef::operator[]` and `IListRefIterator::operator*`
   * weren't syntatically equal for the existing tags at the time
   * (`Unboxed` and `Boxed`).
   */
  static IListRefConstRef<T> front(const list_type& lst) {
    return lst.front();
  }

  static IListRefConstRef<T> iterator_get(
      const typename list_type::const_iterator& it) {
    return *it;
  }
};

/*
 * Specializations of `IListRefTagImplBase` that implement the default
 * implementation for `IListRefTag::Boxed`.
 */
template <typename T, typename ListElemT>
class IListRefTagImplBase<IListRefTag::Boxed, T, ListElemT> {
 public:
  using elem_type = ListElemT;
  using list_type = List<elem_type>;

  static const list_type& unwrap(const IListRef<T>& ilist) {
    return *ilist.payload_.boxed;
  }

  static typename list_type::const_iterator& unwrap(IListRefIterator<T>& it) {
    return it.payload_.boxed_iterator;
  }

  static const typename list_type::const_iterator& unwrap(
      const IListRefIterator<T>& it) {
    return it.payload_.boxed_iterator;
  }

  static IListRefConstRef<T> front(const list_type& lst) {
    return lst[0];
  }

  static IListRefConstRef<T> iterator_get(
      const typename list_type::const_iterator& it) {
    return (*it).get().toTensor();
  }
};

/*
 * Specializations of `IListRefTagImplBase` that implement the default
 * implementation for `IListRefTag::Materialized`.
 */
template <typename T>
class IListRefTagImplBase<IListRefTag::Materialized, T, MaterializedIListRefElem<T>> {
 public:
  using elem_type = MaterializedIListRefElem<T>;
  using list_type = MaterializedIListRef<T>;

  static const list_type& unwrap(const IListRef<T>& ilist) {
    return *ilist.payload_.materialized;
  }

  static typename list_type::const_iterator& unwrap(IListRefIterator<T>& it) {
    return it.payload_.materialized_iterator;
  }

  static const typename list_type::const_iterator& unwrap(
      const IListRefIterator<T>& it) {
    return it.payload_.materialized_iterator;
  }

  static IListRefConstRef<T> front(const list_type& lst) {
    return lst[0];
  }

  static IListRefConstRef<T> iterator_get(
      const typename list_type::const_iterator& it) {
    return *it;
  }
};

/*
 * [Note: ITensorListRef]
 * Specializations necessary for `IListRef<pluss::Series>` type.
 *
 * Since the default implementations are usually done with supporting
 * `Series` in mind, we only have to inherit from the base implementations.
 */
template <>
class IListRefTagImpl<IListRefTag::Unboxed, pluss::Series>
    : public IListRefTagImplBase<IListRefTag::Unboxed, pluss::Series> {};

template <>
class IListRefTagImpl<IListRefTag::Boxed, pluss::Series>
    : public IListRefTagImplBase<IListRefTag::Boxed, pluss::Series> {};

template <>
class IListRefTagImpl<IListRefTag::Materialized, pluss::Series>
    : public IListRefTagImplBase<
          IListRefTag::Materialized,
          pluss::Series,
          MaterializedIListRefElem<pluss::Series>> {};

/*
 * [Note: IOptTensorListRef]
 * Specializations necessary for `IListRef<pluss::OptionalTensorRef>` type.
 *
 * We can't get an `pluss::OptionalTensorRef` directly from an instance of
 * `List<optional<Series>>` (the type that corresponds to the boxed world).
 *
 * So, the default implementation won't help us. Thus, we have to implement
 * this method ourselves.
 */
template <>
class IListRefTagImpl<IListRefTag::Unboxed, pluss::OptionalTensorRef>
    : public IListRefTagImplBase<IListRefTag::Unboxed, pluss::OptionalTensorRef> {};

template <>
class IListRefTagImpl<IListRefTag::Boxed, pluss::OptionalTensorRef>
    : public IListRefTagImplBase<IListRefTag::Boxed, pluss::OptionalTensorRef, std::optional<pluss::Series>> {

 public:
  /*
   * Given an instance of the types corresponding to the `Boxed` tag, we override
   * the default implementation, so that we can return a `pluss::OptionalTensorRef`.
   */
  static IListRefConstRef<pluss::OptionalTensorRef> iterator_get(
      const typename list_type::const_iterator& it) {
    const auto& ivalue = (*it).get();
    if (!ivalue.isNone()) {
        const auto& series = ivalue.toTensor();
        return (series.defined()) ? series : pluss::OptionalTensorRef{};
    }
    return {};
  }
};

template <>
class IListRefTagImpl<IListRefTag::Materialized, pluss::OptionalTensorRef>
    : public IListRefTagImplBase<
          IListRefTag::Materialized,
          pluss::OptionalTensorRef,
          MaterializedIListRefElem<pluss::OptionalTensorRef>> {};

} // namespace MSharp::detail


namespace pluss {

// [Note: ITensorListRef]
using ITensorListRef = MSharp::IListRef<pluss::Series>;
using ITensorListRefIterator = MSharp::IListRefIterator<pluss::Series>;
using MaterializedITensorListRef = MSharp::detail::MaterializedIListRef<pluss::Series>;
// [Note: IOptTensorListRef]
using IOptTensorListRef = MSharp::IListRef<pluss::OptionalTensorRef>;
using IOptTensorListRefIterator = MSharp::IListRefIterator<pluss::OptionalTensorRef>;
using MaterializedIOptTensorListRef = MSharp::detail::MaterializedIListRef<pluss::OptionalTensorRef>;

} // namespace pluss
