cmake_minimum_required(VERSION 3.28)
project(PlussPl_examples NONE)

function(plusspl_add_example target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE PlussPl)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_definitions(${target} PRIVATE PLUSSPL_EXAMPLES_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
endfunction()

plusspl_add_example(plusspl_2d_gallery_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/field_gallery.cpp)
plusspl_add_example(plusspl_2d_discrete_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/discrete_showcase.cpp)
plusspl_add_example(plusspl_2d_fieldmap_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/fieldmap_demo.cpp)
plusspl_add_example(plusspl_2d_all_families_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/all_2d_families_demo.cpp)
plusspl_add_example(plusspl_2d_style_gallery_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/style_gallery_demo.cpp)
plusspl_add_example(plusspl_2d_flash_style_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/flash_style_demo.cpp)
plusspl_add_example(plusspl_2d_flash_subsub_plot_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/flash_subsub_plot_demo.cpp)
plusspl_add_example(plusspl_2d_returns_volatility_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/returns_volatility_demo.cpp)
plusspl_add_example(plusspl_2d_ngarch_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/ngarch_demo.cpp)
plusspl_add_example(plusspl_3d_flash_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/flash_3d_demo.cpp)
plusspl_add_example(plusspl_3d_animation_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/flash_3d_animation_demo.cpp)
plusspl_add_example(plusspl_4d_geometry_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/4D/flash_4d_geometry_demo.cpp)
plusspl_add_example(plusspl_4d_animation_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/4D/flash_4d_animation_demo.cpp)
plusspl_add_example(plusspl_3d_gallery_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/spatial_gallery.cpp)
plusspl_add_example(plusspl_3d_option_surface_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/option_surface_demo.cpp)
plusspl_add_example(plusspl_3d_wave_mesh_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/wave_mesh_demo.cpp)
plusspl_add_example(plusspl_3d_terrain_skin_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/terrain_skin_demo.cpp)
plusspl_add_example(plusspl_3d_cloud_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/cloud_field_demo.cpp)
plusspl_add_example(plusspl_3d_spire_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/spire_matrix_demo.cpp)
plusspl_add_example(plusspl_3d_all_families_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/all_3d_families_demo.cpp)
plusspl_add_example(plusspl_3d_mpl3d_equivalence_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/3D/mpl3d_equivalence_demo.cpp)
plusspl_add_example(plusspl_flash_api_capture_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/flash_api_capture_demo.cpp)
target_link_libraries(plusspl_flash_api_capture_demo PRIVATE PlussMic)

# Backward-compatible build target names for previously used flash demo invocations.
add_custom_target(plusspl_3d_flash_3d_demo DEPENDS plusspl_3d_flash_demo)
add_custom_target(plusspl_3d_flash_3d_animation_demo DEPENDS plusspl_3d_animation_demo)
add_custom_target(plusspl_4d_flash_4d_geometry_demo DEPENDS plusspl_4d_geometry_demo)
add_custom_target(plusspl_4d_flash_4d_animation_demo DEPENDS plusspl_4d_animation_demo)

add_custom_target(plusspl_gallery_outputs
    COMMAND plusspl_2d_gallery_demo
    COMMAND plusspl_2d_discrete_demo
    COMMAND plusspl_2d_fieldmap_demo
    COMMAND plusspl_2d_all_families_demo
    COMMAND plusspl_2d_style_gallery_demo
    COMMAND plusspl_2d_flash_style_demo
    COMMAND plusspl_2d_flash_subsub_plot_demo
    COMMAND plusspl_2d_returns_volatility_demo
    COMMAND plusspl_2d_ngarch_demo
    COMMAND plusspl_3d_flash_demo
    COMMAND plusspl_4d_geometry_demo
    COMMAND plusspl_4d_animation_demo
    COMMAND plusspl_3d_gallery_demo
    COMMAND plusspl_3d_option_surface_demo
    COMMAND plusspl_3d_wave_mesh_demo
    COMMAND plusspl_3d_terrain_skin_demo
    COMMAND plusspl_3d_cloud_demo
    COMMAND plusspl_3d_spire_demo
    COMMAND plusspl_3d_all_families_demo
    COMMAND plusspl_3d_mpl3d_equivalence_demo
    DEPENDS
        plusspl_2d_gallery_demo
        plusspl_2d_discrete_demo
        plusspl_2d_fieldmap_demo
        plusspl_2d_all_families_demo
        plusspl_2d_style_gallery_demo
        plusspl_2d_flash_style_demo
        plusspl_2d_flash_subsub_plot_demo
        plusspl_2d_returns_volatility_demo
        plusspl_2d_ngarch_demo
        plusspl_3d_flash_demo
        plusspl_4d_geometry_demo
        plusspl_4d_animation_demo
        plusspl_3d_gallery_demo
        plusspl_3d_option_surface_demo
        plusspl_3d_wave_mesh_demo
        plusspl_3d_terrain_skin_demo
        plusspl_3d_cloud_demo
        plusspl_3d_spire_demo
        plusspl_3d_all_families_demo
        plusspl_3d_mpl3d_equivalence_demo
        plusspl_flash_api_capture_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Generating PlussPl demo outputs"
)
