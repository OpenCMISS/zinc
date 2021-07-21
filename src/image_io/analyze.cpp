/*
 * OpenCMISS-Zinc Library
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sstream>
#include <fstream>
#include <stdint.h>
#include "general/debug.h"
#include "general/io_stream.h"
#include "analyze.h"
#include "opencmiss/zinc/types/streamid.h"
#include "general/image_utilities.h"
#include "general/mystring.h"
#include "general/debug.h"
#include "general/message.h"

#if defined (ZINC_USE_IMAGEMAGICK)
#  if defined _MSC_VER
   /* When using the gcc compiled headers from msvc we need to replace inline */
#    define inline __inline
#    define MAGICK_STATIC_LINK
#  endif /* defined _MSC_VER */
/* image magick interfaces */
#include "MagickCore/MagickCore.h"
#endif /* defined (ZINC_USE_IMAGEMAGICK) */

#define PRINT_ANALYZE_INFO 0

/**
 * @brief halffloat2float
 * Convert a half float value into a 4 byte float value.
 * Returns a IEEE754 nan representation if the system does
 * not represent floats in this format or if a float is not
 * 4 bytes.
 *
 * @param source 2 byte binary representation of a half float
 * @return 4 byte IEEE754 representation of half float value
 */
float halffloat2float(uint16_t source);

struct Analyze_stream
{
	char *hdr_name;
	int hdr_size;
	char *hdr_buffer;
	int img_size;
	char *img_buffer;
	char *img_name;
};

int octal_string_to_int(char *current_char, unsigned int size)
{
    unsigned int output = 0;
    while(size > 0){
        output = output * 8 + *current_char - '0';
        current_char++;
        size--;
    }
    return output;
}

void from_tar_to_analyze_stream(Analyze_stream *stream, char *buffer, int size)
{
//	tar header looks like this
//	location  size  field
//	0         100   File name
//	100       8     File mode
//	108       8     Owner's numeric user ID
//	116       8     Group's numeric user ID
//	124       12    File size in bytes
//	136       12    Last modification time in numeric Unix time format
//	148       8     Checksum for header block
//	156       1     Link indicator (file type)
//	157       100   Name of linked file
	int current_file_location = 0;
	int size_of_file = 0;
	char *current_file_name = 0;
	do
	{
		size_of_file = octal_string_to_int(
			&buffer[current_file_location+124], 11);
		current_file_name = &buffer[current_file_location];
		current_file_location += 512;
		if (strstr(current_file_name, "hdr"))
		{
			stream->hdr_name = current_file_name;
			stream->hdr_size = size_of_file;
			stream->hdr_buffer = &buffer[current_file_location];
		}
		else if (strstr(current_file_name, "img"))
		{
			stream->img_name = current_file_name;
			stream->img_size = size_of_file;
			stream->img_buffer = &buffer[current_file_location];
		}
		current_file_location = current_file_location +
			(((size_of_file - 1) / 512) + 1) * 512;
	} while (current_file_location + 512 <= size);
}

struct Cmgui_image *Cmgui_image_read_analyze(
		struct Cmgui_image_information *cmgui_image_information,
		enum cmzn_streaminformation_data_compression_type data_compression_type)
{
	struct Cmgui_image *cmgui_image = 0;
#if defined (ZINC_USE_IMAGEMAGICK)
	const char *file_name = 0, file_name_prefix[] = "aze:";
	char tmp100[100];
	char *old_magick_size, magick_size[41];
	int number_of_files = 0, width = 0, height = 0;
	Image *magick_image, *temp_magick_image;
	ImageInfo *magick_image_info;
	ExceptionInfo *magick_exception;

