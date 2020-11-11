// SyncUpsImage.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <algorithm>
#include <filesystem>
#include "FreeImage.h"

/** Generic image loader
    @param lpszPathName Pointer to the full file name
    @param flag Optional load flag constant
    @return Returns the loaded dib if successful, returns NULL otherwise
*/
FIBITMAP* GenericLoader(const char* lpszPathName, int flag) {
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

    // check the file signature and deduce its format
    // (the second argument is currently not used by FreeImage)
    fif = FreeImage_GetFileType(lpszPathName, 0);
    if (fif == FIF_UNKNOWN) {
        // no signature ?
        // try to guess the file format from the file extension
        fif = FreeImage_GetFIFFromFilename(lpszPathName);
    }
    // check that the plugin has reading capabilities ...
    if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
        // ok, let's load the file
        FIBITMAP* dib = FreeImage_Load(fif, lpszPathName, flag);
        // unless a bad file format, we are done !
        return dib;
    }
    return NULL;
}

enum returnStatis{SUCCESSFUL_WO_CONV = 0, FAILED, CONVERTED_SUCCESSFUL};
// return 0 if successful, 1 if failed
int convertToPng(std::string srcFileName, unsigned long* transparent)
{
    bool haveTransparent = false;
    int transparentColor8bits = 0;
    RGBQUAD backgroundColor;
    backgroundColor.rgbRed = 255, backgroundColor.rgbGreen = 255, backgroundColor.rgbBlue = 255, backgroundColor.rgbReserved = 255;

    FIBITMAP* srcImg = GenericLoader(srcFileName.data(), 0);
    int srcBytespp = FreeImage_GetLine(srcImg) / FreeImage_GetWidth(srcImg);
    if ( srcBytespp != 1 && srcBytespp != 4 )
    {
        FreeImage_Unload(srcImg);
        return SUCCESSFUL_WO_CONV;
    }
    BYTE* srcBits = (BYTE*)FreeImage_GetBits(srcImg);
    unsigned srcWidth = FreeImage_GetWidth(srcImg), srcHeight = FreeImage_GetHeight(srcImg);

    if (FreeImage_IsTransparent(srcImg) )
    {
        transparentColor8bits = FreeImage_GetTransparentIndex(srcImg);
    }
    else
    {
        FreeImage_Unload(srcImg);
        return SUCCESSFUL_WO_CONV;
    }

    if (FreeImage_HasBackgroundColor(srcImg))
    {
        FreeImage_GetBackgroundColor(srcImg, &backgroundColor);
    }

    FIBITMAP* dstImg = FreeImage_ConvertTo32Bits(srcImg);
    int dstBytespp = FreeImage_GetLine(dstImg) / FreeImage_GetWidth(dstImg);
    BYTE* dstBits = (BYTE*)FreeImage_GetBits(dstImg);
    unsigned dstWidth = FreeImage_GetWidth(dstImg), dstHeight = FreeImage_GetHeight(dstImg);

    int scale = 1; // no scaling of images

    for (int r = 0; r < srcHeight; ++r)
    {
        srcBits = FreeImage_GetScanLine(srcImg, r);

        for (int c = 0; c < srcWidth; ++c)
        {
            // transparent
            if ((srcBytespp == 1 && transparentColor8bits >= 0 && srcBits[c] == transparentColor8bits)
                || (srcBytespp == 4 && srcBits[c * 4 + FI_RGBA_ALPHA] == 0))
            {
                haveTransparent = true;

                for (int dst_r = 0; dst_r < scale; ++dst_r)
                {
                    dstBits = FreeImage_GetScanLine(dstImg, r * scale + dst_r);
                    for (int dst_c = 0; dst_c < scale; ++dst_c)
                    {
                        dstBits[c * dstBytespp * scale + dst_c * dstBytespp + FI_RGBA_RED] = backgroundColor.rgbRed;
                        dstBits[c * dstBytespp * scale + dst_c * dstBytespp + FI_RGBA_GREEN] = backgroundColor.rgbGreen;
                        dstBits[c * dstBytespp * scale + dst_c * dstBytespp + FI_RGBA_BLUE] = backgroundColor.rgbBlue;
                        dstBits[c * dstBytespp * scale + dst_c * dstBytespp + FI_RGBA_ALPHA] = 0xff;
                    }
                }
            }
        }
    }
   
    if (haveTransparent)
    {
        (*transparent)++;
        FreeImage_Save(FIF_PNG, dstImg, srcFileName.data());
    }

    FreeImage_Unload(srcImg);
    FreeImage_Unload(dstImg);

    return CONVERTED_SUCCESSFUL;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Wrong number of parameters. See below on syntax. Please ensure that you backup your images as program will overwrite them." << std::endl;
        std::cout << "   Trans2Png \"destination directory\"" << std::endl;
        std::cout << "   Eg  : Trans2Png \"D:\\dest\"";
        exit(1);
    }

    std::string dstDirName = argv[1];

    unsigned long filesSuccess = 0, filesExtChanged = 0, filesFailed = 0, totalFiles = 0, filesTransparent = 0;
    std::filesystem::directory_iterator dst_it(argv[1]);

    FreeImage_Initialise();

    for (const auto& dst : dst_it)
    {
        if (!dst.is_regular_file())
        {
            continue;
        }
        std::string dstPath = std::filesystem::absolute(dst).string();
        int startOfDstFileName = (dstPath.find_last_of("/") == std::string::npos ? 0 : dstPath.find_last_of("/"))
            + (dstPath.find_last_of("\\") == std::string::npos ? 0 : dstPath.find_last_of("\\")) + 1;
        std::string dstFileWoExt = dstPath.substr(startOfDstFileName, dstPath.find_last_of(".") - startOfDstFileName);
        std::string dstFileExt = dstPath.substr(dstPath.find_last_of("."));
        if (dstFileExt.size() <= 0)
        {
            std::cout << "Destination file - " + dstPath + " does not have a file extension. Ignored.";
            filesFailed++;
            totalFiles++;
            continue;
        }

        // must be before rename else filename will be wrong
        int status = convertToPng(dstPath, &filesTransparent);

        filesFailed += status == FAILED ? 1 : 0;
        filesSuccess += status == CONVERTED_SUCCESSFUL ? 1 : 0;

        totalFiles++;
    }

    std::cout << "Total files processed : " << totalFiles << std::endl;
    std::cout << "Total files successfully processed : " << filesSuccess << std::endl;
    std::cout << "Total files with transparent background in source : " << filesTransparent << std::endl;
    std::cout << "Total files failed : " << filesFailed << std::endl;

    FreeImage_DeInitialise();
    return 0;
}
