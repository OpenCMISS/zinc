/*******************************************************************************
FILE : geometry.h

LAST MODIFIED : 17 January 2000

DESCRIPTION :
The types and global variables for describing geometry.  Prototypes of functions
for performing coordinate transformations.
???DB.  What about PI ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GEOMETRY_H)
#define GEOMETRY_H

#include "general/value.h"
#include "general/enumerator.h"
#include "general/object.h"


#if !defined (PI)
#define PI 3.14159265358979323846
#endif

/*
Global types
------------
*/
enum Coordinate_system_type
/*******************************************************************************
LAST MODIFIED : 24 August 1999

DESCRIPTION :
The possible coordinate systems.  Values specified to correspond to CMISS
==============================================================================*/
{
	CYLINDRICAL_POLAR=2,
	/*ELEMENT_XI=7,*/
	FIBRE=6,
	NOT_APPLICABLE=8, /* a field with no coordinate system*/
	OBLATE_SPHEROIDAL=5,
	PROLATE_SPHEROIDAL=4,
	RECTANGULAR_CARTESIAN=1,
	SPHERICAL_POLAR=3,
	/*TIME_SERIES=7,*/
	/* NORMALISED_WINDOW_COORDINATES is a rectangular coordinate system
		but indicates that it should be fixed relative to the window viewport 
		rather than in model cordinate space.  The convention is that it goes
		from -1 to 1 in x and y screen space and 0 to 1 in depth. */
	NORMALISED_WINDOW_COORDINATES=9,
	UNKNOWN_COORDINATE_SYSTEM=0
}; /* enum Coordinate_system_type */

struct Coordinate_system
/*******************************************************************************
LAST MODIFIED : 24 August 1999

DESCRIPTION :
==============================================================================*/
{
	enum Coordinate_system_type type;
	union
	{
		FE_value focus;
	} parameters;

	Coordinate_system(enum Coordinate_system_type typeIn = RECTANGULAR_CARTESIAN, FE_value focusIn = 1.0) :
		type(typeIn)
	{
		this->parameters.focus = focusIn;
	}

}; /* struct Coordinate_system */

enum Projection_type
/*******************************************************************************
LAST MODIFIED : 31 May 2000

DESCRIPTION :
???DB.  There will be different types of CYLINDRICAL, depending on where the
cut is and if the projection should overlap (3/2 times round).
==============================================================================*/
{
	CYLINDRICAL_PROJECTION,
	HAMMER_PROJECTION,
	POLAR_PROJECTION,
	THREED_PROJECTION
}; /* enum Projection_type */

typedef struct
/*******************************************************************************
LAST MODIFIED : 14 July 1995

DESCRIPTION :
Represents a linear transformation  translate + T*x .
==============================================================================*/
{
	/* translate */
	FE_value translate_x,translate_y,translate_z;
	/* transformation */
	FE_value txx,txy,txz,tyx,tyy,tyz,tzx,tzy,tzz;
} Linear_transformation;

/*
Global functions
----------------
*/
int linear_transformation(Linear_transformation *linear_transformation,FE_value x,
	FE_value y,FE_value z,FE_value *result_x,FE_value *result_y,FE_value *result_z);
/*******************************************************************************
LAST MODIFIED : 14 July 1995

DESCRIPTION :
Performs a <linear_transformation>.
==============================================================================*/