	if (cmgui_image_information && cmgui_image_information->valid &&
		(cmgui_image_information->file_names ||
		cmgui_image_information->memory_blocks))
	{
		cmgui_image = CREATE(Cmgui_image)();

		int return_code = 1;
		magick_exception = AcquireExceptionInfo();
		magick_image_info = CloneImageInfo((ImageInfo *) NULL);
		old_magick_size = magick_image_info->size;
		magick_image_info->size = (char *)NULL;
#if defined (DEBUG_CODE)
		magick_image_info->verbose = 1;
#endif /* defined (DEBUG_CODE) */
		if (cmgui_image_information->memory_blocks)
		{
			number_of_files = cmgui_image_information->number_of_memory_blocks;
		}
		else
		{
			number_of_files = cmgui_image_information->number_of_file_names;
		}
		for (int i = 0; (i < number_of_files) && return_code; i++)
		{
			width = cmgui_image_information->width;
			height = cmgui_image_information->height;
			if (cmgui_image_information->file_names)
			{
				file_name = cmgui_image_information->file_names[i];
			}
			else
			{
				/* Just use a dummy file_name */
				file_name = "memory";
			}
			AnalyzeStreamsType streamstype = ANALYZE_STREAMS_TYPE_FILE;
			Analyze_stream analyze_stream;
			analyze_stream.hdr_size = 0;
			analyze_stream.hdr_buffer = 0;
			analyze_stream.hdr_name = 0;
			analyze_stream.img_size = 0;
			analyze_stream.img_buffer = 0;
			analyze_stream.img_name = 0;
			char *uncompressed_bytes = 0;
			int uncompressed_length = 0;
			if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_GZIP ||
				data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_BZIP2)
			{
				if (cmgui_image_information->memory_blocks &&
					cmgui_image_information->memory_blocks[i])
				{
					if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_GZIP)
						uncompressed_length = open_gzip_stream(cmgui_image_information->memory_blocks[i]->buffer,
							cmgui_image_information->memory_blocks[i]->length, &uncompressed_bytes);
					else if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_BZIP2)
						uncompressed_length = open_bzip2_stream(cmgui_image_information->memory_blocks[i]->buffer,
							cmgui_image_information->memory_blocks[i]->length, &uncompressed_bytes);
				}
				else if (file_name)
				{
					std::ifstream analyzeFileStream(file_name, std::ifstream::binary);
					if (analyzeFileStream.is_open())
					{
						analyzeFileStream.seekg (0, analyzeFileStream.end);
						int fileSize = analyzeFileStream.tellg();
						char *memblock = new char [fileSize];
						analyzeFileStream.seekg (0, analyzeFileStream.beg);
						analyzeFileStream.read (memblock, fileSize);
						analyzeFileStream.close();
						if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_GZIP)
							uncompressed_length = open_gzip_stream(memblock, fileSize, &uncompressed_bytes);
						else if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_BZIP2)
							uncompressed_length = open_bzip2_stream(memblock, fileSize, &uncompressed_bytes);
						delete[] memblock;
					}
				}
				if (uncompressed_length > 0 && uncompressed_bytes)
				{
					from_tar_to_analyze_stream(&analyze_stream,
						uncompressed_bytes, uncompressed_length);
					file_name = analyze_stream.hdr_name;
					if (analyze_stream.img_buffer && analyze_stream.hdr_buffer)
					{
						streamstype = ANALYZE_STREAMS_TYPE_MEMORY;
					}
				}
			}

			AnalyzeImageHandler analyze = AnalyzeImageHandler(streamstype);
			if ((streamstype == ANALYZE_STREAMS_TYPE_FILE && analyze.setFilename(file_name) &&
				analyze.readHeader()) ||
				(streamstype == ANALYZE_STREAMS_TYPE_MEMORY && analyze_stream.hdr_buffer &&
					analyze.readHeader(analyze_stream.hdr_buffer)))
			{
				int numberOfDimensions = analyze.getNumberOfDimensions();
				if (numberOfDimensions == 3)
				{
					width = analyze.getWidth();
					height = analyze.getHeight();
					sprintf(magick_image_info->filename, "%s%s",
						file_name_prefix, file_name);
					size_t filename_len = strlen(magick_image_info->filename);
					// We have to change the file suffix of the Analyze header file
					// because 'hdr' is already known to ImageMagick.
					magick_image_info->filename[filename_len-3] = 'a';
					magick_image_info->filename[filename_len-2] = 'z';
					magick_image_info->filename[filename_len-1] = 'e';
					magick_image_info->orientation = static_cast<OrientationType>(analyze.getOrientation() + 1);
					if ((0 < width) && (0 < height))
					{
						sprintf(magick_size, "%dx%d", width, height);
						magick_image_info->size = magick_size;
					}
					int number_of_components = analyze.getNumberOfComponents();
					int number_of_bits_per_component = analyze.getComponentDepth();
					magick_image_info->colorspace = GRAYColorspace;
					if (3 == number_of_components)
					{
						magick_image_info->colorspace = RGBColorspace;
					}
					if (number_of_bits_per_component == 8 ||
						number_of_bits_per_component == 16 ||
						number_of_bits_per_component == 32)
					{
						if (streamstype == ANALYZE_STREAMS_TYPE_FILE)
							analyze.readImageData();
						else if (streamstype == ANALYZE_STREAMS_TYPE_MEMORY)
							analyze.readImageData(analyze_stream.img_buffer,
								analyze_stream.img_size);

						snprintf(tmp100, 99, "%d", analyze.getGlMax());
						SetImageOption(magick_image_info, "quantum:maximum", tmp100);
						snprintf(tmp100, 99, "%d", analyze.getGlMin());
						SetImageOption(magick_image_info, "quantum:minimum", tmp100);
						snprintf(tmp100, 99, "%s", analyze.getQuantumFormat());
						SetImageOption(magick_image_info, "quantum:format", tmp100);
						magick_image_info->depth = number_of_bits_per_component;
						magick_image_info->type = GrayscaleType;
						if (analyze.isBigEndian())
						{
							magick_image_info->endian = MSBEndian;
						}
						else
						{
							magick_image_info->endian = LSBEndian;
						}
						for (int i = 0; (i < analyze.getDepth()) && return_code; i++)
						{
							struct Cmgui_image_information_memory_block memory_block = analyze.getImageMemoryBlock(i);

							magick_image = BlobToImage(magick_image_info,
								memory_block.buffer,
								memory_block.length,
								magick_exception);

							if (magick_image)
							{
								if (cmgui_image->magick_image)
								{
									/* add image to the end of the doubly linked-list of images */
									temp_magick_image = cmgui_image->magick_image;
									while (temp_magick_image->next)
									{
										temp_magick_image = temp_magick_image->next;
									}
									temp_magick_image->next = magick_image;
									magick_image->previous = temp_magick_image;
								}
								else
								{
									cmgui_image->magick_image = magick_image;
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"Could not read image: %s\nYou may need to add a prefix indicating the file format.", file_name);
								return_code = 0;
							}
						}
					}
					else
					{
						return_code = 0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "Analyze image handler not able to handle %d dimensions", numberOfDimensions);
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "Analyze image handler not able to open file '%s'", file_name);
				return_code = 0;
			}
			if (uncompressed_bytes)
				DEALLOCATE(uncompressed_bytes);
		}
		magick_image_info->size = (char *)NULL;
		/* restore original size for ImageMagick to clean up */
		magick_image_info->size = old_magick_size;
		if (return_code && cmgui_image->magick_image)
		{
			cmgui_image->number_of_images =
				get_magick_image_number_of_consistent_images(
					cmgui_image->magick_image);
			if (0 < cmgui_image->number_of_images)
			{
				get_magick_image_parameters(cmgui_image->magick_image,
					&cmgui_image->width, &cmgui_image->height,
					&cmgui_image->number_of_components,
					&cmgui_image->number_of_bytes_per_component, 1);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Cmgui_image_read.  Image size not constant over series");
				return_code = 0;
			}
		}
		DestroyImageInfo(magick_image_info);
		DestroyExceptionInfo(magick_exception);
		if (!return_code)
		{
			DESTROY(Cmgui_image)(&cmgui_image);
			cmgui_image = 0;
		}
	}
