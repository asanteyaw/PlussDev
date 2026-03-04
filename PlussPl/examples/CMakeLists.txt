cmake_minimum_required(VERSION 3.28)
project(PlussPl_examples NONE)

function(plusspl_add_example target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE PlussPl)
    target_compile_features(${target} PRIVATE cxx_std_20)
endfunction()

plusspl_add_example(plusspl_2d_gallery_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/field_gallery.cpp)
plusspl_add_example(plusspl_2d_discrete_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/discrete_showcase.cpp)
plusspl_add_example(plusspl_2d_fieldmap_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/2D/fieldmap_demo.cpp)
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

add_custom_target(plusspl_gallery_outputs
    COMMAND plusspl_2d_gallery_demo
    COMMAND plusspl_2d_discrete_demo
    COMMAND plusspl_2d_fieldmap_demo
    COMMAND plusspl_3d_gallery_demo
    COMMAND plusspl_3d_option_surface_demo
    COMMAND plusspl_3d_wave_mesh_demo
    COMMAND plusspl_3d_terrain_skin_demo
    COMMAND plusspl_3d_cloud_demo
    COMMAND plusspl_3d_spire_demo
    DEPENDS
        plusspl_2d_gallery_demo
        plusspl_2d_discrete_demo
        plusspl_2d_fieldmap_demo
        plusspl_3d_gallery_demo
        plusspl_3d_option_surface_demo
        plusspl_3d_wave_mesh_demo
        plusspl_3d_terrain_skin_demo
        plusspl_3d_cloud_demo
        plusspl_3d_spire_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Generating PlussPl demo outputs"
)
