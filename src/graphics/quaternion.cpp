/*******************************************************************************
FILE : quaternion.hpp

LAST MODIFIED : 17 October 2007

DESCRIPTION : A class of quaternion operations, any new quaternion
operations should be added into this class.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "quaternion.hpp"
#include <math.h>

#include "general/debug.h"

void Quaternion::set(const double quat_w, const double quat_x, const double quat_y, const double quat_z)
{
	 ENTER(Quaternion::set);
	 w = quat_w;
	 x = quat_x;
	 y = quat_y;
	 z = quat_z;
	 LEAVE;
}

 void Quaternion::get(double *values) const
/*******************************************************************************
LAST MODIFIED : 18 Oct 2007

DESCRIPTION :
get the values stored in this quaternion and put it into the argument
values, must allocate values before calling this function.
==============================================================================*/
 {
		ENTER(Quaternion::get);
		values[0] = w;
		values[1] = x;
		values[2] = y;
		values[3] = z;
		LEAVE;
 }
 
 void Quaternion::normalise()
 {
		ENTER(Quaternion::normalise);
		const double tolerance = 0.00000001;
		const double magnitude = sqrt(w*w+ x*x + y*y + z*z);
		if (fabs(magnitude - 1.0) > tolerance)
		{
			 w = w / magnitude;
			 x = x / magnitude;
			 y = y / magnitude;
			 z = z / magnitude;
		}
		LEAVE;
 }

 void Quaternion::interpolated_with_SLERP(const Quaternion &from, const Quaternion &to, const double normalised_time)
/*******************************************************************************
LAST MODIFIED : 18 Oct 2007

DESCRIPTION :
Spherical linear interpolation of two quaternions at time index
(normalised time between these two quaternion).
==============================================================================*/
 {
		ENTER(Quaternion::interpolated_with_SLERP);
		const double tolerance = 0.00000001;
		double cosOmega = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
		double tol[4];
		if (cosOmega < 0.0)
		{
			 cosOmega = -cosOmega;
			 tol[0] = - to.x;
			 tol[1] = - to.y;
			 tol[2] = - to.z;
			 tol[3] = - to.w;
		}
		else
		{
			 tol[0] = to.x;
			 tol[1] = to.y;
			 tol[2] = to.z;
			 tol[3] = to.w;
		}

		double scale0, scale1;
		if (fabs(1.0-cosOmega)>tolerance)
		{
			 const double omega = acos(cosOmega);
			 const double sinOmega = sin(omega);
			 scale0 = sin((1.0-normalised_time) * omega) / sinOmega;
			 scale1 = sin(normalised_time * omega) / sinOmega;
		}
		else
		{
			 scale0 = 1.0 - normalised_time;
			 scale1 = normalised_time;
		}
		x = scale0 * from.x + scale1 * tol[0];
		y = scale0 * from.y + scale1 * tol[1]; 
		z = scale0 * from.z + scale1 * tol[2];
		w = scale0 * from.w +scale1 * tol[3];
		normalise();

		LEAVE;
 }

 int Quaternion::quaternion_to_matrix(double *values)
/*******************************************************************************
LAST MODIFIED : 18 Oct 2007

DESCRIPTION :
Convert the quaternion to a matrix, the argument values must be allocated before this function.
==============================================================================*/
 {
		int return_code;

		ENTER(Quaternion::quaternion_to_matrix);
		return_code = 0;
		if (values)
		{
			 normalise();
			 const double x2 = x + x;
			 const double y2 = y + y;
			 const double z2 = z + z;
			 const double xx = x * x2;
			 const double xy = x * y2;
			 const double xz = x * z2;
			 const double yy = y * y2;
			 const double yz = y * z2;
			 const double zz = z * z2;
			 const double wx = w * x2;
			 const double wy = w * y2;
			 const double wz = w * z2;
			 values[0] = 1 - yy - zz;
			 values[1] = xy - wz;
			 values[2] = xz+wy;
			 values[3] = 0;
			 values[4] = xy + wz;
			 values[5] = 1 - xx - zz;
			 values[6] = yz-  wx;
			 values[7] = 0;
			 values[8] = xz - wy;
			 values[9] = yz + wx;
			 values[10] = 1 - xx - yy;
			 values[11] = 0;
			 values[12] = 0;
			 values[13] = 0;
			 values[14] = 0;
			 values[15] = 1;
			 return_code = 1;
		}
		LEAVE;

		return (return_code);
 }

int Quaternion::matrix_to_quaternion(double *source, double *destination)
/*******************************************************************************
LAST MODIFIED : 22 Feb 2008

DESCRIPTION :
Convert the matrix to a quaternion, the argument values must be allocated before this function.
==============================================================================*/
 {
		int return_code;
		double trace, s, w, x, y, z;

		ENTER(Quaternion::matrix_to_quaternion);
		return_code = 0;
		trace = source[0] + source[5] + source[10] + 1;
		if (trace > 0)
		{
			 s = 0.5f /sqrtf(trace);
			 w = 0.25f / s;
			 x = (source[9] - source[6]) *s;
 			 y = (source[2] - source[8]) *s;
 			 z = (source[4] - source[1]) *s;
			 return_code = 1;
		}
		else
		{
			 if ((source[0]>source[5]) && (source[0]>source[10]))
			 {
					s = 2.0f * sqrtf(1.0f+source[0]-source[5]-source[10]);
					w = (source[6] - source[9])/s;
					x = 0.25f * s;
					y = (source[1] + source[4])/s;
					z = (source[2] + source[8])/s;
			 }
			 else if (source[5]>source[10])
			 {
					s = 2.0f * sqrtf(1.0f+source[5]-source[0]-source[10]);
					w = (source[2] - source[8])/s;
					x = (source[1] + source[4])/s;
					y = 0.25f * s;
					z = (source[6] + source[9])/s;
			 }
			 else
			 {
					s = 2.0f * sqrtf(1.0f+source[10]-source[0]-source[5]);
					w = (source[1] - source[4])/s;
					x = (source[2] + source[8])/s;
					y = (source[6] + source[9])/s;
					z = 0.25f * s;
			 }
			 return_code = 1;
		}
 		destination[0] = w;
 		destination[1] = x;
 		destination[2] = y;
 		destination[3] = z;
		LEAVE;

		return (return_code);
 }