#else
	display_message(ERROR_MESSAGE, "Analyze image handler not implemented for this version.");
	USE_PARAMETER(cmgui_image_information);
#endif /* defined (ZINC_USE_IMAGEMAGICK) */

	return cmgui_image;
}

enum EndianEnum systemEndianTest()
{
	unsigned char swapTest[2] = { 1, 0 };
	if (*(short *) swapTest == 1 )
	{
		return EndianLittle;
	}

	return EndianBig;
}

AnalyzeImageHandler::AnalyzeImageHandler(enum AnalyzeStreamsType streamsTypeIn)
	: filename(0)
	, bigEndian(false)
	, data(0)
	, streamsType(streamsTypeIn)
{
}

AnalyzeImageHandler::~AnalyzeImageHandler()
{
	if (filename)
		DEALLOCATE(filename);

	if (data && streamsType != ANALYZE_STREAMS_TYPE_MEMORY)
		DEALLOCATE(data);
}

bool AnalyzeImageHandler::setFilename(const char *filenameIn)
{
	return (0 != (filename = duplicate_string(filenameIn)));
}

bool AnalyzeImageHandler::testFileEndianSystemEndianMatch()
{
	bool match = false;
	//enum EndianEnum systemOrder = systemEndianTest();
	if ((hdr.hk.sizeof_hdr == 348) || (hdr.hk.extents == 16384))
	{
		match = true;
	}

	return match;
}

