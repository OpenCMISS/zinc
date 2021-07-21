/*******************************************************************************
FILE : computed_field_wrappers.h

LAST MODIFIED : 18 October 2000

DESCRIPTION :
Functions for converting fields in a not-so-usable state into more useful
quantities, usually for graphical display or editing. For example, making a
wrapper rectangular Cartesian field out of a prolate coordinate field, making
fibre_axes out of a fibre field.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_WRAPPERS_H)
#define COMPUTED_FIELD_WRAPPERS_H

struct cmzn_field;

/**
* Returns a RECTANGULAR_CARTESIAN coordinate field that may be the original
* <coordinate field> if it is already in this coordinate system, or a
* coordinate transformation field wrapper for it if it is not.
* Notes:
* Used to ensure RC coordinate fields are passed to graphics functions.
* The NORMALISED_WINDOW_COORDINATES system is a rectangular cartesian system
* but indicates that the graphics objects produced should be displayed in the
* window coordinates rather than the model 3D coordinates and so are also not
* wrapped.
* @return  Accessed field. Up to caller to destroy reference.
*/
cmzn_field *cmzn_field_get_coordinate_field_wrapper(
	cmzn_field *coordinate_field);

/**
 * @return true if vector field requires wrapping to convert to RC, otherwise false.
 */
bool cmzn_field_vector_needs_wrapping(cmzn_field *vector_field);

/**
 * Takes the vector and coordinate fields and returns an appropriate rectangular
 * Cartesian vector field e.g. for use in graphics/conversions.
 * Will be the same as the original field if both are RC or other non-convertable types.
 * This involves making a fibre axes field wrapper if the field has 3 or fewer
 * components and a FIBRE coordinate system (this requires the coordinate_field too).
 * If the field has 3 or fewer components and a non-RECTANGULAR_CARTESIAN coordinate
 * system, a vector coordinate transformation field wrapper will be made for it.
 * NOTE:
 * @param coordinate_field  The coordinate field at which the vectors are located.
 * Note: must be RC i.e. already wrapped if non RC.
 * @return  Accessed field. Up to caller to destroy reference.
 */
cmzn_field *cmzn_field_get_vector_field_wrapper(
	cmzn_field *vector_field, cmzn_field *coordinate_field);

#endif /* !defined (COMPUTED_FIELD_WRAPPERS_H) */
