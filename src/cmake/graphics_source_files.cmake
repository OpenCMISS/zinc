
# Defines GRAPHICS_SRCS

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( GRAPHICS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/graphics_json_export.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/graphics_json_import.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/graphics_json_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/scene_json_export.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/scene_json_import.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/sceneviewer_json_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/spectrum_json_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/tessellation_json_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/auxiliary_graphics_types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/scene_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/colour.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/complex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/element_point_ranges.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/environment_map.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph_axes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph_circular.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph_colour_bar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_vertex_array.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/import_graphics_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/iso_field_calculation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/laguer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/material.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/mcubes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/order_independent_transparency.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_to_finite_elements.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_stl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_vrml.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_wavefront.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_triangularisation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/quaternion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_coordinate_system.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_viewer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/selection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_program.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_uniforms.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/spectrum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/spectrum_component.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/tessellation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_line.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/threejs_export.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/triangle_mesh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/volume_texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/webgl_export.cpp )
SET( GRAPHICS_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/graphics_json_export.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/graphics_json_import.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/graphics_json_io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/scene_json_export.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/scene_json_import.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/sceneviewer_json_io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/spectrum_json_io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/tessellation_json_io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/auxiliary_graphics_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_module.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/scene_stream.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/colour.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/complex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/element_point_ranges.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/environment_map.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph_axes.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph_circular.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glyph_colour_bar.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_object.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_object.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_object_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_object_highlight.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_vertex_array.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/image.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/import_graphics_object.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/iso_field_calculation.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/laguer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/material.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/material.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/mcubes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/order_independent_transparency.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/quaternion.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_alias.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_binary_wavefront.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_stl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_vrml.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_wavefront.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_to_finite_elements.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_triangularisation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_coordinate_system.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scenefilter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_viewer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/selection.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_program.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_uniforms.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/spectrum.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/spectrum.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/spectrum_component.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/tessellation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_line.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/threejs_export.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/triangle_mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/volume_texture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/webgl_export.hpp )

IF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
	SET( GRAPHICS_SRCS ${GRAPHICS_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_library.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/light.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_gl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scenefilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_picker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/tile_graphics_objects.cpp )
	SET( GRAPHICS_HDRS ${GRAPHICS_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/font.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/graphics_library.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/light.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_gl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_picker.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/tile_graphics_objects.h )
ENDIF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