void SwapRange2(void *void_p, BufferSizeType n)
{
	uint16_t *p = (uint16_t *)void_p;
	uint16_t h1, h2;
	BufferSizeType i;

	for (i = 0; i < n; i++)
	{
		h1 = (*p) & 0xff;
		h2 = ((*p) >> 8) & 0xff;
		*p = (h1 << 8) | h2;

		p = p + 1;
	}
}

void SwapRange4(void *void_p, BufferSizeType n)
{
	uint32_t *p = (uint32_t *)void_p;
	uint32_t h1, h2, h3, h4;
	BufferSizeType i;

	for (i = 0; i < n; i++)
	{
		h1 = (*p) & 0xff;
		h2 = ((*p) >> 8) & 0xff;
		h3 = ((*p) >> 16) & 0xff;
		h4 = ((*p) >> 24) & 0xff;
		*p = (h1 << 24) | (h2 << 16) | (h3 << 8) | h4;

		p = p + 1;
	}

}

void SwapRange8(void *void_p, BufferSizeType n)
{
	uint64_t *p = (uint64_t *)void_p;
	uint64_t h1, h2, h3, h4, h5, h6, h7, h8;
	BufferSizeType i;

	for (i = 0; i < n; i++)
	{
		h1 = (*p) & 0xff;
		h2 = ((*p) >> 8) & 0xff;
		h3 = ((*p) >> 16) & 0xff;
		h4 = ((*p) >> 24) & 0xff;
		h5 = ((*p) >> 32) & 0xff;
		h6 = ((*p) >> 40) & 0xff;
		h7 = ((*p) >> 48) & 0xff;
		h8 = ((*p) >> 56) & 0xff;
		*p = (h1 << 56) | (h2 << 48) | (h3 << 40) | (h4 << 32) | (h5 << 24) | (h6 << 16) | (h7 << 8) | h8;

		p = p + 1;
	}

}

template <class myType>
void ByteSwap(myType *p)
{
	switch (sizeof(myType))
	{
	case 2:
		SwapRange2((void *)p, 1);
		break;
	case 4:
		SwapRange4((void *)p, 1);
		break;
	case 8:
		SwapRange8((void *)p, 1);
		break;
	default:
		printf("No can do!\n");
	}
}

template <class myType>
void ByteSwapRange(myType *p, BufferSizeType n)
{
	switch (sizeof(myType))
	{
	case 2:
		SwapRange2((void *)p, n);
		break;
	case 4:
		SwapRange4((void *)p, n);
		break;
	case 8:
		SwapRange8((void *)p, n);
		break;
	default:
		printf("No can do!\n");
	}
}

void AnalyzeImageHandler::swapBytesIfEndianessDifferent()
{
	if (!testFileEndianSystemEndianMatch())
	{
		ByteSwap<int>(&hdr.hk.sizeof_hdr);
		ByteSwap<int>(&hdr.hk.extents);
		ByteSwap<short int>(&hdr.hk.session_error);

		ByteSwapRange<short int>(&hdr.dime.dim[0], 8);
		ByteSwap<short int>(&hdr.dime.unused1);
		ByteSwap<short int>(&hdr.dime.datatype);
		ByteSwap<short int>(&hdr.dime.bitpix);
		ByteSwap<short int>(&hdr.dime.dim_un0);
		ByteSwapRange<float>(&hdr.dime.pixdim[0], 8);
		ByteSwap<float>(&hdr.dime.vox_offset);
		ByteSwap<float>(&hdr.dime.roi_scale);
		ByteSwap<float>(&hdr.dime.funused1);
		ByteSwap<float>(&hdr.dime.funused2);
		ByteSwap<float>(&hdr.dime.cal_max);
		ByteSwap<float>(&hdr.dime.cal_min);
		ByteSwap<int>(&hdr.dime.compressed);
		ByteSwap<int>(&hdr.dime.verified);
		ByteSwap<int>(&hdr.dime.glmax);
		ByteSwap<int>(&hdr.dime.glmin);

		ByteSwap<int>(&hdr.hist.views);
		ByteSwap<int>(&hdr.hist.vols_added);
		ByteSwap<int>(&hdr.hist.start_field);
		ByteSwap<int>(&hdr.hist.field_skip);
		ByteSwap<int>(&hdr.hist.omax);
		ByteSwap<int>(&hdr.hist.omin);
		ByteSwap<int>(&hdr.hist.smax);
		ByteSwap<int>(&hdr.hist.smin);
	}
}