int cartesian_to_cylindrical_polar(FE_value x,FE_value y,FE_value z_in,FE_value *r,
	FE_value *theta,FE_value *z,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 21 August 1995

DESCRIPTION :
For transforming from cylindrical polar to cartesian coordinates.
x = r*cos(theta)
y = r*sin(theta)
z = z_in
==============================================================================*/

int cylindrical_polar_to_cartesian(FE_value r,FE_value theta,FE_value z_in,FE_value *x,
	FE_value *y,FE_value *z,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 21 August 1995

DESCRIPTION :
For transforming from cylindrical polar to cartesian coordinates.
x = r*cos(theta)
y = r*sin(theta)
z = z_in
==============================================================================*/

int cartesian_to_spherical_polar(FE_value x,FE_value y,FE_value z,FE_value *r,FE_value *theta,
	FE_value *phi,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 17 January 2000

DESCRIPTION :
For transforming from spherical polar to cartesian coordinates.
x = r*cos(phi)*cos(theta)
y = r*cos(phi)*sin(theta)
z = r*sin(phi)
==============================================================================*/

int spherical_polar_to_cartesian(FE_value r,FE_value theta,FE_value phi,FE_value *x,
	FE_value *y,FE_value *z,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 6 July 1994

DESCRIPTION :
For transforming from spherical polar to cartesian coordinates.
x = r*cos(phi)*cos(theta)
y = r*cos(phi)*sin(theta)
z = r*sin(phi)
==============================================================================*/

int cartesian_to_prolate_spheroidal(FE_value x,FE_value y,FE_value z,FE_value focus,
	FE_value *lambda,FE_value *mu,FE_value *theta,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 18 October 1995

DESCRIPTION :
For transforming from cartesian to prolate spheroidal coordinates.
x = focus*cosh(lambda)*cos(mu)
y = focus*sinh(lambda)*sin(mu)*cos(theta)
z = focus*sinh(lambda)*sin(mu)*sin(theta)
==============================================================================*/

int prolate_spheroidal_to_cartesian(FE_value lambda,FE_value mu,FE_value theta,
	FE_value focus,FE_value *x,FE_value *y,FE_value *z,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 17 October 1995

DESCRIPTION :
For transforming from prolate spheroidal to cartesian coordinates.
x = focus*cosh(lambda)*cos(mu)
y = focus*sinh(lambda)*sin(mu)*cos(theta)
z = focus*sinh(lambda)*sin(mu)*sin(theta)
==============================================================================*/

int Hammer_projection(FE_value mu,FE_value theta,FE_value *x,FE_value *y,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 19 October 1995

DESCRIPTION :
For performing the Hammer projection starting from prolate spheroidal
coordinates.
==============================================================================*/

int polar_projection(FE_value mu,FE_value theta,FE_value *x,FE_value *y,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 19 October 1995

DESCRIPTION :
For performing the polar projection starting from prolate spheroidal
coordinates.
==============================================================================*/

int oblate_spheroidal_to_cartesian(FE_value lambda,FE_value mu,FE_value theta,
	FE_value focus,FE_value *x,FE_value *y,FE_value *z,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 8 September 1998

DESCRIPTION :
For transforming from oblate spheroidal to cartesian coordinates.
???DB.  What are the formulae.
==============================================================================*/

enum Coordinate_system_type get_coordinate_system_type(
	struct Coordinate_system *coordinate_system);
/*******************************************************************************
LAST MODIFIED : 25 January 1999

DESCRIPTION :
Returns the type of the coordinate system passed to it.
==============================================================================*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(Coordinate_system_type);

char *Coordinate_system_string(
	const Coordinate_system *coordinate_system);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Allocates and returns a pointer to a string containing the type and focus (if 
any) for the given coordinate_system. eg "rectangular_cartesian" or 
"prolate_spheroidal focus 1.0". Must Deallocate the string in the calling
function.
==============================================================================*/

int Coordinate_systems_match(struct Coordinate_system *coordinate_system1,
	struct Coordinate_system *coordinate_system2);
/*******************************************************************************
LAST MODIFIED : 25 August 1999

DESCRIPTION :
Returns true if the two coordinate systems are the same - includes comparing
focus for prolate and oblate spheroidal systems.
==============================================================================*/

int convert_Coordinate_system(
  struct Coordinate_system *source_coordinate_system,
  int number_of_source_coordinates, const FE_value *source_coordinates,
  struct Coordinate_system *destination_coordinate_system,
  int number_of_destinations_coordinates, FE_value *destination_coordinates,
  FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Convert the <source_coordinates> into the <destination_coordinates>.
Calculate the <jacobian> if not NULL.
???DB.  Should the dimension be part of struct Coordinate_system ?
???DB.  Can we get rid of most of io_devices/conversion ?
==============================================================================*/

/***************************************************************************//**
 * Converts source values and derivatives from their current coordinate system
 * into rectangular cartesian, returning them in the 3 component
 * <rc_coordinates> array. If <rc_derivatives> is not NULL, the derivatives are
 * also converted to rc and returned in that 9-component FE_value array.
 * Note that odd coordinate systems, such as FIBRE are treated as if they are
 * RECTANGULAR_CARTESIAN, which just causes a copy of values.
 * If <element_dimension> or the number of components in <field> are less than 3,
 * the missing places in the <rc_coordinates> and <rc_derivatives> arrays are
 * cleared to zero.
 *
 * Note the order of derivatives:
 * 1. All the <element_dimension> derivatives of component 1.
 * 2. All the <element_dimension> derivatives of component 2.
 * 3. All the <element_dimension> derivatives of component 3.
 */
int convert_coordinates_and_derivatives_to_rc(struct Coordinate_system *source_coordinate_system,
	int source_components, FE_value *source_coordinates, FE_value *source_derivatives,
	int element_dimension, FE_value *rc_coordinates, FE_value *rc_derivatives);

/***************************************************************************//**
 * Query if coordinate system type varies non-linearly with screen space.
 * Currently only RECTANGULAR_CARTESIAN and NORMALISED_WINDOW_COORDINATES are
 * linear, all other types are non-linear.
 * @return  1 if non-linear, 0 if linear.
 */
int Coordinate_system_type_is_non_linear(enum Coordinate_system_type type);

#endif /* !defined (GEOMETRY_H) */
