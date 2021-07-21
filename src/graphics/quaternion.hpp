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

#if !defined (QUATERNION_HPP)
#define QUATERNION_HPP

class Quaternion
{

public:

	 Quaternion(const double quat_w, const double quat_x, const double quat_y, const double quat_z)
	 {
			set(quat_w, quat_x, quat_y, quat_z);
	 };

	 Quaternion()
	 {
			set(1, 0, 0, 0);
	 };

	 ~Quaternion()
	 {
	 }

	 void set(const double quat_w, const double quat_x, const double quat_y, const double quat_z);

	 void get(double *values) const;

	 void normalise();

	 void interpolated_with_SLERP(const Quaternion &from, const Quaternion &to, const double normalised_time);

	 int quaternion_to_matrix(double *values);

	 static int matrix_to_quaternion(double *source, double *destination);

private:
	 
	 double w, x, y, z;

};
#endif /* !defined (QUATERNION_HPP) */

