// search.cpp -- placeholder, intentionally empty.
//
// Search/search::Result (LYMPH_REDESIGN_PLAN.md phase 5) landed fully inline
// in search.h instead of here -- matching the convention every other
// substantial header this session actually used (stage.h, balance.h,
// columnrouter.h, selection.h, builder.h are all header-only too, despite
// being non-trivial), rather than this file's earlier, different plan to
// hold the (trial x fold) dispatch body. Left as a harmless empty
// translation unit (picked up, and doing nothing, by Lymph/CMakeLists.txt's
// CONFIGURE_DEPENDS glob) rather than deleted, in case something search.h-
// adjacent later grows large enough to actually want a .cpp home.
