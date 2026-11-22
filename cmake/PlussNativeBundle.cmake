# Bundle static native libraries (libzip, Arrow, Parquet and everything they
# link) into the installed distribution, so a downloaded archive needs no
# package installed on the consumer's machine.
#
#   pluss_native_bundle(<name> TARGETS <imported targets...>)
#
# walks the link closure of the given imported targets, installs every static
# library it finds under <libdir>/pluss-native, and installs a generated
# PlussNativeBundle.cmake that defines Pluss::<name> as an INTERFACE target
# linking those copies (plus the system libraries and frameworks they need) by
# a path relative to the package, so the archive stays relocatable.
#
# Shared libraries in the closure are an error: they cannot be bundled.

include_guard(GLOBAL)

# Reduce one INTERFACE_LINK_LIBRARIES entry to a plain token, or "" to drop it.
function(_pluss_native_clean_entry ENTRY OUT)
  set(item "${ENTRY}")
  if(item MATCHES "^\\$<INSTALL_INTERFACE:")
    set(${OUT} "" PARENT_SCOPE)
    return()
  endif()
  if(item MATCHES "^\\$<\\$<CONFIG:[Dd][Ee][Bb][Uu][Gg]>:")
    set(${OUT} "" PARENT_SCOPE)
    return()
  endif()
  # Unwrap the wrappers that only carry usage-requirement intent.
  if(item MATCHES "^\\$<(LINK_ONLY|BUILD_INTERFACE):(.*)>$")
    set(item "${CMAKE_MATCH_2}")
  endif()
  if(item MATCHES "\\$<")
    message(FATAL_ERROR
      "pluss_native_bundle: cannot interpret link entry '${ENTRY}'. "
      "Extend _pluss_native_clean_entry in cmake/PlussNativeBundle.cmake.")
  endif()
  set(${OUT} "${item}" PARENT_SCOPE)
endfunction()

# Add TARGET_NAME's closure to the caller's lists. Libraries are appended after
# everything they link (post-order), so reversing the list puts every library
# before the libraries it needs, as static linking requires.
function(_pluss_native_walk TARGET_NAME)
  if("${TARGET_NAME}" IN_LIST _pluss_native_seen)
    return()
  endif()
  list(APPEND _pluss_native_seen "${TARGET_NAME}")
  get_target_property(type ${TARGET_NAME} TYPE)
  if(type STREQUAL "SHARED_LIBRARY" OR type STREQUAL "MODULE_LIBRARY")
    message(FATAL_ERROR
      "pluss_native_bundle: ${TARGET_NAME} is a shared library; the distribution "
      "needs static libraries so it works without installing anything. Use a "
      "static dependency toolchain (for example a vcpkg static triplet).")
  endif()
  set(deps "")
  foreach(prop IN ITEMS INTERFACE_LINK_LIBRARIES INTERFACE_LINK_LIBRARIES_RELEASE
                        INTERFACE_LINK_LIBRARIES_NOCONFIG)
    get_target_property(value ${TARGET_NAME} ${prop})
    if(value)
      list(APPEND deps ${value})
    endif()
  endforeach()
  foreach(dep IN LISTS deps)
    _pluss_native_clean_entry("${dep}" entry)
    if(NOT entry)
      continue()
    endif()
    if(TARGET "${entry}")
      if(entry STREQUAL "Threads::Threads")
        list(APPEND _pluss_native_system "Threads::Threads")
      else()
        _pluss_native_walk("${entry}")
      endif()
    elseif(IS_ABSOLUTE "${entry}")
      if(entry MATCHES "\\.(a|lib)$")
        list(APPEND _pluss_native_files "${entry}")
      else()
        # A shared or stub library from the build machine's SDK: record only
        # its linker name, never its path.
        get_filename_component(name "${entry}" NAME)
        string(REGEX REPLACE "^lib" "" name "${name}")
        string(REGEX REPLACE "(\\.[0-9]+)*\\.(dylib|so|tbd)(\\.[0-9.]+)?$" "" name "${name}")
        list(APPEND _pluss_native_system "-l${name}")
      endif()
    else()
      list(APPEND _pluss_native_system "${entry}")
    endif()
  endforeach()
  if(type STREQUAL "STATIC_LIBRARY" OR type STREQUAL "UNKNOWN_LIBRARY")
    set(file "")
    foreach(prop IN ITEMS IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION
                          IMPORTED_LOCATION_NOCONFIG IMPORTED_LOCATION_RELWITHDEBINFO
                          IMPORTED_LOCATION_MINSIZEREL)
      if(NOT file)
        get_target_property(value ${TARGET_NAME} ${prop})
        if(value AND EXISTS "${value}")
          set(file "${value}")
        endif()
      endif()
    endforeach()
    if(NOT file)
      message(FATAL_ERROR "pluss_native_bundle: no library file found for ${TARGET_NAME}")
    endif()
    list(APPEND _pluss_native_files "${file}")
  endif()
  set(_pluss_native_seen "${_pluss_native_seen}" PARENT_SCOPE)
  set(_pluss_native_files "${_pluss_native_files}" PARENT_SCOPE)
  set(_pluss_native_system "${_pluss_native_system}" PARENT_SCOPE)
