#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/*!
* @file tga.h
* @brief All C library functional.
*/

/**
* @brief Loads TGA from file.
*
* ## Example
*
* ```c
* int w, h, b;
* uint8_t* data = tga_load("image.tga", &w, &h, &b);
* ```
*
* @param Filename Image file to load.
* @param Width Pointer to int to store image width.
* @param Height Pointer to int to store image height.
* @param Bpp Pointer to int to store size of pixel in bytes.
*/
uint8_t* tga_load(const char* Filename, int* Width, int* Height, int* Bpp);
/**
* @brief Loads TGA from memory buffer.
* @param Data Buffer storing image file data.
* @param Size Size of buffer.
* @param Width Pointer to int to store image width.
* @param Height Pointer to int to store image height.
* @param Bpp Pointer to int to store sixe of pixel in bytes.
*/
uint8_t* tga_load_memory(uint8_t* Data, int Size, int* Width, int* Height, int* Bpp);

static void tga_load_compressed_paletted_8(uint8_t* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size, size_t PixelSize);
static void tga_load_compressed_paletted_16(uint16_t* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size, size_t PixelSize);
static void tga_load_compressed_true_color_24(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size);
static void tga_load_compressed_true_color_32(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size);
static void tga_load_compressed_monochrome(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size);

void tga_load_compressed_paletted_8(uint8_t* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size, size_t PixelSize)
{
	uint8_t Index;
	uint8_t Pixel[4];
	uint8_t Tmp;
	uint8_t* ColorMapPtr;

	for (size_t i = 0; i < Size; i++)
	{
		Index = *InBuffer++;
		ColorMapPtr = &ColorMap[Index * PixelSize];

		memcpy(Pixel, ColorMapPtr, PixelSize);
		Tmp = Pixel[0]; Pixel[0] = Pixel[2]; Pixel[2] = Tmp;
		memcpy(OutBuffer, Pixel, PixelSize); OutBuffer += PixelSize;
	}
}

void tga_load_compressed_paletted_16(uint16_t* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size, size_t PixelSize)
{
	uint16_t Index;
	uint8_t Pixel[4];
	uint8_t Tmp;
	uint8_t* ColorMapPtr;

	for (size_t i = 0; i < Size; i++)
	{
		Index = *InBuffer++;
		ColorMapPtr = &ColorMap[Index * PixelSize];

		memcpy(Pixel, ColorMapPtr, PixelSize);
		Tmp = Pixel[0]; Pixel[0] = Pixel[2]; Pixel[2] = Tmp;
		memcpy(OutBuffer, Pixel, PixelSize); OutBuffer += PixelSize;
	}
}

static void tga_load_compressed_true_color_24(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size)
{
	uint8_t Header;
	uint8_t Pixel[3];
	size_t i, j, PixelCount;

	for (i = 0; i < Size; )
	{
		Header = *InBuffer++;
		PixelCount = (Header & 0x7F) + 1;

		if (Header & 0x80)
		{
			Pixel[2] = *InBuffer++;
			Pixel[1] = *InBuffer++;
			Pixel[0] = *InBuffer++;

			for (j = 0; j < PixelCount; j++)
			{
				memcpy(OutBuffer, Pixel, 3); OutBuffer += 3;
			}

			i += PixelCount;
		}
		else
		{
			for (j = 0; j < PixelCount; j++)
			{
				Pixel[2] = *InBuffer++;
				Pixel[1] = *InBuffer++;
				Pixel[0] = *InBuffer++;
				memcpy(OutBuffer, Pixel, 3); OutBuffer += 3;
			}

			i += PixelCount;
		}
	}
}

static void tga_load_compressed_true_color_32(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size)
{
	uint8_t Header;
	uint8_t Pixel[4];
	size_t i, j, PixelCount;

	for (i = 0; i < Size; )
	{
		Header = *InBuffer++;
		PixelCount = (Header & 0x7F) + 1;

		if (Header & 0x80)
		{
			Pixel[2] = *InBuffer++;
			Pixel[1] = *InBuffer++;
			Pixel[0] = *InBuffer++;
			Pixel[3] = *InBuffer++;

			for (j = 0; j < PixelCount; j++)
			{
				memcpy(OutBuffer, Pixel, 4); OutBuffer += 4;
			}

			i += PixelCount;
		}
		else
		{
			for (j = 0; j < PixelCount; j++)
			{
				Pixel[2] = *InBuffer++;
				Pixel[1] = *InBuffer++;
				Pixel[0] = *InBuffer++;
				Pixel[3] = *InBuffer++;

				memcpy(OutBuffer, Pixel, 4); OutBuffer += 4;
			}

			i += PixelCount;
		}
	}
}

void tga_load_compressed_monochrome(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size)
{
	uint8_t Header;
	uint8_t Red;
	size_t i, j, PixelCount;

	for (i = 0; i < Size; )
	{
		Header = *InBuffer++;
		PixelCount = (Header & 0x7F) + 1;

		if (Header & 0x80)
		{
			Red = *InBuffer++;

			for (j = 0; j < PixelCount; j++)
			{
				*OutBuffer++ = Red;
			}

			i += PixelCount;
		}
		else
		{
			for (j = 0; j < PixelCount; j++)
			{
				*OutBuffer++ = *InBuffer++;
			}

			i += PixelCount;
		}
	}
}