int AnalyzeImageHandler::determineDimensions()
{
	numberOfDimensions = 0;
	if (hdr.dime.dim[1] > 1)
	{
		numberOfDimensions = 1;
		if (hdr.dime.dim[2] > 1)
		{
			numberOfDimensions = 2;
			if (hdr.dime.dim[3] > 1)
			{
				numberOfDimensions = 3;
				if (hdr.dime.dim[4] > 1)
				{
					numberOfDimensions = 4;
				}
			}
		}
	}
	return numberOfDimensions;
}

int AnalyzeImageHandler::getNumberOfDimensions()
{
	return numberOfDimensions;
}

int AnalyzeImageHandler::getHeight()
{
	return hdr.dime.dim[1];
}

int AnalyzeImageHandler::getWidth()
{
	return hdr.dime.dim[2];
}

int AnalyzeImageHandler::getDepth()
{
	return hdr.dime.dim[3];
}

int AnalyzeImageHandler::getComponentDepth()
{
	int depth = 0;
	switch (hdr.dime.datatype)
	{
	case ANALYZE_DT_BINARY:
		depth = 1;
		break;
	case ANALYZE_DT_UNSIGNED_CHAR:
		depth = 8;
		break;
	case ANALYZE_DT_SIGNED_SHORT:
		depth = 16;
		break;
	case ANALYZE_DT_RGB:
		depth = 24;
		break;
	case ANALYZE_DT_SIGNED_INT:
	case ANALYZE_DT_FLOAT:
		depth = 32;
		break;
	case ANALYZE_DT_COMPLEX:
	case ANALYZE_DT_DOUBLE:
		depth = 64;
		break;
	default:
		depth = 0;
	}

	return depth; //hdr.dime.bitpix - Cannot be relied upon to give the correct value.
}

int AnalyzeImageHandler::getNumberOfComponents()
{
	int numberOfComponents = 1;
	if (hdr.dime.datatype == ANALYZE_DT_RGB)
		numberOfComponents = 3;

	return numberOfComponents;
}

struct Cmgui_image_information_memory_block AnalyzeImageHandler::getImageMemoryBlock(int index)
{
	int block_length = getHeight()*getWidth()*getComponentDepth()/8;
	struct Cmgui_image_information_memory_block ii;
	ii.buffer = (char *)data + index * block_length;
	ii.length = block_length;
	ii.memory_block_is_imagemagick_blob = 0;

	return ii;
}

int AnalyzeImageHandler::getGlMax() const
{
	return hdr.dime.glmax;
}

int AnalyzeImageHandler::getGlMin() const
{
	return hdr.dime.glmin;
}

const char * AnalyzeImageHandler::getQuantumFormat() const
{
	const char * format = 0; // Undefined
	switch (hdr.dime.datatype)
	{
	case ANALYZE_DT_FLOAT:
	case ANALYZE_DT_COMPLEX:
	case ANALYZE_DT_DOUBLE:
		format = "FloatingPoint";
		break;
	case ANALYZE_DT_SIGNED_SHORT:
	case ANALYZE_DT_SIGNED_INT:
		format = "Signed";
		break;
	case ANALYZE_DT_BINARY:
	case ANALYZE_DT_UNSIGNED_CHAR:
	case ANALYZE_DT_RGB:
		format = "Unsigned";
		break;
	default:
		format = "Undefined";
	}
	return format;
}