endfunction()

function(pluss_native_bundle NAME)
  cmake_parse_arguments(ARG "" "" "TARGETS" ${ARGN})
  set(_pluss_native_seen "")
  set(_pluss_native_files "")
  set(_pluss_native_system "")
  foreach(target IN LISTS ARG_TARGETS)
    _pluss_native_walk("${target}")
  endforeach()

  # Post-order reversed: dependents first, the libraries they need after them.
  list(REVERSE _pluss_native_files)
  list(REMOVE_DUPLICATES _pluss_native_files)

  # "-framework" and its argument travel as one token; drop duplicates.
  set(system_tokens "")
  set(pending_framework FALSE)
  foreach(token IN LISTS _pluss_native_system)
    if(pending_framework)
      set(token "-framework ${token}")
      set(pending_framework FALSE)
    elseif(token STREQUAL "-framework")
      set(pending_framework TRUE)
      continue()
    endif()
    list(APPEND system_tokens "${token}")
  endforeach()
  list(REMOVE_DUPLICATES system_tokens)

  set(_bundle_dir "${CMAKE_INSTALL_LIBDIR}/pluss-native")
  set(file_names "")
  foreach(file IN LISTS _pluss_native_files)
    get_filename_component(file_name "${file}" NAME)
    list(APPEND file_names "${file_name}")
  endforeach()
  list(LENGTH file_names unique_count)
  list(REMOVE_DUPLICATES file_names)
  list(LENGTH file_names distinct_count)
  if(NOT unique_count EQUAL distinct_count)
    message(FATAL_ERROR "pluss_native_bundle: two bundled libraries share a file name")
  endif()
  install(FILES ${_pluss_native_files} DESTINATION "${_bundle_dir}")

  set(content "# Generated by cmake/PlussNativeBundle.cmake. Relocatable: all paths are\n")
  string(APPEND content "# relative to this file.\n")
  string(APPEND content "get_filename_component(_pluss_native_dir \"\${CMAKE_CURRENT_LIST_DIR}/../../pluss-native\" ABSOLUTE)\n")
  string(APPEND content "set(_pluss_native_libs \"\")\n")
  foreach(file_name IN LISTS file_names)
    string(APPEND content "list(APPEND _pluss_native_libs \"\${_pluss_native_dir}/${file_name}\")\n")
  endforeach()
  foreach(token IN LISTS system_tokens)
    if(token STREQUAL "Threads::Threads")
      string(APPEND content "find_package(Threads REQUIRED)\n")
    endif()
    string(APPEND content "list(APPEND _pluss_native_libs [==[${token}]==])\n")
  endforeach()
  string(APPEND content "if(NOT TARGET Pluss::${NAME})\n")
  string(APPEND content "  add_library(Pluss::${NAME} INTERFACE IMPORTED)\n")
  string(APPEND content "  set_target_properties(Pluss::${NAME} PROPERTIES INTERFACE_LINK_LIBRARIES \"\${_pluss_native_libs}\")\n")
  string(APPEND content "endif()\n")
  file(WRITE "${CMAKE_BINARY_DIR}/PlussNativeBundle.cmake" "${content}")
  install(FILES "${CMAKE_BINARY_DIR}/PlussNativeBundle.cmake"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Pluss")
  message(STATUS "Bundled native libraries: ${file_names}")
  message(STATUS "Bundled native system links: ${system_tokens}")
endfunction()
