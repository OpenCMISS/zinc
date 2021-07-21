
/*
FILE : cad_element.h

CREATED : 19 June 2010

DESCRIPTION :
The data structures used for representing cad elements in the graphical
interface to CMZN.
*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CAD_ELEMENT_H)
#define CAD_ELEMENT_H

enum Cad_primitive_type
{
	Cad_primitive_INVALID,
	Cad_primitive_SHAPE,
	Cad_primitive_SURFACE,
	Cad_primitive_CURVE,
	Cad_primitive_POINT
};

struct Cad_primitive_identifier
{
	enum Cad_primitive_type type;
	int number;
};

const char *Cad_primitive_type_string(Cad_primitive_type type);

#endif /* !defined (CAD_ELEMENT_H) */

