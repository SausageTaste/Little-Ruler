#include "u_fileutils.h"

#ifdef _WIN32
#pragma warning(disable:4996)
// To disable fopen deprecated error which is caused by tga.h
#endif

#include <tga.h>
#include <lodepng.h>
#include <fmt/format.h>

#include "s_logger_god.h"
#include "u_filesystem.h"


// Image file type check
namespace {

    enum class ImageType { png, tga };

    bool isBufPNG(const uint8_t* const buf) {
        return (buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G');
    }

}


// Image reader functions
namespace {

    bool parseImagePNG(dal::ImageFileData& output, std::vector<uint8_t>& dataBuffer) {
        unsigned int w, h;
        auto error = lodepng::decode(output.vector(), w, h, dataBuffer);
        if ( error ) {
            dalError(fmt::format("PNG decode error: {}", lodepng_error_text(error)));
            return false;
        }

        output.setDimensions(static_cast<size_t>(w), static_cast<size_t>(h), 4);

        output.flipY();  // TGA does this automatically but not this.

        dalAssert(output.checkValidity());
        return true;
    }

    bool parseImageTGA(dal::ImageFileData& output, std::vector<uint8_t>& dataBuffer) {
        int w, h, p;
        std::unique_ptr<uint8_t, decltype(free)*> result{
            tga_load_memory(dataBuffer.data(), static_cast<int>(dataBuffer.size()), &w, &h, &p), free
        };

        if ( nullptr == result ) {
            dalError("Failed to parse tga file.");
            return false;
        }

        output.setDimensions(static_cast<size_t>(w), static_cast<size_t>(h), static_cast<size_t>(p));

        const auto resArrSize = output.width() * output.height() * output.pixSize();
        output.vector().clear();
        output.vector().insert(output.vector().begin(), result.get(), result.get() + resArrSize);

        dalAssert(output.checkValidity());
        return true;
    }

}


// file utils
namespace dal {

    bool loadFileText(const char* const respath, std::string& buffer) {
        auto file = fileopen(respath, FileMode2::read);
        if ( nullptr == file ) {
            return false;
        }
        else {
            return file->readText(buffer);
        }
    }

    bool loadFileImage(const char* const respath, ImageFileData& data) {
        std::vector<uint8_t> fileBuffer;
        {
            auto file = fileopen(respath, FileMode2::bread);
            if ( nullptr == file ) {
                return false;
            }

            const auto fileSize = file->getSize();
            fileBuffer.resize(fileSize);
            if ( !file->read(fileBuffer.data(), fileBuffer.size()) ) {
                return false;
            }
        }

        ImageType imgtype;
        {
            if ( isBufPNG(fileBuffer.data()) ) {
                imgtype = ImageType::png;
            }
            else {
                const auto extension = findExtension(respath);
                
                if ( extension == "png" ) {
                    imgtype = ImageType::png;
                }
                else if ( extension == "tga" ) {
                    imgtype = ImageType::tga;
                }
                else {
                    dalError(fmt::format("Image type '{}' is not supported: {}", extension, respath));
                    return false;
                }
            }
        }

        decltype(parseImagePNG)* parseFunc = nullptr;
        switch ( imgtype ) {

        case ImageType::png:
            parseFunc = parseImagePNG;
            break;
        case ImageType::tga:
            parseFunc = parseImageTGA;
            break;

        }

        if ( !parseFunc(data, fileBuffer) ) {
            dalError(fmt::format("Error while parsing image: {}", respath));
            return false;
        }
        else {
            return true;
        }
    }

    bool loadFileBuffer(const char* const respath, std::vector<uint8_t>& buffer) {
        auto file = fileopen(respath, FileMode2::bread);
        if ( nullptr == file ) {
            return false;
        }

        const auto fileSize = file->getSize();
        buffer.resize(fileSize);
        return file->read(buffer.data(), buffer.size()) == fileSize;
    }

}
