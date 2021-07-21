#include <sstream>
#include <fstream>
#include <streambuf>
#include <istream>
#include <stdint.h>
#include "analyze.h"
#include "general/debug.h"
#include "general/io_stream.h"
#include "analyze_object_map.hpp"
#include "opencmiss/zinc/types/streamid.h"
#include "general/image_utilities.h"
#include "general/mystring.h"
#include "general/debug.h"
#include "general/message.h"

template <class myType>
void ByteSwapWhenSystemIsLittleEndian(myType *p)
{
	if (systemEndianTest() == EndianLittle)
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
			break;
		}
	}
}

#if defined (ZINC_USE_IMAGEMAGICK)
#  if defined _MSC_VER
   /* When using the gcc compiled headers from msvc we need to replace inline */
#    define inline __inline
#    define MAGICK_STATIC_LINK
#  endif /* defined _MSC_VER */
/* image magick interfaces */
#include "MagickCore/MagickCore.h"
#endif /* defined (ZINC_USE_IMAGEMAGICK) */

AnalyzeObjectEntry::AnalyzeObjectEntry():
	display_flag(1), copy_flag(0), mirror(0),
	status(0), neighbors_used(0),	shades(1),
	start_red(0), start_green(0), start_blue(0),
	end_red(0), end_green(0), end_blue(0),
	x_rot(0), y_rot(0), z_rot(0),
	x_shift(0), y_shift(0), z_shift(0),
	x_center(0), y_center(0), z_center(0),
	x_rot_increment(0), y_rot_increment(0), z_rot_increment(0),
	x_shift_increment(0), y_shift_increment(0), z_shift_increment(0),
	min_x(0), min_y(0), min_z(0),
	max_x(0), max_y(0), max_z(0),
	opacity(0.5), opacity_thick(1), blend_factor(0), voxels_count(0)
{
}

AnalyzeObjectEntry::~AnalyzeObjectEntry()
{
}

template <typename myType>
void AnalyzeObjectEntry::readBytes(std::istream &inputStream, myType *valueOut, const int count, const bool byteSwap)
{
	if(inputStream.read(reinterpret_cast<char *>(valueOut), sizeof(myType) * count).fail())
	{

	}
	if(byteSwap)
		ByteSwapWhenSystemIsLittleEndian<myType>(valueOut);
}

void AnalyzeObjectEntry::printInformation()
{
	display_message(INFORMATION_MESSAGE, "Analyze ObjectEntry: %s\n", name);
	display_message(INFORMATION_MESSAGE, "	Red: %d Green: %d Blue: %d\n", end_red, end_green, end_blue);
	display_message(INFORMATION_MESSAGE, "	Min X: %d Min Y: %d Min Z: %d\n", min_x, min_y, min_z);
	display_message(INFORMATION_MESSAGE, "	Max X: %d Max Y: %d MaxZ: %d\n", max_x, max_y, max_z);
	display_message(INFORMATION_MESSAGE, "	voxels_count: %u\n", voxels_count);
}

void AnalyzeObjectEntry::setVoxelsCount(int valueIn)
{
	voxels_count = valueIn;
}

