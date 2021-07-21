/**
 * @file elementfieldtemplateid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENTFIELDTEMPLATEID_H__
#define CMZN_ELEMENTFIELDTEMPLATEID_H__

/**
 * @brief  A template defining field parameter mapping and interpolation over
 * an element chart.
 *
 * A template defining parameter mapping and interpolation for a scalar
 * quantity over an element chart, to apply to field components across elements
 * of a mesh. Consists of an element basis, plus how to evaluate the parameter
 * multiplying each function in the basis. The element parameter for each basis
 * function is defined as a sum of zero or more terms, with each term
 * multiplied by zero or more scale factors given by their local indexes.
 * Depending on the parameter mapping mode, the terms map node parameters,
 * element parameters or spatially constant field parameters. For node
 * parameters each term extracts a node value/derivative version for a local
 * node. When used on the mesh, a local-to-global node map for this template is
 * applied to give different parameters for each element field component, and
 * element scale factors are similarly indexed for each element.
 */
struct cmzn_elementfieldtemplate;
typedef struct cmzn_elementfieldtemplate *cmzn_elementfieldtemplate_id;

/**
 * Modes for how element parameters are mapped from global DOFs.
 */
enum cmzn_elementfieldtemplate_parameter_mapping_mode
{
	CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID = 0,
		/*!< Invalid mapping mode */
	CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT = 1,
		/*!< Element parameters are mapped directly by element */
	CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD = 2,
		/*!< Constant value for field component */
	CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE = 3
		/*!< Element parameters are mapped from nodes via local-to-global node map */
};

/**
 * Unique types for element scale factors which together with node/element and
 * scale factor identifier allow merging of common scale factors in
 * neighbouring elements, or for different templates in the same element.
 * Note that types with the PATCH suffix are intended to be used for rescaling,
 * e.g. scaling arc-length derivatives to be with respect to element chart.
 * Note that a NODE type scale factor must scale parameters from exactly one
 * local node.
 * Note that ELEMENT types are currently limited to using scale factor
 * identifier 0, meaning unique to a given element and element field template.
 */
enum cmzn_elementfieldtemplate_scale_factor_type
{
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID = 0,
		/*!< Invalid type */
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL = 1,
		/*!< General linear map coefficients private to an element */
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH = 2,
		/*!< Patch to local element coordinate scaling private to an element */
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL = 3,
		/*!< General linear map coefficients shared across multiple points in the mesh */
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH = 4,
		/*!< Patch to local element coordinate scaling shared across multiple points in the mesh */
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL = 5,
		/*!< General linear map coefficients shared through global nodes */
	CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH = 6
		/*!< Patch to local element coordinate scaling shared through global nodes */
};

#endif
