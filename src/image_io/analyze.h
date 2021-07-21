/*
 * OpenCMISS-Zinc Library
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IMAGE_IO_ANALYZE_H_
#define IMAGE_IO_ANALYZE_H_

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/types/streamid.h"
#include "analyze_header.h"

struct Cmgui_image_information;
struct Cmgui_image;

enum AnalyzeStreamsType
{
	ANALYZE_STREAMS_TYPE_INVALID= 0,
	ANALYZE_STREAMS_TYPE_FILE = 1,
	ANALYZE_STREAMS_TYPE_MEMORY = 2,
};

class AnalyzeImageHandler
{
public:
	explicit AnalyzeImageHandler(enum AnalyzeStreamsType);
	~AnalyzeImageHandler();

	bool readHeader();
	bool readHeader(void *hdrBuffer);
	void readImageData();
	void readImageData(void *imgBuffer, int buffer_length);
	int getNumberOfDimensions();
	int getWidth();
	int getHeight();
	int getDepth();
	int getComponentDepth();
	int getNumberOfComponents();
	struct Cmgui_image_information_memory_block getImageMemoryBlock(int index);
	int getGlMax() const;
	int getGlMin() const;
	const char *getQuantumFormat() const;
	bool isBigEndian() const { return bigEndian; }
	bool setFilename(const char *filenameIn);

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
	void readHeaderInternal();
	void readImageInternal(unsigned int sz);

	const char *filename;
	bool bigEndian;
	int numberOfDimensions;
	int width;
	int height;
	struct dsr hdr;
	void *data;
	enum AnalyzeStreamsType streamsType;
};

/**
 * Creates a Cmgui_image containing the images listed in the image information.
 * Specific function for the Analyze format.
 *
 * @param cmgui_image_information
 * @return A Cmgui_image structure if successful, 0 otherwise.
 */
struct Cmgui_image *Cmgui_image_read_analyze(
	struct Cmgui_image_information *cmgui_image_information,
	enum cmzn_streaminformation_data_compression_type data_compression_type);

typedef ::size_t      BufferSizeType;

void SwapRange2(void *void_p, BufferSizeType n);
void SwapRange4(void *void_p, BufferSizeType n);
void SwapRange8(void *void_p, BufferSizeType n);

enum EndianEnum
{
	EndianBig,
	EndianLittle
};

enum EndianEnum systemEndianTest();

#endif /* IMAGE_IO_ANALYZE_H_ */