uint8_t* tga_load(const char* Filename, int* Width, int* Height, int* Bpp)
{
	FILE* f = fopen(Filename, "rb");
	if (f == NULL) return NULL;

	size_t Size = 0;
	fseek(f, 0, SEEK_END);
	Size = ftell(f);
	rewind(f);

	uint8_t* Buffer = (uint8_t*)malloc(Size);
	if (Buffer == NULL) return NULL;
	if (fread(Buffer, 1, Size, f) != Size)
	{
		fclose(f);
		return NULL;
	}

	fclose(f);

	uint8_t* Result = tga_load_memory(Buffer, Size, Width, Height, Bpp);
	free(Buffer);
	return Result;
}

uint8_t* tga_load_memory(uint8_t* Data, int Size, int* Width, int* Height, int* Bpp)
{
	struct
	{
		uint8_t IDLength;
		uint8_t ColorMapType;
		uint8_t ImageType;

		uint16_t ColorMapOrigin;
		uint16_t ColorMapLength;
		uint8_t  ColorMapEntrySize;

		uint16_t XOrigin;
		uint16_t YOrigin;
		uint16_t Width;
		uint16_t Height;
		uint8_t  Bits;
		uint8_t  ImageDescriptor;
	} Header;

	Header.IDLength          = *Data++;
	Header.ColorMapType      = *Data++;
	Header.ImageType         = *Data++;
	Header.ColorMapOrigin    = *((uint16_t*)Data); Data += sizeof(uint16_t);
	Header.ColorMapLength    = *((uint16_t*)Data); Data += sizeof(uint16_t);
	Header.ColorMapEntrySize = *Data++;
	Header.XOrigin           = *((uint16_t*)Data); Data += sizeof(uint16_t);
	Header.YOrigin           = *((uint16_t*)Data); Data += sizeof(uint16_t);
	Header.Width             = *((uint16_t*)Data); Data += sizeof(uint16_t);
	Header.Height            = *((uint16_t*)Data); Data += sizeof(uint16_t);
	Header.Bits              = *Data++;
	Header.ImageDescriptor   = *Data++;

	Data += Header.ImageDescriptor;

	size_t ColorMapElementSize = Header.ColorMapEntrySize / 8;
	size_t ColorMapSize = Header.ColorMapLength * ColorMapElementSize;
	uint8_t* ColorMap = (uint8_t*)malloc(ColorMapSize);

	if (Header.ColorMapType == 1)
	{
		memcpy(ColorMap, Data, ColorMapSize);
		Data += ColorMapSize;
	}

	size_t PixelSize = Header.ColorMapLength == 0 ? (Header.Bits / 8) : ColorMapElementSize;
	size_t DataSize = Size - sizeof(Header) - (Header.ColorMapType == 1 ? ColorMapSize : 0);
	size_t ImageSize = Header.Width * Header.Height * PixelSize;

	uint8_t* Result = (uint8_t*)calloc(ImageSize, 1);

	switch (Header.ImageType)
	{
		case 0: break; // No Image
		case 1: // Uncompressed paletted
		{
			if (PixelSize == 3 || PixelSize == 4)
			{
				switch (Header.Bits)
				{
				case 8:  tga_load_compressed_paletted_8 ((uint8_t*) Data, ColorMap, Result, Header.Width * Header.Height, PixelSize); break;
				case 16: tga_load_compressed_paletted_16((uint16_t*)Data, ColorMap, Result, Header.Width * Header.Height, PixelSize); break;
				}
			}

			break;
		}
		case 2: // Uncompressed TrueColor
		{
			if (Header.Bits = 24 || Header.Bits == 32)
			{
				memcpy(Result, Data, ImageSize);
				uint8_t Tmp;

				for (size_t i = 0; i < ImageSize; i += PixelSize)
				{
					Tmp = Result[i]; Result[i] = Result[i + 2]; Result[i + 2] = Tmp;
				}
			}

			break;
		}

		case 3: // Uncompressed Monochrome
		{
			if (Header.Bits == 8)
			{
				memcpy(Result, Data, ImageSize);
			}

			break;
		}

		case 9: break; // Compressed paletted TODO
		case 10: // Compressed TrueColor
		{
			switch (Header.Bits)
			{
			case 24: tga_load_compressed_true_color_24(Data, Result, Header.Width * Header.Height); break;
			case 32: tga_load_compressed_true_color_32(Data, Result, Header.Width * Header.Height); break;
			}

			break;
		}

		case 11: // Compressed Monocrhome
		{
			if (Header.Bits == 8)
			{
				tga_load_compressed_monochrome(Data, Result, Header.Width * Header.Height);
			}

			break;
		}
	}

	*Width = Header.Width;
	*Height = Header.Height;
	*Bpp = PixelSize;

	free(ColorMap);
	return Result;
}