void AnalyzeImageHandler::setFileEndian()
{
	if (testFileEndianSystemEndianMatch())
		bigEndian = (systemEndianTest() == EndianBig);
	else
		bigEndian = (systemEndianTest() != EndianBig);
}

void AnalyzeImageHandler::readImageInternal(unsigned int sz)
{
	if (hdr.dime.glmax == 0.0 && hdr.dime.glmin == 0.0)
	{
		switch (hdr.dime.datatype)
		{
			case ANALYZE_DT_FLOAT:
			{
				int16_t *p = (int16_t *)data;
				float value;
				int max = -65530, min = 65530;
				for (size_t i = 0; i < sz; i++)
				{
					value = halffloat2float(*p);
					if (value < min)
						min = (int)((value) - 0.5);
					if (value > max)
						max = (int)((value) + 0.5);
					p++;
				}
				hdr.dime.glmax = max;
				hdr.dime.glmin = min;
			} break;
			case ANALYZE_DT_SIGNED_SHORT:
			{
				short int *p = (short int *)data;
				int max = -65530, min = 65530;
				for (size_t i = 0; i < sz; i++)
				{
					if (*p < min)
						min = *p;
					if (*p > max)
						max = *p;
					p++;
				}
				hdr.dime.glmax = max;
				hdr.dime.glmin = min;
			} break;
			case ANALYZE_DT_SIGNED_INT:
			{
				hdr.dime.glmax = 2147483647;
				hdr.dime.glmin = -2147483647;
			} break;
			case ANALYZE_DT_BINARY:
			{
				hdr.dime.glmax = 1;
				hdr.dime.glmin = 0;
			} break;
			case ANALYZE_DT_UNSIGNED_CHAR:
			case ANALYZE_DT_RGB:
			{
				hdr.dime.glmax = 255;
				hdr.dime.glmin = 0;
			} break;
			case ANALYZE_DT_COMPLEX:
			case ANALYZE_DT_DOUBLE:
			default:
			{
				display_message(ERROR_MESSAGE, "Not handling this case of Analyze image format. Please implement");
			}
		}
#if PRINT_ANALYZE_INFO
		printf("setting glmax, glmin = [%d, %d]\n", hdr.dime.glmax, hdr.dime.glmin);
#endif
	}
}

void AnalyzeImageHandler::readImageData(void *imgBuffer, int buffer_length)
{
	if (data && streamsType != ANALYZE_STREAMS_TYPE_MEMORY)
		DEALLOCATE(data);
	data = imgBuffer;
	int bytes = getComponentDepth()/4;
	size_t sz = buffer_length / bytes;
	readImageInternal(static_cast<unsigned int>(sz));
}

void AnalyzeImageHandler::readImageData()
{
	char *data_filename = duplicate_string(filename);
	size_t len = strlen(data_filename);
	data_filename[len-3] = 'i';
	data_filename[len-2] = 'm';
	data_filename[len-1] = 'g';
#if PRINT_ANALYZE_INFO
	printf("filename = %s\n", data_filename);
#endif
	FILE *file = fopen(data_filename, "rb");
	fseek(file, 0L, SEEK_END);
	size_t sz = ftell(file);
	//You can then seek back to the beginning:

	fseek(file, 0L, SEEK_SET);
#if PRINT_ANALYZE_INFO
	printf("image data size = %ld\n", sz);
#endif
	if (data && streamsType != ANALYZE_STREAMS_TYPE_MEMORY)
		DEALLOCATE(data);

	int bytes = getComponentDepth()/4;
	ALLOCATE(data, char, bytes*sz);
	fread(data, sizeof(char), bytes*sz, file);
	readImageInternal(static_cast<unsigned int>(sz));
}

int AnalyzeImageHandler::getOrientation() const
{
	int orientation = hdr.hist.orient;
	if (orientation > 2 || orientation < 0)
	{
		orientation = -1;
	}

	return orientation;
}

