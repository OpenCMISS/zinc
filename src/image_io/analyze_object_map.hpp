/*
 * OpenCMISS-Zinc Library
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ANALYZE_OBJECT_MAP_HPP_
#define ANALYZE_OBJECT_MAP_HPP_

#include <istream>
#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/types/streamid.h"
#include "analyze_header.h"
#include "analyze.h"

struct Cmgui_image_information;
struct Cmgui_image;

const int        OBJ_VERSION1 = 880102;
const int        OBJ_VERSION2 = 880801;
const int        OBJ_VERSION3 = 890102;
static const int OBJ_VERSION4 = 900302;
static const int OBJ_VERSION5 = 910402;
static const int OBJ_VERSION6 = 910926;
static const int OBJ_VERSION7 = 20050829;

class AnalyzeObjectEntry
{
public:
	explicit AnalyzeObjectEntry();
	~AnalyzeObjectEntry();

	void readFromFilePointer(std::istream &inputStream, const bool byteSwap, const bool blendFactor);

	void printInformation();

	void setVoxelsCount(int valueIn);

private:
	char name[33];              /*bytes   0-31*/
	int display_flag;           /*bytes  32-35*/
	unsigned char copy_flag;              /*bytes  36-36*/
	unsigned char mirror;            /*bytes  37-37*/
	unsigned char status;            /*bytes  38-38*/
	unsigned char neighbors_used;     /*bytes  39-39*/
	int shades;                /*bytes  40-43*/
	int start_red;              /*bytes  44-47*/
	int start_green;            /*bytes  48-51*/
	int start_blue;             /*bytes  52-55*/
	int end_red;                /*bytes  53-58*/
	int end_green;              /*bytes  59-62*/
	int end_blue;               /*bytes  63-66*/
	int x_rot;             /*bytes  67-70*/
	int y_rot;             /*bytes  71-74*/
	int z_rot;             /*bytes  75-78*/
	int x_shift;          /*bytes  79-82*/
	int y_shift;          /*bytes  83-86*/
	int z_shift;          /*bytes  87-90*/
	int x_center;               /*bytes  91-94*/
	int y_center;               /*bytes  95-98*/
	int z_center;               /*bytes  99-102*/
	int x_rot_increment;    /*bytes  103-106*/
	int y_rot_increment;    /*bytes  107-110*/
	int z_rot_increment;    /*bytes  111-114*/
	int x_shift_increment; /*bytes  115-118*/
	int y_shift_increment; /*bytes  119-121*/
	int z_shift_increment; /*bytes  122-125*/
	short int  min_x;         /*bytes  126-127*/
	short int  min_y;         /*bytes  128-129*/
	short int  min_z;         /*bytes  130-131*/
	short int  max_x;         /*bytes  132-133*/
	short int  max_y;         /*bytes  134-135*/
	short int  max_z;         /*bytes  136-137*/
	float opacity;               /*bytes  138-141*/
	int   opacity_thick;      /*bytes  142-145*/
	float blend_factor;           /*bytes  146-149*/
	unsigned int voxels_count;

	template <typename myType>
	void readBytes(std::istream &inputStream, myType *dest, const int count, const bool byteSwap);
};

class AnalyzeObjectMapHandler
{
public:
	explicit AnalyzeObjectMapHandler(enum AnalyzeStreamsType);
	~AnalyzeObjectMapHandler();

	bool readImageInformation();
	bool readImageInformation(void *buffer, int buffer_size);
	void readImageData();
	void readImageData(void *buffer);
	int getNumberOfDimensions();
	int getWidth();
	int getHeight();
	int getDepth();
	int getNumberOfObjects();
	int getComponentDepth();
	int getNumberOfComponents();
	struct Cmgui_image_information_memory_block getImageMemoryBlock(int index);
	int getGlMax() const;
	int getGlMin() const;
	const char *getQuantumFormat() const;
	bool isBigEndian() const { return bigEndian; }
	bool setFilename(const char *filenameIn);
	void printInformation();
	AnalyzeObjectEntry *getObjectEntry(int index);

	/**
	 * @brief getOrientation
	 * Get the orientation of the data as an integer:
	 *  0 for transverse
	 *  1 for coronal
	 *  2 for sagittal
	 *  -1 for any other
	 *
	 * @return orientation value as an int
	 */
	int getOrientation() const;

private:
	void setFileEndian();
	int determineDimensions();
	bool testFileEndianSystemEndianMatch();
	void swapBytesIfEndianessDifferent();
	bool readImageInformationInternal(void *buffer, int buffer_size);
	void readImageInternal(unsigned int sz);

	const char *filename;
	bool bigEndian;
	int numberOfDimensions;
	int width;
	int height;
	int depth;
	void *data;
	int numberOfObjects;
	AnalyzeObjectEntry **entriesArray;
	int version;
	bool readImage(std::istream &inputStream);

	enum AnalyzeStreamsType streamsType;
};

/**
 * Creates a Cmgui_image containing the images listed in the image information.
 * Specific function for the Analyze format.
 *
 * @param cmgui_image_information
 * @return A Cmgui_image structure if successful, 0 otherwise.
 */
struct Cmgui_image *Cmgui_image_read_analyze_object_map(
	struct Cmgui_image_information *cmgui_image_information,
	enum cmzn_streaminformation_data_compression_type data_compression_type);

#endif /* ANALYZE_OBJECT_MAP_HPP_ */
