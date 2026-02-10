#pragma once

#include <MSharp/macros/Export.h>

namespace MSharp {
struct TensorImpl;
}

namespace pluss {
class TensorBase;

// MemOverlap: Whether or not there is memory overlap
//
// No: Absolutely no memory overlap
// Yes: Absolutely yes memory overlap
// TooHard: There might be memory overlap, but it was too expensive to compute.
//
// NB: Please update the python test for these if you renumber them.
enum class MemOverlap { No, Yes, TooHard };

enum class MemOverlapStatus { Full, Partial, No, TooHard };

TORCH_API MemOverlap has_internal_overlap(const TensorBase& t);
TORCH_API MemOverlap has_internal_overlap(MSharp::TensorImpl* t);

TORCH_API void assert_no_internal_overlap(const TensorBase& t);
TORCH_API void assert_no_internal_overlap(MSharp::TensorImpl* t);

TORCH_API MemOverlapStatus
get_overlap_status(const TensorBase& a, const TensorBase& b);
TORCH_API MemOverlapStatus
get_overlap_status(const MSharp::TensorImpl* a, const MSharp::TensorImpl* b);

TORCH_API void assert_no_partial_overlap(
    const TensorBase& a,
    const TensorBase& b);
void assert_no_partial_overlap(MSharp::TensorImpl* a, MSharp::TensorImpl* b);

TORCH_API void assert_no_overlap(const TensorBase& a, const TensorBase& b);
TORCH_API void assert_no_overlap(MSharp::TensorImpl* a, MSharp::TensorImpl* b);

} // namespace pluss
