/**
 * FILE : write_fieldml.hpp
 * 
 * FieldML 0.5 model writer.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_WRITE_FIELDML_HPP)
#define CMZN_WRITE_FIELDML_HPP

struct cmzn_region;

/**
 * Write model in region in FieldML 0.5 format.
 */
int write_fieldml_file(struct cmzn_region *region, const char *pathandfilename);

#endif /* !defined (CMZN_WRITE_FIELDML_HPP) */