void AnalyzeImageHandler::readHeaderInternal()
{
	setFileEndian(); // Set this before swapping bytes.
	swapBytesIfEndianessDifferent();
#if PRINT_ANALYZE_INFO
	printf("right now - %d\n", hdr.hk.sizeof_hdr);
	printf("right now - %d\n", hdr.hk.extents);
	printf("dimensions = [%d, %d, %d, %d, %d]\n", hdr.dime.dim[0], hdr.dime.dim[1], hdr.dime.dim[2], hdr.dime.dim[3], hdr.dime.dim[4]);
#endif
	determineDimensions();
#if PRINT_ANALYZE_INFO
	printf("type = %s [%d, %d]\n", hdr.hk.data_type, hdr.dime.datatype, hdr.dime.bitpix);
	printf("voxel offset = %f\n", hdr.dime.vox_offset);
	printf("glmax, glmin = [%d, %d]\n", hdr.dime.glmax, hdr.dime.glmin);
	printf("calmax, calmin = [%.2f, %.2f]\n", hdr.dime.cal_max, hdr.dime.cal_min);
	printf("omax, omin, smax, smin = [%d, %d], [%d, %d]\n", hdr.hist.omax, hdr.hist.omin, hdr.hist.smax, hdr.hist.smin);
	printf("orient = %d\n", hdr.hist.orient);
#endif
}

bool AnalyzeImageHandler::readHeader(void *hdrbuffer)
{
	bool success = false;
	if (hdrbuffer)
	{
		success = true;
		memcpy(&hdr, hdrbuffer, sizeof(hdr));
		readHeaderInternal();
	}
	return success;
}

bool AnalyzeImageHandler::readHeader()
{
	bool success = false;
	FILE *file = fopen(filename, "rb");
	if (file)
	{
		success = true;
		fread(&hdr, sizeof(hdr), 1, file);
		fclose(file);
		readHeaderInternal();
	}

	return success;
}

union floatbitconvert
{
	float f;
	uint32_t u;
};

float halffloat2float(uint16_t input)
{
	floatbitconvert myresult, nan;
	myresult.u = nan.u = 0xFFC00000u;

	uint32_t source = input;
	uint16_t hs, he, hm;
	uint32_t xs, xe, xm;
	int32_t xes;
	int e;
	static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
	float one = 1.0; // Used for checking IEEE754 floating point format, we are of course assuming that a float is 4 bytes!!
	uint32_t *ip = 0; // Used for checking IEEE754 floating point format

	if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
		ip = (uint32_t *) &one;
		if (systemEndianTest() != EndianBig)
		{
			ByteSwap<uint32_t>(ip);
		}

		if( (*ip) != (uint32_t)0x3F800000u ) { // Check for exact IEEE 754 bit pattern of 1.0
			return nan.f;  // Floating point bit pattern is not IEEE 754
		}

		if (sizeof(float) != 4) { // float is not 4 bytes
			return nan.f;
		}

		checkieee = 0; // Everything checks out OK
	}

	if( (input & 0x7FFFu) == 0 ) {  // Signed zero
		myresult.u = source << 16;
	} else { // Not zero
		hs = source & 0x8000u;  // Pick off sign bit
		he = source & 0x7C00u;  // Pick off exponent bits
		hm = source & 0x03FFu;  // Pick off mantissa bits
		if( he == 0 ) {  // Denormal will convert to normalized
			e = -1; // The following loop figures out how much extra to adjust the exponent
			do {
				e++;
				hm <<= 1;
			} while( (hm & 0x0400u) == 0 ); // Shift until leading bit overflows into exponent bit
			xs = ((uint32_t) hs) << 16; // Sign bit
			xes = ((int32_t) (he >> 10)) - 15 + 127 - e; // Exponent unbias the halfp, then bias the single
			xe = (uint32_t) (xes << 23); // Exponent
			xm = ((uint32_t) (hm & 0x03FFu)) << 13; // Mantissa
			myresult.u = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
		} else if( he == 0x7C00u ) {  // Inf or NaN (all the exponent bits are set)
			if( hm == 0 ) { // If mantissa is zero ...
				myresult.u = (((uint32_t) hs) << 16) | ((uint32_t) 0x7F800000u); // Signed Inf
			} else {
				myresult.u = 0xFFC00000u; // NaN, only 1st mantissa bit set
			}
		} else { // Normalized number
			xs = ((uint32_t) hs) << 16; // Sign bit
			xes = ((int32_t) (he >> 10)) - 15 + 127; // Exponent unbias the halfp, then bias the single
			xe = (uint32_t) (xes << 23); // Exponent
			xm = ((uint32_t) hm) << 13; // Mantissa
			myresult.u = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
		}
	}

	return myresult.f;
}