void AnalyzeObjectEntry::readFromFilePointer(std::istream &inputStream, const bool byteSwap, const bool blendFactor)
{
	readBytes<char>(inputStream, name, 32, byteSwap);
	readBytes<int>(inputStream, &display_flag, 1, byteSwap);
	readBytes<unsigned char>(inputStream, &copy_flag, 1, byteSwap);
	readBytes<unsigned char>(inputStream, &mirror, 1, byteSwap);
	readBytes<unsigned char>(inputStream, &status, 1, byteSwap);
	readBytes<unsigned char>(inputStream, &neighbors_used, 1, byteSwap);
	readBytes<int>(inputStream, &shades, 1, byteSwap);
	readBytes<int>(inputStream, &start_red, 1, byteSwap);
	readBytes<int>(inputStream, &start_green, 1, byteSwap);
	readBytes<int>(inputStream, &start_blue, 1, byteSwap);
	readBytes<int>(inputStream, &end_red, 1, byteSwap);
	readBytes<int>(inputStream, &end_green, 1, byteSwap);
	readBytes<int>(inputStream, &end_blue, 1, byteSwap);
	readBytes<int>(inputStream, &x_rot, 1, byteSwap);
	readBytes<int>(inputStream, &y_rot, 1, byteSwap);
	readBytes<int>(inputStream, &z_rot, 1, byteSwap);
	readBytes<int>(inputStream, &x_shift, 1, byteSwap);
	readBytes<int>(inputStream, &y_shift, 1, byteSwap);
	readBytes<int>(inputStream, &z_shift, 1, byteSwap);
	readBytes<int>(inputStream, &x_center, 1, byteSwap);
	readBytes<int>(inputStream, &y_center, 1, byteSwap);
	readBytes<int>(inputStream, &z_center, 1, byteSwap);
	readBytes<int>(inputStream, &x_rot_increment, 1, byteSwap);
	readBytes<int>(inputStream, &y_rot_increment, 1, byteSwap);
	readBytes<int>(inputStream, &z_rot_increment, 1, byteSwap);
	readBytes<int>(inputStream, &x_shift_increment, 1, byteSwap);
	readBytes<int>(inputStream, &y_shift_increment, 1, byteSwap);
	readBytes<int>(inputStream, &z_shift_increment, 1, byteSwap);
	readBytes<short int>(inputStream, &min_x, 1, byteSwap);
	readBytes<short int>(inputStream, &min_y, 1, byteSwap);
	readBytes<short int>(inputStream, &min_z, 1, byteSwap);
	readBytes<short int>(inputStream, &max_x, 1, byteSwap);
	readBytes<short int>(inputStream, &max_y, 1, byteSwap);
	readBytes<short int>(inputStream, &max_z, 1, byteSwap);
	readBytes<float>(inputStream, &opacity, 1, byteSwap);
	readBytes<int>(inputStream, &opacity_thick, 1, byteSwap);
	readBytes<float>(inputStream, &blend_factor, 1, byteSwap);
}

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

AnalyzeObjectMapHandler::AnalyzeObjectMapHandler(enum AnalyzeStreamsType streamsTypeIn) :
	filename(0),
	bigEndian(false),
	data(0),
	version(0),
	streamsType(streamsTypeIn)
{
}

AnalyzeObjectMapHandler::~AnalyzeObjectMapHandler()
{
	if (filename)
		DEALLOCATE(filename);
	for(int i = 0; i < numberOfObjects; i++)
	{
		delete entriesArray[i];
	}
	delete[] entriesArray;
	if (data)
		DEALLOCATE(data);
}


bool AnalyzeObjectMapHandler::readImage(std::istream &inputStream)
{
	int numebr_of_pixels = width * height;
	//printf("width: %d height: %d depth: %d", width, height, depth);
	if (numebr_of_pixels > 0)
	{
		int readLimit = 10000;
		unsigned char *slice_values  = new unsigned char[readLimit];
		int current_slices = 1;
		int current_pixel_num = 0;
		bool cont = true;
		ALLOCATE(data, unsigned char, numebr_of_pixels * depth);
		unsigned char *current_data = (unsigned char *)data;
		int counter = 0;
		int *object_found = new int[numberOfObjects];
		for (int i = 0; i < numberOfObjects; i++)
		{
			object_found[i] = 0;
		}
		do
		{
			inputStream.read(reinterpret_cast<char *>(slice_values), readLimit);
			for (int i = 0; (i  < readLimit && cont) ;)
			{
				counter++;
				for (int j = 0; j < slice_values[i]; j++)
				{
					*current_data = slice_values[i+1];
					current_data++;
					object_found[ slice_values[i+1]] = object_found[slice_values[i+1]] + slice_values[i];
				}
				current_pixel_num = current_pixel_num + slice_values[i];
				if (current_pixel_num >= numebr_of_pixels)
				{
					//printf("counter %d, numebr_of_pixels %d, current %d\n", counter,
					//	numebr_of_pixels, current_pixel_num);
					current_pixel_num = 0;
					counter = 0;
					if (current_slices == depth)
					{

						cont = false;
					}
					else
					{
						current_slices++;
						//printf("--------------slice %d-----------\n", current_slices);
					}
				}
				i = i + 2;
			}
		} while (cont == true);
		for (int i = 0; i < numberOfObjects; i++)
		{
			entriesArray[i]->setVoxelsCount(object_found[i]);
		}
		delete[] object_found;
	}

	return true;
}

