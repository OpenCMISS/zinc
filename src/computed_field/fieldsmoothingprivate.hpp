/**
 * FILE : fieldsmoothingprivate.hpp
 * 
 * Implementation of field smoothing class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_FIELDSMOOTHINGPRIVATE_HPP)
#define CMZN_FIELDSMOOTHINGPRIVATE_HPP

#include "opencmiss/zinc/fieldsmoothing.h"
#include "opencmiss/zinc/status.h"

struct cmzn_fieldsmoothing
{
private:
	cmzn_fieldsmoothing_algorithm algorithm;
	double time;
	int access_count;

public:

	cmzn_fieldsmoothing(cmzn_fieldmodule_id fieldmodule);

	~cmzn_fieldsmoothing();

	cmzn_fieldsmoothing_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_fieldsmoothing_id &fieldsmoothing)
	{
		if (!fieldsmoothing)
			return CMZN_ERROR_ARGUMENT;
		--(fieldsmoothing->access_count);
		if (fieldsmoothing->access_count <= 0)
			delete fieldsmoothing;
		fieldsmoothing = 0;
		return CMZN_OK;
	}

	cmzn_fieldsmoothing_algorithm getAlgorithm() const
	{
		return this->algorithm;
	}

	int setAlgorithm(cmzn_fieldsmoothing_algorithm algorithmIn)
	{
		if (algorithmIn == CMZN_FIELDSMOOTHING_ALGORITHM_INVALID)
			return CMZN_ERROR_ARGUMENT;
		this->algorithm = algorithmIn;
		return CMZN_OK;
	}

	double getTime() const
	{
		return this->time;
	}

	void setTime(double timeIn)
	{
		this->time = timeIn;
	}
};

#endif /* !defined (CMZN_FIELDSMOOTHINGPRIVATE_HPP) */
