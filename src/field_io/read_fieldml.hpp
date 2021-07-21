/**
 * FILE : read_fieldml.hpp
 * 
 * FieldML 0.5 model reader.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_READ_FIELDML_HPP)
#define CMZN_READ_FIELDML_HPP

struct cmzn_region;

/**
 * Determines whether the named file is FieldML format by the quick and
 * dirty method of finding a <Fieldml> tag near the beginning.
 * @return  Boolean true if FieldML format, false if not.
 */
bool is_FieldML_file(const char *filename);

/**
 * Determines whether the memory buffer is in FieldML format by
 * finding a <Fieldml> tag near the beginning.
 * @return  Boolean true if FieldML format, false if not.
 */
bool is_FieldML_memory_block(unsigned int memory_buffer_size, const void *memory_buffer);

/**
 * Reads model in FieldML format from the named file.
 */
int parse_fieldml_file(struct cmzn_region *region, const char *filename);

#endif /* !defined (CMZN_READ_FIELDML_HPP) */