bool AnalyzeObjectMapHandler::readImageInformationInternal(void *buffer, int buffer_size)
{
	membuf my_buf((char *)buffer, (char *)buffer + buffer_size);
	std::istream is(&my_buf);
	bool byteSwapped = false;
	int header[6] = {1};

	if( is.read(reinterpret_cast<char *>(header), sizeof(int) * 5).fail() )
	{
		return false;
	}
	// version 7 byte swapped
	if( header[0] == -1913442047 || header[0] == 1323699456 )
	{
		byteSwapped = true;
		ByteSwapWhenSystemIsLittleEndian<int>(&(header[0]));
		ByteSwapWhenSystemIsLittleEndian<int>(&(header[1]));
		ByteSwapWhenSystemIsLittleEndian<int>(&(header[2]));
		ByteSwapWhenSystemIsLittleEndian<int>(&(header[3]));
		ByteSwapWhenSystemIsLittleEndian<int>(&(header[4]));
	}
	bool blendFactor = false;
	if(header[0] == OBJ_VERSION7)
	{
		if((is.read(reinterpret_cast<char *>(&(header[5])), sizeof(int) * 1) ).fail())
		{
			return false;
		}

		if(byteSwapped)
		{
			ByteSwapWhenSystemIsLittleEndian<int>(&(header[5]));
		}
		blendFactor = true;
	}
	if(header[5] > 1)
	{
		this->numberOfDimensions = 4;
	}
	else if(header[3] > 1)
	{
		this->numberOfDimensions = 3;
	}
	else if(header[2] > 1)
	{
		this->numberOfDimensions = 2;
	}
	else
	{
		this->numberOfDimensions = 1;
	}
	version = header[0];
	width = header[1];
	height = header[2];
	depth = header[3];

	numberOfObjects = header[4];
	if((header[4] < 1) || (header[4] > 256))
	{
		return false;
	}

	entriesArray = new AnalyzeObjectEntry*[numberOfObjects];
	for(int i = 0; i < numberOfObjects; i++)
	{
		entriesArray[i] = new AnalyzeObjectEntry();
		entriesArray[i]->readFromFilePointer(is, byteSwapped, blendFactor);
	}
	readImage(is);

	return true;
}

bool AnalyzeObjectMapHandler::readImageInformation(void *buffer, int buffer_size)
{
	bool success = false;
	if (buffer && buffer_size)
		success = readImageInformationInternal(buffer, buffer_size);
	return success;
}

bool AnalyzeObjectMapHandler::readImageInformation()
{
	bool success = false;
	std::ifstream fileStream(filename, std::ifstream::binary);
	if (fileStream.is_open())
	{
		fileStream.seekg (0, fileStream.end);
		int fileSize = fileStream.tellg();
		char *memblock = new char [fileSize];
		fileStream.seekg (0, fileStream.beg);
		fileStream.read (memblock, fileSize);
		fileStream.close();
		if (memblock && fileSize > 0)
			success = readImageInformationInternal((void *)memblock, fileSize);
		delete[] memblock;
	}

	return success;
}

AnalyzeObjectEntry *AnalyzeObjectMapHandler::getObjectEntry(int index)
{
	if (numberOfObjects > index)
		return entriesArray[index];
	return 0;
}

