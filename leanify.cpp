#include "leanify.h"

#include <iostream>

#include "formats/miniz/miniz.h"
#include "formats/zopfli/zlib_container.h"

#include "formats/format.h"
#include "formats/gft.h"
#include "formats/gz.h"
#include "formats/ico.h"
#include "formats/jpeg.h"
#include "formats/lua.h"
#include "formats/pe.h"
#include "formats/png.h"
#include "formats/swf.h"
#include "formats/tar.h"
#include "formats/rdb.h"
#include "formats/xml.h"
#include "formats/zip.h"

// Leanify the file
// and move the file ahead size_leanified bytes
// the new location of the file will be file_pointer - size_leanified
// it's designed this way to avoid extra memmove or memcpy
// return new size
size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified /*= 0*/)
{
    if (depth > max_depth)
    {
        return Format(file_pointer, file_size).Leanify(size_leanified);
    }
    if (memcmp(file_pointer, Png::header_magic, sizeof(Png::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "PNG detected." << std::endl;
        }
        return Png(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Jpeg::header_magic, sizeof(Jpeg::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "JPEG detected." << std::endl;
        }
        return Jpeg(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Lua::header_magic, sizeof(Lua::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "Lua detected." << std::endl;
        }
        return Lua(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Zip::header_magic, sizeof(Zip::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "ZIP detected." << std::endl;
        }
        return Zip(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Pe::header_magic, sizeof(Pe::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "PE detected." << std::endl;
        }
        return Pe(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Gz::header_magic, sizeof(Gz::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "GZ detected." << std::endl;
        }
        return Gz(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Ico::header_magic, sizeof(Ico::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "ICO detected." << std::endl;
        }
        return Ico(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, "(DWF V06.00)", 12) == 0)
    {
        if (is_verbose)
        {
            std::cout << "DWF detected." << std::endl;
        }
        return Zip((char *)file_pointer + 12, file_size - 12).Leanify(size_leanified) + 12;
    }
    else if (memcmp(file_pointer, Gft::header_magic, sizeof(Gft::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "GFT detected." << std::endl;
        }
        return Gft(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Rdb::header_magic, sizeof(Rdb::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "RDB detected." << std::endl;
        }
        return Rdb(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (memcmp(file_pointer, Swf::header_magic, sizeof(Swf::header_magic)) == 0 ||
             memcmp(file_pointer, Swf::header_magic_deflate, sizeof(Swf::header_magic_deflate)) == 0 ||
             memcmp(file_pointer, Swf::header_magic_lzma, sizeof(Swf::header_magic_lzma)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "SWF detected." << std::endl;
        }
        return Swf(file_pointer, file_size).Leanify(size_leanified);
    }
    else
    {
        // tar file does not have header magic
        // ustar is optional
        {
            Tar t(file_pointer, file_size);
            // checking first record checksum
            if (t.IsValid())
            {
                if (is_verbose)
                {
                    std::cout << "tar detected." << std::endl;
                }
                return t.Leanify(size_leanified);
            }
        }

        // XML file does not have header magic
        // have to parse and see if there are any errors.
        {
            Xml x(file_pointer, file_size);
            if (x.IsValid())
            {
                if (is_verbose)
                {
                    std::cout << "XML detected." << std::endl;
                }
                return x.Leanify(size_leanified);
            }
        }
    }

    if (is_verbose)
    {
        std::cout << "Format not supported!" << std::endl;
    }
    // for unsupported format, just memmove it.
    return Format(file_pointer, file_size).Leanify(size_leanified);
}


size_t ZlibRecompress(void *src, size_t src_len, size_t size_leanified /*= 0*/)
{
    if (!is_fast)
    {
        size_t s = 0;
        unsigned char *buffer = (unsigned char *)tinfl_decompress_mem_to_heap(src, src_len, &s, TINFL_FLAG_PARSE_ZLIB_HEADER);
        if (!buffer)
        {
            std::cerr << "Decompress Zlib data failed." << std::endl;
        }
        else
        {
            ZopfliOptions zopfli_options;
            ZopfliInitOptions(&zopfli_options);
            zopfli_options.numiterations = iterations;

            size_t new_size = 0;
            unsigned char *out_buffer = NULL;
            ZopfliZlibCompress(&zopfli_options, buffer, s, &out_buffer, &new_size);
            mz_free(buffer);
            if (new_size < src_len)
            {
                memcpy((char *)src - size_leanified, out_buffer, new_size);
                delete[] out_buffer;
                return new_size;
            }
            delete[] out_buffer;
        }
    }

    memmove((char *)src - size_leanified, src, src_len);
    return src_len;
}