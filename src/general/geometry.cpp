/*******************************************************************************
FILE : geometry.c

LAST MODIFIED : 21 June 2000

DESCRIPTION :
Functions for performing coordinate transformations.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "general/enumerator_private.hpp"
#include "general/debug.h"
#include "general/geometry.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/message.h"

/*
Global functions
----------------
*/
int linear_transformation(Linear_transformation *linear_transformation,FE_value x,
	FE_value y,FE_value z,FE_value *result_x,FE_value *result_y,FE_value *result_z)
/*******************************************************************************
LAST MODIFIED : 14 July 1995

DESCRIPTION :
Performs a <linear_transformation>.
==============================================================================*/
{
	int return_code;

	ENTER(linear_transformation);
	return_code=1;
	if (linear_transformation)
	{
		*result_x=(linear_transformation->translate_x)+
			(linear_transformation->txx)*x+(linear_transformation->txy)*y+
			(linear_transformation->txz)*z;
		*result_y=(linear_transformation->translate_y)+
			(linear_transformation->tyx)*x+(linear_transformation->tyy)*y+
			(linear_transformation->tyz)*z;
		*result_z=(linear_transformation->translate_z)+
			(linear_transformation->tzx)*x+(linear_transformation->tzy)*y+
			(linear_transformation->tzz)*z;
	}
	else
	{
		*result_x=x;
		*result_y=y;
		*result_z=z;
	}
	LEAVE;

	return (return_code);
} /* linear_transformation */