int AnalyzeObjectMapHandler::getNumberOfDimensions()
{
	return this->numberOfDimensions;
}

int AnalyzeObjectMapHandler::getNumberOfObjects()
{
	return this->numberOfObjects;
}

bool AnalyzeObjectMapHandler::setFilename(const char *filenameIn)
{
	return (0 != (filename = duplicate_string(filenameIn)));
}

int AnalyzeObjectMapHandler::getHeight()
{
	return height;
}

int AnalyzeObjectMapHandler::getWidth()
{
	return width;
}

int AnalyzeObjectMapHandler::getDepth()
{
	return depth;
}

int AnalyzeObjectMapHandler::getComponentDepth()
{
	return 8; //Unsigned char
}

int AnalyzeObjectMapHandler::getNumberOfComponents()
{
	return 1;
}

int AnalyzeObjectMapHandler::getOrientation() const
{
	return 0;
}

int AnalyzeObjectMapHandler::getGlMax() const
{
	return 255;
}

int AnalyzeObjectMapHandler::getGlMin() const
{
	return 0;
}

const char * AnalyzeObjectMapHandler::getQuantumFormat() const
{
	return "Unsigned";
}

void AnalyzeObjectMapHandler::printInformation()
{
	display_message(INFORMATION_MESSAGE, "Object Map version: %d, dimension: %d\n", version, numberOfDimensions);
	display_message(INFORMATION_MESSAGE, "Width: %d, Height: %d, Depth: %d\n", width, height, depth);
	display_message(INFORMATION_MESSAGE, "Number Of Objects: %d\n", numberOfObjects);
}

struct Cmgui_image_information_memory_block AnalyzeObjectMapHandler::getImageMemoryBlock(int index)
{
	int block_length = getHeight()*getWidth()*getComponentDepth()/8;
	struct Cmgui_image_information_memory_block ii;
	ii.buffer = (char *)data + index * block_length;
	ii.length = block_length;
	ii.memory_block_is_imagemagick_blob = 0;

	return ii;
}

struct Cmgui_image *Cmgui_image_read_analyze_object_map(
		struct Cmgui_image_information *cmgui_image_information,
		enum cmzn_streaminformation_data_compression_type data_compression_type)
{
	struct Cmgui_image *cmgui_image = 0;
#if defined (ZINC_USE_IMAGEMAGICK)
	const char *file_name = 0, file_name_prefix[] = "aze:";
	char tmp100[100];
	char *old_magick_size, magick_size[41];
	int width = 0, height = 0;
	Image *magick_image, *temp_magick_image;
	ImageInfo *magick_image_info;
	ExceptionInfo *magick_exception;

	if (cmgui_image_information && cmgui_image_information->valid &&
		((cmgui_image_information->file_names && cmgui_image_information->number_of_file_names == 1) ||
		(cmgui_image_information->memory_blocks && cmgui_image_information->number_of_memory_blocks == 1)))
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

