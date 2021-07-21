/*******************************************************************************
FILE : render_to_finite_elements.c

LAST MODIFIED : 5 January 1998

DESCRIPTION :
Renders gtObjects to VRML file
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (RENDER_TO_FINITE_ELEMENTS_H)
#define RENDER_TO_FINITE_ELEMENTS_H

#include "opencmiss/zinc/node.h"
#include "general/enumerator.h"

struct cmzn_scenefilter;
/*
Global types
------------
*/

enum Render_to_finite_elements_mode
/******************************************************************************
LAST MODIFIED : 7 December 2005

DESCRIPTION :
Renders the visible objects as finite elements into the specified <fe_region>.
==============================================================================*/
{
	/* Renders the scene as square linear product elements.  A collapsed linear
		product element will be used to represent a triangle. */
	RENDER_TO_FINITE_ELEMENTS_LINEAR_PRODUCT,
	RENDER_TO_FINITE_ELEMENTS_SURFACE_NODE_CLOUD,
	RENDER_TO_FINITE_ELEMENTS_NODES
};

PROTOTYPE_ENUMERATOR_FUNCTIONS(Render_to_finite_elements_mode)

/*
Global functions
----------------
*/

/**
 * Renders the visible objects as finite elements into the specified <fe_region>.
 * @param region  Region to put new elements in.
 * @param group  Optional group to put new elements in.
 * @param nodeset  For SURFACE_POINT_CLOUD render_mode only, the nodeset to put
 * points in.
 */
int render_to_finite_elements(cmzn_region_id source_region,
	const char *graphics_name, cmzn_scenefilter *filter,
	enum Render_to_finite_elements_mode render_mode,
	cmzn_region_id region, cmzn_field_group_id group,
	cmzn_field_id coordinate_field, cmzn_nodeset_id nodeset,
	FE_value line_density, FE_value line_density_scale_factor,
	FE_value surface_density, FE_value surface_density_scale_factor);

#endif /* !defined (RENDER_TO_FINITE_ELEMENTS_H) */