int cartesian_to_cylindrical_polar(FE_value x,FE_value y,FE_value z_in,FE_value *r,
	FE_value *theta,FE_value *z,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For transforming from cartesian to cylindrical polar coordinates.
x = r*cos(theta)
y = r*sin(theta)
z = z
==============================================================================*/
{
	FE_value r2;
	int return_code;

	ENTER(cartesian_to_cylindrical_polar);
	return_code=1;
	r2=x*x+y*y;
	*r=(FE_value)sqrt(r2);
	if ((x!=0)||(y!=0))
	{
		*theta=(FE_value)atan2(y,x);
	}
	else
	{
		*theta=(FE_value)0;
	}
	*z=z_in;
	if (jacobian)
	{
		if (*r>0)
		{
			jacobian[0]=x/(*r);
			jacobian[1]=y/(*r);
			jacobian[2]=(FE_value)0;
			jacobian[3]= -y/r2;
			jacobian[4]=x/r2;
			jacobian[5]=(FE_value)0;
		}
		else
		{
			jacobian[0]=(FE_value)0;
			jacobian[1]=(FE_value)0;
			jacobian[2]=(FE_value)0;
			jacobian[3]=(FE_value)0;
			jacobian[4]=(FE_value)0;
			jacobian[5]=(FE_value)0;
		}
		jacobian[6]=(FE_value)0;
		jacobian[7]=(FE_value)0;
		jacobian[8]=(FE_value)1;
	}
	LEAVE;

	return (return_code);
} /* cartesian_to_cylindrical_polar */

int cylindrical_polar_to_cartesian(FE_value r,FE_value theta,FE_value z_in,FE_value *x,
	FE_value *y,FE_value *z,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For transforming from cylindrical polar to cartesian coordinates.
x = r*cos(theta)
y = r*sin(theta)
z = z_in
==============================================================================*/
{
	int return_code;

	ENTER(cylindrical_polar_to_cartesian);
	return_code=1;
	*x=r*(FE_value)cos(theta);
	*y=r*(FE_value)sin(theta);
	*z=z_in;
	if (jacobian)
	{
		jacobian[0]=(FE_value)cos(theta);
		jacobian[1]= -r*(FE_value)sin(theta);
		jacobian[2]=(FE_value)0;
		jacobian[3]=(FE_value)sin(theta);
		jacobian[4]=r*(FE_value)cos(theta);
		jacobian[5]=(FE_value)0;
		jacobian[6]=(FE_value)0;
		jacobian[7]=(FE_value)0;
		jacobian[8]=(FE_value)1;
	}
	LEAVE;

	return (return_code);
} /* cylindrical_polar_to_cartesian */

int cartesian_to_spherical_polar(FE_value x,FE_value y,FE_value z,FE_value *r,FE_value *theta,
	FE_value *phi,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 18 January 2000

DESCRIPTION :
For transforming from spherical polar to cartesian coordinates.
x = r*cos(phi)*cos(theta)
y = r*cos(phi)*sin(theta)
z = r*sin(phi)
==============================================================================*/
{
	double cos_phi,cos_theta,r_temp,sin_phi,sin_theta;
	int return_code;

	ENTER(cartesian_to_spherical_polar);
	return_code=1;
	r_temp=sqrt(x*x+y*y+z*z);
	*r=(FE_value)r_temp;
	if (0<r_temp)
	{
		sin_phi=(double)z/r_temp;
		cos_phi=sqrt(x*x+y*y)/r_temp;
		*phi=(FE_value)atan2(sin_phi,cos_phi);
		if (0<cos_phi)
		{
			cos_theta=(double)x/(r_temp*cos_phi);
			sin_theta=(double)y/(r_temp*cos_phi);
			*theta=(FE_value)atan2(sin_theta,cos_theta);
			if (jacobian)
			{
				jacobian[0]=(FE_value)(cos_phi*cos_theta);
				jacobian[1]=(FE_value)(cos_phi*sin_theta);
				jacobian[2]=(FE_value)sin_phi;
				jacobian[3]=(FE_value)(-sin_theta/(r_temp*cos_phi));
				jacobian[4]=(FE_value)(cos_theta/(r_temp*cos_phi));
				jacobian[5]=(FE_value)0;
				jacobian[6]=(FE_value)(-sin_phi*cos_theta/r_temp);
				jacobian[7]=(FE_value)(-sin_phi*sin_theta/r_temp);
				jacobian[8]=(FE_value)(cos_phi/r_temp);
			}
		}
		else
		{
			*theta=(FE_value)0;
			if (jacobian)
			{
				jacobian[0]=(FE_value)0;
				jacobian[1]=(FE_value)0;
				jacobian[2]=(FE_value)0;
				jacobian[3]=(FE_value)0;
				jacobian[4]=(FE_value)0;
				jacobian[5]=(FE_value)0;
				jacobian[6]=(FE_value)0;
				jacobian[7]=(FE_value)0;
				jacobian[8]=(FE_value)0;
			}
		}
	}
	else
	{
		*theta=(FE_value)0;
		*phi=(FE_value)0;
		if (jacobian)
		{
			jacobian[0]=(FE_value)0;
			jacobian[1]=(FE_value)0;
			jacobian[2]=(FE_value)0;
			jacobian[3]=(FE_value)0;
			jacobian[4]=(FE_value)0;
			jacobian[5]=(FE_value)0;
			jacobian[6]=(FE_value)0;
			jacobian[7]=(FE_value)0;
			jacobian[8]=(FE_value)0;
		}
	}
	LEAVE;

	return (return_code);
} /* cartesian_to_spherical_polar */

int spherical_polar_to_cartesian(FE_value r,FE_value theta,FE_value phi,FE_value *x,
	FE_value *y,FE_value *z,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For transforming from spherical polar to cartesian coordinates.
x = r*cos(phi)*cos(theta)
y = r*cos(phi)*sin(theta)
z = r*sin(phi)
==============================================================================*/
{
	int return_code;

	ENTER(spherical_polar_to_cartesian);
	return_code=1;
	*x=r*(FE_value)cos(phi);
	*y=(*x)*(FE_value)sin(theta);
	*x *= (FE_value)cos(theta);
	*z=r*(FE_value)sin(phi);
	if (jacobian)
	{
		jacobian[0]=(FE_value)(cos(phi)*cos(theta));
		jacobian[1]= -r*(FE_value)(cos(phi)*sin(theta));
		jacobian[2]= -r*(FE_value)(sin(phi)*cos(theta));
		jacobian[3]=(FE_value)(cos(phi)*sin(theta));
		jacobian[4]=r*(FE_value)(cos(phi)*cos(theta));
		jacobian[5]= -r*(FE_value)(sin(phi)*sin(theta));
		jacobian[6]=(FE_value)sin(phi);
		jacobian[7]=(FE_value)0;
		jacobian[8]=r*(FE_value)cos(phi);
	}
	LEAVE;

	return (return_code);
} /* spherical_polar_to_cartesian */

int cartesian_to_prolate_spheroidal(FE_value x,FE_value y,FE_value z,FE_value focus,
	FE_value *lambda,FE_value *mu,FE_value *theta,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For transforming from cartesian to prolate spheroidal coordinates.
x = focus*cosh(lambda)*cos(mu)
y = focus*sinh(lambda)*sin(mu)*cos(theta)
z = focus*sinh(lambda)*sin(mu)*sin(theta)
==============================================================================*/
{
	int return_code;
	double a1,a2,a3,a4,a5,a6,a7,a8,a9,x_norm,y_norm,z_norm;

	ENTER(cartesian_to_prolate_spheroidal);
	return_code=1;
	/* normalize the cartesian coordianates */
	x_norm=(double)x/(double)focus;
	y_norm=(double)y/(double)focus;
	z_norm=(double)z/(double)focus;
	/* sinh(lambda)*sinh(lambda) - sin(mu)*sin(mu) */
	a1=x_norm*x_norm+y_norm*y_norm+z_norm*z_norm-1;
	/* sinh(lambda)*sinh(lambda) + sin(mu)*sin(mu) */
	a2=sqrt(a1*a1+4*(y_norm*y_norm+z_norm*z_norm));
	/* sinh(lambda)*sinh(lambda) */
	a3=(a1+a2)/2;
	if (a3<0)
	{
		a3=0;
	}
	/* sinh(lambda) */
	a4=sqrt(a3);
	/* sin(mu)*sin(mu) */
	a5=(a2-a1)/2;
	if (a5>1)
	{
		a5=1;
	}
	else
	{
		if (a5<0)
		{
			a5=0;
		}
	}
	/* sin(mu) */
	a6=sqrt(a5);
	*mu=(FE_value)asin(a6);
	/* sin(theta) */
	if ((a7=a4*a6)>0)
	{
		a7=z_norm/a7;
	}
	if (a7>=1)
	{
		a7=1;
		*theta=(FE_value)(PI/2.);
	}
	else
	{
		if (a7<= -1)
		{
			a7=1;
			*theta=(FE_value)(-PI/2.);
		}
		else
		{
			*theta=(FE_value)asin(a7);
		}
	}
	*lambda=(FE_value)log(a4+sqrt(1+a3));
	if (x<0)
	{
		*mu=(FE_value)PI-(*mu);
	}
	if (y<0)
	{
		*theta=(FE_value)PI-(*theta);
	}
	else
	{
		if (*theta<0)
		{
			*theta += (FE_value)(2.*PI);
		}
	}
	if (jacobian)
	{
		/* a2=(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda)) */
		/* a4=sinh(lambda) */
		/* a6=sin(mu) */
		/* a5=sinh(lambda)*sin(mu) */
		a5=a4*a6;
		if ((a2>0)&&(a5>0))
		{
			a2 *= focus;
			a5 *= focus;
			/* a3=cosh(lambda) */
			a3=sqrt(1+a3);
			/* a8=cos(mu) */
			a8=sqrt(1-a6*a6);
			if (x<0)
			{
				a8= -a8;
			}
			/* a9=cos(theta) */
			a9=sqrt(1-a7*a7);
			if (y<0)
			{
				a9= -a9;
			}
			/* a1=sinh(lambda)*cos(mu) */
			a1=a4*a8;
			/* a3=cosh(lambda)*sin(mu) */
			a3 *= a6;
			/* sinh(lambda)*cos(mu)/(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda))) */
			jacobian[0]=(FE_value)(a1/a2);
			/* cosh(lambda)*sin(mu)*cos(theta)/
				(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda))) */
			jacobian[1]=(FE_value)(a3*a9/a2);
			/* cosh(lambda)*sin(mu)*sin(theta)/
				(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda))) */
			jacobian[2]=(FE_value)(a3*a7/a2);
			/* -cosh(lambda)*sin(mu)/(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda))) */
			jacobian[3]=(FE_value)(-a3/a2);
			/* sinh(lambda)*cos(mu)*cos(theta)/
				(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda))) */
			jacobian[4]=(FE_value)(a1*a9/a2);
			/* sinh(lambda)*cos(mu)*sin(theta)/
				(sin(mu)*sin(mu)+sinh(lambda)*sinh(lambda))) */
			jacobian[5]=(FE_value)(a1*a7/a2);
			jacobian[6]=(FE_value)0;
			/* -sin(theta)/(sinh(lambda)*sin(mu)) */
			jacobian[7]=(FE_value)(-a7/a5);
			/* cos(theta)/(sinh(lambda)*sin(mu)) */
			jacobian[8]=(FE_value)(a9/a5);
		}
		else
		{
			return_code=0;
		}
	}
	LEAVE;

	return (return_code);
} /* cartesian_to_prolate_spheroidal */

int prolate_spheroidal_to_cartesian(FE_value lambda,FE_value mu,FE_value theta,
	FE_value focus,FE_value *x,FE_value *y,FE_value *z,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For transforming from prolate spheroidal to cartesian coordinates.
x = focus*cosh(lambda)*cos(mu)
y = focus*sinh(lambda)*sin(mu)*cos(theta)
z = focus*sinh(lambda)*sin(mu)*sin(theta)
==============================================================================*/
{
	int return_code;
	double a1,a2,a3,a4,a5,a6,a7,a8,a9;

	ENTER(prolate_spheroidal_to_cartesian);
	return_code=1;
	a1=(double)focus*sinh((double)lambda);
	a2=(double)focus*cosh((double)lambda);
	a3=sin((double)mu);
	a4=cos((double)mu);
	a5=sin((double)theta);
	a6=cos((double)theta);
	a7=a1*a3;
	*x=(FE_value)(a2*a4);
	*y=(FE_value)(a7*a6);
	*z=(FE_value)(a7*a5);
	if (jacobian)
	{
		a8=a1*a4;
		a9=a2*a3;
		jacobian[0]=(FE_value)a8;
		jacobian[1]=(FE_value)(-a9);
		jacobian[2]=(FE_value)0;
		jacobian[3]=(FE_value)(a9*a6);
		jacobian[4]=(FE_value)(a8*a6);
		jacobian[5]=(FE_value)(-a7*a5);
		jacobian[6]=(FE_value)(a9*a5);
		jacobian[7]=(FE_value)(a8*a5);
		jacobian[8]=(FE_value)(a7*a6);
	}
	LEAVE;

	return (return_code);
} /* prolate_spheroidal_to_cartesian */

int Hammer_projection(FE_value mu,FE_value theta,FE_value *x,FE_value *y,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For performing the Hammer projection starting from prolate spheroidal
coordinates.
==============================================================================*/
{
	int return_code;
	double a1,a2,a3,a4,a5,a6,a7,k;

	ENTER(Hammer_projection);
	a1=(double)mu-(PI/2);
	a2=cos(a1);
	a3=fmod((double)theta,2*PI);
	if (a3<0)
	{
		a3=(a3+PI)/2;
	}
	else
	{
		a3=(a3-PI)/2;
	}
	a4=cos(a3);
	a5=1+a2*a4;
	if (a5>0)
	{
		k=1/sqrt(a5);
		a6=sin(a1);
		a7=sin(a3);
		*x=(FE_value)(-k*a2*a7);
		*y=(FE_value)(k*a6);
		if (jacobian)
		{
			jacobian[0]=(FE_value)(0.5*k*a6*a7*(1+a5)/a5);
			jacobian[1]=(FE_value)(-0.25*a2*k*(2*a4+a2*(1+a4*a4))/a5);
			jacobian[2]=(FE_value)(0.5*k*(2*a2+a4*(1+a2*a2))/a5);
			jacobian[3]=(FE_value)(0.25*k*a6*a2*a7/a5);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Hammer_projection.  Divide by 0");
		return_code=0;
	}
/*  a1=(double)mu-(PI/2);
	a2=cos(a1);
	a3=((double)theta-PI)/2;
	k=1+a2*cos(a3);
	if (k>0)
	{
		k=1/sqrt(k);
		*x=(ZnReal)(-k*a2*sin(a3));
		*y=(ZnReal)(k*sin(a1));
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Hammer_projection.  Divide by 0");
		return_code=0;
	}*/
	LEAVE;

	return (return_code);
} /* Hammer_projection */

int polar_projection(FE_value mu,FE_value theta,FE_value *x,FE_value *y,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
For performing the polar projection starting from prolate spheroidal
coordinates.
x=mu*cos(theta)
y=mu*sin(theta)
==============================================================================*/
{
	double a1,a2;
	int return_code;

	ENTER(polar_projection);
	a1=cos((double)theta);
	a2=sin((double)theta);
	*x=(FE_value)((double)mu*a1);
	*y=(FE_value)((double)mu*a2);
	if (jacobian)
	{
		jacobian[0]=(FE_value)a1;
		jacobian[1]= -(*y);
		jacobian[2]=(FE_value)a2;
		jacobian[3]= *x;
	}
	return_code=1;
	LEAVE;

	return (return_code);
} /* polar_projection */

int oblate_spheroidal_to_cartesian(FE_value lambda,FE_value mu,FE_value theta,
	FE_value focus,FE_value *x,FE_value *y,FE_value *z,FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 24 November 1999

DESCRIPTION :
For transforming from oblate spheroidal to cartesian coordinates.
x = focus*cosh(lambda)*cos(mu)*sin(theta)
y = focus*sinh(lambda)*sin(mu)
z = focus*cosh(lambda)*cos(mu)*cos(theta)
==============================================================================*/
{
	int return_code;
	double a1,a2,a3,a4,a5,a6,a7,a8,a9;

	ENTER(oblate_spheroidal_to_cartesian);
	return_code=1;
	a1=(double)focus*sinh((double)lambda);
	a2=(double)focus*cosh((double)lambda);
	a3=sin((double)mu);
	a4=cos((double)mu);
	a5=sin((double)theta);
	a6=cos((double)theta);
	a7=a2*a4;
	*x=(FE_value)(a7*a5);
	*y=(FE_value)(a1*a3);
	*z=(FE_value)(a7*a6);
	if (jacobian)
	{
		a8=a1*a4;
		a9=a2*a3;
		jacobian[0]=(FE_value)(a8*a5);
		jacobian[1]=(FE_value)(-a9*a5);
		jacobian[2]=(FE_value)(a7*a6);
		jacobian[3]=(FE_value)a9;
		jacobian[4]=(FE_value)a8;
		jacobian[5]=(FE_value)0;
		jacobian[6]=(FE_value)(a8*a6);
		jacobian[7]=(FE_value)(-a9*a6);
		jacobian[8]=(FE_value)(-a7*a5);
	}
	LEAVE;

	return (return_code);
} /* oblate_spheroidal_to_cartesian */

enum Coordinate_system_type get_coordinate_system_type(
	struct Coordinate_system *coordinate_system)
/*******************************************************************************
LAST MODIFIED : 25 January 1999

DESCRIPTION :
Returns the type of the coordinate system passed to it.
==============================================================================*/
{
	enum Coordinate_system_type coordinate_system_type;

	ENTER(get_coordinate_system_type);
	if (coordinate_system)
	{
		coordinate_system_type=coordinate_system->type;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_coordinate_system_type.  Invalid coordinate_system ");
		coordinate_system_type=UNKNOWN_COORDINATE_SYSTEM;
	}
	LEAVE;

	return (coordinate_system_type);
} /* get_coordinate_system_type */

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Coordinate_system_type)
/*******************************************************************************
LAST MODIFIED : 11 February 2003

DESCRIPTION :
Returns a pointer to a static string token for the given <type>.
The calling function must not deallocate the returned string.
*** Must ensure implemented correctly for new Coordinate_system_type ***
==============================================================================*/
{
	const char *coordinate_system_type_string;

	ENTER(ENUMERATOR_STRING(Coordinate_system_type));
	switch (enumerator_value)
	{
		case RECTANGULAR_CARTESIAN:
		{
			coordinate_system_type_string="rectangular_cartesian";
		} break;
		case CYLINDRICAL_POLAR:
		{
			coordinate_system_type_string="cylindrical_polar";
		} break;
		case SPHERICAL_POLAR:
		{
			coordinate_system_type_string="spherical_polar";
		} break;
		case PROLATE_SPHEROIDAL:
		{
			coordinate_system_type_string="prolate_spheroidal";
		} break;
		case OBLATE_SPHEROIDAL:
		{
			coordinate_system_type_string="oblate_spheroidal";
		} break;
		case FIBRE:
		{
			coordinate_system_type_string="fibre";
		} break;
		case NORMALISED_WINDOW_COORDINATES:
		{
			coordinate_system_type_string="normalised_window_coordinates";
		} break;
		case NOT_APPLICABLE:
		{
			coordinate_system_type_string="not_applicable";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Coordinate_system_type_to_string.  Invalid coordinate system type");
			coordinate_system_type_string=(const char *)NULL;
		} break;
	}
	LEAVE;

	return (coordinate_system_type_string);
} /* ENUMERATOR_STRING(Coordinate_system_type) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Coordinate_system_type)

char *Coordinate_system_string(const Coordinate_system *coordinate_system)
/*******************************************************************************
LAST MODIFIED : 24 November 1999

DESCRIPTION :
Allocates and returns a pointer to a string containing the type and focus (if
any) for the given coordinate_system. eg "rectangular_cartesian" or
"prolate_spheroidal focus 1". Must Deallocate the string in the calling
function.
==============================================================================*/
{
	char *coordinate_system_string;
	char global_temp_string[1000];
	int error;

	ENTER(Coordinate_system_string);
	coordinate_system_string=(char *)NULL;
	if (coordinate_system)
	{
		if ((coordinate_system_string=duplicate_string(
					ENUMERATOR_STRING(Coordinate_system_type)(coordinate_system->type))))
		{
			/* need to deal with focus */
			error=0;
			switch (coordinate_system->type)
			{
				case PROLATE_SPHEROIDAL:
				case OBLATE_SPHEROIDAL:
				{
					sprintf(global_temp_string," focus %g",
						coordinate_system->parameters.focus);
					append_string(&coordinate_system_string,global_temp_string,&error);
				} break;
				default:
				{
				}
			}
			if (error)
			{
				display_message(ERROR_MESSAGE,
					"Coordinate_system_string.  Error appending focus");
				DEALLOCATE(coordinate_system_string);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Coordinate_system_string.  Error duplicating type string");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Coordinate_system_string.  Invalid argument");
	}
	LEAVE;

	return (coordinate_system_string);
} /* Coordinate_system_string */

int Coordinate_systems_match(struct Coordinate_system *coordinate_system1,
	struct Coordinate_system *coordinate_system2)
/*******************************************************************************
LAST MODIFIED : 21 June 2000

DESCRIPTION :
Returns true if the two coordinate systems are the same - includes comparing
focus for prolate and oblate spheroidal systems.
==============================================================================*/
{
	int return_code;

	ENTER(Coordinate_systems_match);
	return_code=0;
	if (coordinate_system1&&coordinate_system2)
	{
		if (coordinate_system1->type==coordinate_system2->type)
		{
			if ((PROLATE_SPHEROIDAL==coordinate_system1->type)||
				(OBLATE_SPHEROIDAL==coordinate_system1->type))
			{
				/* check focus matches */
				if (coordinate_system1->parameters.focus==
					coordinate_system2->parameters.focus)
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			}
			else
			{
				return_code=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Coordinate_systems_match. Invalid argument");
	}
	LEAVE;

	return (return_code);
} /* Coordinate_systems_match */

int convert_Coordinate_system(
	struct Coordinate_system *source_coordinate_system,
	int number_of_source_coordinates, const FE_value *source_coordinates,
	struct Coordinate_system *destination_coordinate_system,
	int number_of_destination_coordinates, FE_value *destination_coordinates,
	FE_value *jacobian)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Convert the <source_coordinates> into the <destination_coordinates>.
Calculate the <jacobian> if not NULL.
???DB.  Should the dimension be part of struct Coordinate_system ?
???DB.  Can we get rid of most of io_devices/conversion ?
==============================================================================*/
{
	FE_value *destination_values,local_destination_values[3],local_source_values[3],
		*jacobian_1,*jacobian_2, temp1[9],temp2[9],x,y,z;
	int return_code = 0;

	ENTER(convert_Coordinate_system);
	if (source_coordinate_system&&(number_of_source_coordinates>0)&&
		source_coordinates&&destination_coordinate_system&&
		(number_of_destination_coordinates>0)&&destination_coordinates)
	{
		/* Either point to the actual values if they are 3D or
			copy and point if they are less */
		const FE_value *source_values;
		if (number_of_source_coordinates >= 3)
		{
			source_values = source_coordinates;
		}
		else
		{
			local_source_values[0] = source_coordinates[0];
			if (number_of_source_coordinates >=2)
			{
				local_source_values[1] = source_coordinates[1];
			}
			else
			{
				local_source_values[1] = 0;
			}
			local_source_values[2] = 0;
			source_values = local_source_values;
		}
		if (number_of_destination_coordinates >= 3)
		{
			destination_values = destination_coordinates;
		}
		else
		{
			destination_values = local_destination_values;
		}
		/* for the two stage conversion */
		if (jacobian)
		{
			jacobian_1=temp1;
			jacobian_2=temp2;
		}
		else
		{
			/* set to NULL if no input jacobian defined */
			jacobian_1=(FE_value *)NULL;
			jacobian_2=(FE_value *)NULL;
		}
		switch (source_coordinate_system->type)
		{
			case RECTANGULAR_CARTESIAN:
			case NORMALISED_WINDOW_COORDINATES:
			{
				switch (destination_coordinate_system->type)
				{
					case CYLINDRICAL_POLAR:
					{
						return_code=cartesian_to_cylindrical_polar(source_values[0],
							source_values[1],source_values[2],
							&destination_values[0],&destination_values[1],
							&destination_values[2],jacobian);
					} break;
					case SPHERICAL_POLAR:
					{
						return_code=cartesian_to_spherical_polar(source_values[0],
							source_values[1],source_values[2],
							&destination_values[0],&destination_values[1],
							&destination_values[2],jacobian);
					} break;
					case  PROLATE_SPHEROIDAL:
					{
						return_code=cartesian_to_prolate_spheroidal(source_values[0],
							source_values[1],source_values[2],
							destination_coordinate_system->parameters.focus,
							&destination_values[0],&destination_values[1],
							&destination_values[2],jacobian);
					} break;
					case RECTANGULAR_CARTESIAN:
					case NORMALISED_WINDOW_COORDINATES:
					{
						/* just do a copy */
						memcpy(destination_values,source_values,
							3*sizeof(destination_values));
						if (jacobian)
						{
							identity_matrix_FE_value(3, jacobian);
						}
						return_code=1;
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,"convert_Coordinate_system.  "
							"Conversion from rectangular_cartesian to %s not implemented",
							ENUMERATOR_STRING(Coordinate_system_type)(
							destination_coordinate_system->type));
						return_code=0;
					} break;
				}
			} break;
			case CYLINDRICAL_POLAR:
			{
				switch (destination_coordinate_system->type)
				{
					case RECTANGULAR_CARTESIAN:
					case NORMALISED_WINDOW_COORDINATES:
					{
						return_code=cylindrical_polar_to_cartesian(source_values[0],
							source_values[1],source_values[2],
							&destination_values[0],&destination_values[1],
							&destination_values[2],jacobian);
					} break;
					case PROLATE_SPHEROIDAL:
					{
						/* two stage conversion */
						return_code=cylindrical_polar_to_cartesian(source_values[0],
							source_values[1],source_values[2],&x,&y,&z,jacobian_1);
						if (return_code)
						{
							return_code=cartesian_to_prolate_spheroidal(x,y,z,
								destination_coordinate_system->parameters.focus,
								&destination_values[0],&destination_values[1],
								&destination_values[2],jacobian_2);
							/* if the jacobians were defined calculate the overall one */
							if (return_code&&jacobian_1&&jacobian_2)
							{
								/* jacobian=jacobian_2*jacobian_1 */
								return_code=multiply_matrix_FE_value(3,3,3,jacobian_2,
									jacobian_1,jacobian);
							}
						}
					} break;
					case CYLINDRICAL_POLAR:
					{
						/* just do a copy */
						memcpy(destination_values, source_values, 3*sizeof(destination_values));
						if (jacobian)
						{
							identity_matrix_FE_value(3, jacobian);
						}
						return_code = 1;
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,"convert_Coordinate_system.  "
							"Conversion from cylindrical_polar to %s not implemented",
							ENUMERATOR_STRING(Coordinate_system_type)(
							destination_coordinate_system->type));
						return_code=0;
					} break;
				}
			} break;
			case SPHERICAL_POLAR:
			{
				switch (destination_coordinate_system->type)
				{
					case RECTANGULAR_CARTESIAN:
					case NORMALISED_WINDOW_COORDINATES:
					{
						return_code=spherical_polar_to_cartesian(source_values[0],
							source_values[1],source_values[2],
							&destination_values[0],&destination_values[1],
							&destination_values[2],jacobian);
					} break;
					case PROLATE_SPHEROIDAL:
					{
						/* two stage conversion */
						return_code=spherical_polar_to_cartesian(source_values[0],
							source_values[1],source_values[2],&x,&y,&z,jacobian_1);
						if (return_code)
						{
							return_code=cartesian_to_prolate_spheroidal(x,y,z,
								destination_coordinate_system->parameters.focus,
								&destination_values[0],&destination_values[1],
								&destination_values[2],jacobian_2);
							/* if the jacobians were defined calculate the overall one */
							if (return_code&&jacobian_1&&jacobian_2)
							{
								/* jacobian=jacobian_2*jacobian_1 */
								return_code=multiply_matrix_FE_value(3,3,3,jacobian_2,
									jacobian_1,jacobian);
							}
						}
					} break;
					case CYLINDRICAL_POLAR:
					{
						/* two stage conversion */
						return_code=spherical_polar_to_cartesian(source_values[0],
							source_values[1],source_values[2],&x,&y,&z,jacobian_1);
						if (return_code)
						{
							return_code=cartesian_to_cylindrical_polar(x,y,z,
								&destination_values[0],&destination_values[1],
								&destination_values[2],jacobian_2);
							/* if the jacobians were defined calculate the overall one */
							if (return_code&&jacobian_1&&jacobian_2)
							{
								/* jacobian=jacobian_2*jacobian_1 */
								return_code=multiply_matrix_FE_value(3,3,3,jacobian_2,
									jacobian_1,jacobian);
							}
						}
					} break;
					case SPHERICAL_POLAR:
					{
						/* just do a copy */
						memcpy(destination_values, source_values, 3*sizeof(destination_values));
						if (jacobian)
						{
							identity_matrix_FE_value(3, jacobian);
						}
						return_code = 1;
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,"convert_Coordinate_system.  "
							"Conversion from spherical_polar to %s not implemented",
							ENUMERATOR_STRING(Coordinate_system_type)(
							destination_coordinate_system->type));
						return_code=0;
					} break;
				}
			} break;
			case PROLATE_SPHEROIDAL:
			{
				switch (destination_coordinate_system->type)
				{
					case RECTANGULAR_CARTESIAN:
					case NORMALISED_WINDOW_COORDINATES:
					{
						return_code=prolate_spheroidal_to_cartesian(source_values[0],
							source_values[1],source_values[2],
							source_coordinate_system->parameters.focus,
							&destination_values[0],&destination_values[1],
							&destination_values[2],jacobian);
					} break;
					case CYLINDRICAL_POLAR:
					{
						/* two stage conversion */
						return_code=prolate_spheroidal_to_cartesian(source_values[0],
							source_values[1],source_values[2],
							source_coordinate_system->parameters.focus,&x,&y,&z,jacobian_1);
						if (return_code)
						{
							return_code=cartesian_to_cylindrical_polar(x,y,z,
								&destination_values[0],&destination_values[1],
								&destination_values[2],jacobian_2);
							/* if the jacobians were defined calculate the overall one */
							if (return_code&&jacobian_1&&jacobian_2)
							{
								/* jacobian=jacobian_2*jacobian_1 */
								return_code=multiply_matrix_FE_value(3,3,3,jacobian_2,
									jacobian_1,jacobian);
							}
						}
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,"convert_Coordinate_system.  "
							"Conversion from prolate_spheroidal to %s not implemented",
							ENUMERATOR_STRING(Coordinate_system_type)(
							destination_coordinate_system->type));
						return_code=0;
					} break;
				}
			} break;
			case OBLATE_SPHEROIDAL:
			{
				display_message(ERROR_MESSAGE,"convert_Coordinate_system.  "
					"Conversion from oblate_spheroidal not implemented");
				return_code=0;
			} break;
			case FIBRE:
			{
				display_message(ERROR_MESSAGE,"convert_Coordinate_system.  "
					"Conversion from fibre not implemented ");
				return_code=0;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"convert_Coordinate_system. Invalid source coordinate system type");
				return_code=0;
			} break;
		}
		if (number_of_destination_coordinates < 3)
		{
			destination_coordinates[0] = local_destination_values[0];
			if (number_of_destination_coordinates >=2)
			{
				destination_coordinates[1] = local_destination_values[1];
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"convert_Coordinate_system.  Invalid argument");
	}
	LEAVE;

	return (return_code);
} /* convert_Coordinate_system */

int convert_coordinates_and_derivatives_to_rc(struct Coordinate_system *source_coordinate_system,
	int source_components, FE_value *source_coordinates, FE_value *source_derivatives,
	int element_dimension, FE_value *rc_coordinates, FE_value *rc_derivatives)
{
	FE_value *source;
	FE_value coordinates[3],derivatives[9],*destination,x,y,z,*jacobian,temp[9];
	int i,j,return_code;

	if (source_coordinate_system && source_components && source_coordinates &&
		rc_coordinates && ((0 == rc_derivatives) || (source_derivatives && (0 < element_dimension))))
	{
		/* copy coordinates, padding to 3 components */
		for (i=0;i<3;i++)
		{
			if (i<source_components)
			{
				coordinates[i] = source_coordinates[i];
			}
			else
			{
				coordinates[i]=0.0;
			}
		}
		if (rc_derivatives)
		{
			/* copy derivatives, padding to 3 components x 3 dimensions */
			destination=derivatives;
			source=source_derivatives;
			for (i=0;i<3;i++)
			{
				for (j=0;j<3;j++)
				{
					if ((i<source_components)&&(j<element_dimension))
					{
						*destination = *source;
						source++;
					}
					else
					{
						*destination = 0.0;
					}
					destination++;
				}
			}
			/* make sure jacobian only calculated if rc_derivatives requested */
			jacobian=temp;
		}
		else
		{
			jacobian=NULL;
		}

		switch (source_coordinate_system->type)
		{
			case CYLINDRICAL_POLAR:
			{
				cylindrical_polar_to_cartesian(
					coordinates[0],coordinates[1],coordinates[2],&x,&y,&z,jacobian);
			} break;
			case SPHERICAL_POLAR:
			{
				spherical_polar_to_cartesian(
					coordinates[0],coordinates[1],coordinates[2],&x,&y,&z,jacobian);
			} break;
			case PROLATE_SPHEROIDAL:
			{
				prolate_spheroidal_to_cartesian(
					coordinates[0],coordinates[1],coordinates[2],
					source_coordinate_system->parameters.focus,&x,&y,&z,jacobian);
			} break;
			case OBLATE_SPHEROIDAL:
			{
				oblate_spheroidal_to_cartesian(
					coordinates[0],coordinates[1],coordinates[2],
					source_coordinate_system->parameters.focus,&x,&y,&z,jacobian);
			} break;
			default:
			{
				/* treat all others as RECTANGULAR_CARTESIAN; copy coordinates */
				x=coordinates[0];
				y=coordinates[1];
				z=coordinates[2];
				if (rc_derivatives)
				{
					for (i=0;i<9;i++)
					{
						rc_derivatives[i]=static_cast<FE_value>(derivatives[i]);
					}
					/* clear jacobian to avoid derivative conversion below */
					jacobian=NULL;
				}
			} break;
		}
		rc_coordinates[0]=x;
		rc_coordinates[1]=y;
		rc_coordinates[2]=z;
		if (jacobian)
		{
			for (i=0;i<3;i++)
			{
				/* derivative of x with respect to xi[i] */
				rc_derivatives[i]=
					jacobian[0]*derivatives[0+i]+
					jacobian[1]*derivatives[3+i]+
					jacobian[2]*derivatives[6+i];
				/* derivative of y with respect to xi[i] */
				rc_derivatives[3+i]=
					jacobian[3]*derivatives[0+i]+
					jacobian[4]*derivatives[3+i]+
					jacobian[5]*derivatives[6+i];
				/* derivative of z with respect to xi[i] */
				rc_derivatives[6+i]=
					jacobian[6]*derivatives[0+i]+
					jacobian[7]*derivatives[3+i]+
					jacobian[8]*derivatives[6+i];
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"convert_coordinates_and_derivatives_to_rc.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
}

int Coordinate_system_type_is_non_linear(enum Coordinate_system_type type)
{
	return ((RECTANGULAR_CARTESIAN != type) &&
		(NORMALISED_WINDOW_COORDINATES != type));
}