		width = cmgui_image_information->width;
		height = cmgui_image_information->height;
		if (cmgui_image_information->file_names)
		{
			file_name = cmgui_image_information->file_names[0];
		}
		else
		{
			/* Just use a dummy file_name */
			file_name = "memory";
		}
		AnalyzeStreamsType streamstype = ANALYZE_STREAMS_TYPE_FILE;
		char *uncompressed_bytes = 0;
		int uncompressed_length = 0;
		if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_GZIP ||
			data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_BZIP2)
		{
			if (cmgui_image_information->memory_blocks &&
				cmgui_image_information->memory_blocks[0])
			{
				if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_GZIP)
					uncompressed_length = open_gzip_stream(cmgui_image_information->memory_blocks[0]->buffer,
						cmgui_image_information->memory_blocks[0]->length, &uncompressed_bytes);
				else if (data_compression_type == CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_BZIP2)
					uncompressed_length = open_bzip2_stream(cmgui_image_information->memory_blocks[0]->buffer,
						cmgui_image_information->memory_blocks[0]->length, &uncompressed_bytes);
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
				streamstype = ANALYZE_STREAMS_TYPE_MEMORY;
			}
		}

		AnalyzeObjectMapHandler analyzeObject = AnalyzeObjectMapHandler(streamstype);
		if ((streamstype == ANALYZE_STREAMS_TYPE_FILE && analyzeObject.setFilename(file_name) &&
			analyzeObject.readImageInformation()) ||
			(streamstype == ANALYZE_STREAMS_TYPE_MEMORY &&
				analyzeObject.readImageInformation(cmgui_image_information->memory_blocks[0]->buffer,
					cmgui_image_information->memory_blocks[0]->length)))
		{
			analyzeObject.printInformation();
			int numberOfDimensions = analyzeObject.getNumberOfDimensions();
			int numberOfObjects = analyzeObject.getNumberOfObjects();
			for (int i = 0; i < numberOfObjects; i++)
			{
				display_message(INFORMATION_MESSAGE, "#%d ", i);
				AnalyzeObjectEntry *entry = analyzeObject.getObjectEntry(i);
				entry->printInformation();
			}

			if (numberOfDimensions <= 3)
			{
				width = analyzeObject.getWidth();
				height = analyzeObject.getHeight();
				sprintf(magick_image_info->filename, "%s%s",
					file_name_prefix, file_name);
				size_t filename_len = strlen(magick_image_info->filename);
				// We have to change the file suffix of the Analyze header file
				// because 'hdr' is already known to ImageMagick.
				magick_image_info->filename[filename_len-3] = 'a';
				magick_image_info->filename[filename_len-2] = 'z';
				magick_image_info->filename[filename_len-1] = 'e';
				magick_image_info->orientation = static_cast<OrientationType>(0);
				if ((0 < width) && (0 < height))
				{
					sprintf(magick_size, "%dx%d", width, height);
					magick_image_info->size = magick_size;
				}
				int number_of_components = analyzeObject.getNumberOfComponents();
				int number_of_bits_per_component = analyzeObject.getComponentDepth();
				magick_image_info->colorspace = GRAYColorspace;
				if (3 == number_of_components)
				{
					magick_image_info->colorspace = RGBColorspace;
				}
				if (number_of_bits_per_component == 8 ||
					number_of_bits_per_component == 16 ||
					number_of_bits_per_component == 32)
				{
					snprintf(tmp100, 99, "%d", analyzeObject.getGlMax());
					SetImageOption(magick_image_info, "quantum:maximum", tmp100);
					snprintf(tmp100, 99, "%d", analyzeObject.getGlMin());
					SetImageOption(magick_image_info, "quantum:minimum", tmp100);
					snprintf(tmp100, 99, "%s", analyzeObject.getQuantumFormat());
					SetImageOption(magick_image_info, "quantum:format", tmp100);
					magick_image_info->depth = number_of_bits_per_component;
					magick_image_info->type = GrayscaleType;
					magick_image_info->endian = MSBEndian;
//					if (analyzeObject.isBigEndian())
//					{
//						magick_image_info->endian = MSBEndian;
//					}
//					else
//					{
//						magick_image_info->endian = LSBEndian;
//					}
					for (int i = 0; (i < analyzeObject.getDepth()) && return_code; i++)
					{
						struct Cmgui_image_information_memory_block memory_block = analyzeObject.getImageMemoryBlock(i);

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
			display_message(ERROR_MESSAGE, "Analyze object map handler not able to open file '%s'", file_name);
			return_code = 0;
		}
		if (uncompressed_bytes)
			DEALLOCATE(uncompressed_bytes);

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
	display_message(ERROR_MESSAGE, "Analyze object map handler not implemented for this version.");
	USE_PARAMETER(cmgui_image_information);
#endif /* defined (ZINC_USE_IMAGEMAGICK) */

	return cmgui_image;
}
