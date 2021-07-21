
# Defines IMAGE_PROCESSING_SRCS

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( IMAGE_PROCESSING_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_image_resample.cpp )
SET( IMAGE_PROCESSING_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_image_resample.h )

IF( ZINC_USE_ITK )
	SET( IMAGE_PROCESSING_SRCS ${IMAGE_PROCESSING_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_threshold_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_binary_threshold_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_canny_edge_detection_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_mean_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_sigmoid_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_discrete_gaussian_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_curvature_anisotropic_diffusion_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_derivative_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_rescale_intensity_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_connected_threshold_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_gradient_magnitude_recursive_gaussian_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_fast_marching_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_binary_dilate_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_binary_erode_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_histogram_image_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_image_filter.cpp )
	SET( IMAGE_PROCESSING_HDRS ${IMAGE_PROCESSING_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_binary_dilate_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_binary_erode_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_binary_threshold_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_canny_edge_detection_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_connected_threshold_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_curvature_anisotropic_diffusion_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_derivative_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_discrete_gaussian_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_fast_marching_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_gradient_magnitude_recursive_gaussian_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_histogram_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_mean_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_rescale_intensity_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_sigmoid_image_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image_processing/computed_field_threshold_image_filter.h )
ENDIF( ZINC_USE_ITK )
