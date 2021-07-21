/**
 * FILE : import_finite_element.h
 *
 * Functions for importing finite element data from EX file format.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (IMPORT_FINITE_ELEMENT_H)
#define IMPORT_FINITE_ELEMENT_H

#include <stdio.h>
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_region.h"
#include "general/io_stream.h"
#include "region/cmiss_region.hpp"
#include "opencmiss/zinc/types/streamid.h"

/*
Global types
------------
*/

struct FE_import_time_index
/*******************************************************************************
LAST MODIFIED : 29 October 2002

DESCRIPTION :
Points to a specific time value which the nodes represent
==============================================================================*/
{
	FE_value time;
}; /* FE_import_time_index */

/*
Global functions
----------------
*/

/***************************************************************************//**
 * Reads finite element groups in exnode/exelem format. Groups in the file are
 * created as child group regions of <region>, sharing its definitions of
 * fields, nodes and elements.
 * It is good practice to read the file into a newly created region and check it
 * can be merged into the global region before doing so, otherwise failure to
 * merge incompatible data will leave the global region in a compromised state.
 * 
 * Where objects not within the file are referred to, such as nodes in a pure
 * exelem file or elements in embedded element:xi fields, local objects of the
 * correct type are made as placeholders and all checking is left to the merge.
 * Embedding elements are located by a region path starting at the root region
 * in the file; if no path is supplied they are placed in the root region. If
 * objects are repeated in the file, they are merged correctly.

 * @param region the region which field data and groups are read into.
 * @param input_file the input stream from which data is read.
 * @param time_index if non-NULL then the values in this read are assumed
 *        to belong to the specified time.  This means that the nodal values
 *        will be read into an array and the correct index put into the
 *        corresponding time array.
 */
int read_exregion_file(struct cmzn_region *region,
	struct IO_stream *input_file, struct FE_import_time_index *time_index);

/*****************************************************************************//**
 * Version of read_exregion_file that reads nodes as data points. 
 */
int read_exdata_file(struct cmzn_region *region,
	struct IO_stream *input_file, struct FE_import_time_index *time_index);

/*****************************************************************************//**
 * Version of read_exregion_file that opens and closes file <file_name>.
 */
int read_exregion_file_of_name(struct cmzn_region *region, const char *file_name,
	struct IO_stream_package *io_stream_package,
	struct FE_import_time_index *time_index, int useData,
	enum cmzn_streaminformation_data_compression_type data_compression_type);

#endif /* !defined (IMPORT_FINITE_ELEMENT_H) */
