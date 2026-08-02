#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>

#include "../../src/render/screens.h"

// BMP file header structure
#pragma pack(push, 1)
struct BMPHeader
{
    uint16_t signature;       // 'BM'
    uint32_t fileSize;        // Size of the BMP file in bytes
    uint16_t reserved1;       // Reserved
    uint16_t reserved2;       // Reserved
    uint32_t dataOffset;      // Offset to the start of image data
    uint32_t headerSize;      // Size of the header
    int32_t width;            // Width of the image
    int32_t height;           // Height of the image
    uint16_t planes;          // Number of color planes
    uint16_t bitsPerPixel;    // Bits per pixel
    uint32_t compression;     // Compression method
    uint32_t imageSize;       // Size of the image data
    int32_t xPixelsPerMeter;  // Horizontal resolution
    int32_t yPixelsPerMeter;  // Vertical resolution
    uint32_t colorsUsed;      // Number of colors in the palette
    uint32_t colorsImportant; // Number of important colors
};
#pragma pack(pop)

class ImageConverter
{
public:
    // Convert monochrome bytes directly to BMP
    static bool monochromeBytesToBmp(const std::vector<uint8_t> &imageData, int width, int height, const std::string &outputPath)
    {
        if (imageData.size() * 8 != width * height)
        {
            std::cerr << "Size mismatch: expected " << (width * height / 8) << " bytes, got " << imageData.size() << std::endl;
            return false;
        }

        // Calculate row padding - each row must be aligned to 4 bytes
        int rowSize = width * 3; // 3 bytes per pixel (RGB)
        int padding = (4 - (rowSize % 4)) % 4;
        int paddedRowSize = rowSize + padding;

        // Create BMP header
        BMPHeader header;
        header.signature = 0x4D42; // 'BM'
        header.fileSize = sizeof(BMPHeader) + paddedRowSize * height;
        header.reserved1 = 0;
        header.reserved2 = 0;
        header.dataOffset = sizeof(BMPHeader);
        header.headerSize = 40; // BITMAPINFOHEADER size
        header.width = width;
        header.height = height; // Positive for bottom-up image
        header.planes = 1;
        header.bitsPerPixel = 24; // 24 bits per pixel (RGB)
        header.compression = 0;   // No compression
        header.imageSize = paddedRowSize * height;
        header.xPixelsPerMeter = 2835; // 72 DPI
        header.yPixelsPerMeter = 2835; // 72 DPI
        header.colorsUsed = 0;
        header.colorsImportant = 0;

        // Open output file
        std::ofstream bmpFile(outputPath, std::ios::binary);
        if (!bmpFile)
        {
            std::cerr << "Failed to open output file" << std::endl;
            return false;
        }

        // Write header
        bmpFile.write(reinterpret_cast<const char *>(&header), sizeof(BMPHeader));

        // Convert monochrome data to RGB pixels and write to BMP
        std::vector<uint8_t> rowBuffer(paddedRowSize, 0);

        for (int y = height - 1; y >= 0; y--)
        { // BMP is stored bottom-up
            // Clear row buffer
            std::fill(rowBuffer.begin(), rowBuffer.end(), 0);

            for (int x = 0; x < width; x++)
            {
                // Calculate bit position in the monochrome data
                int bitPos = y * width + x;
                int bytePos = bitPos / 8;
                int bitOffset = 7 - (bitPos % 8); // MSB first

                // Check if we're in bounds
                if (bytePos < imageData.size())
                {
                    // Get the pixel value (0 = black, 1 = white)
                    uint8_t bit = (imageData[bytePos] >> bitOffset) & 1;

                    // Set RGB value (BGR order in BMP)
                    uint8_t pixelValue = (bit == 0) ? 0 : 255;
                    rowBuffer[x * 3] = pixelValue;     // Blue
                    rowBuffer[x * 3 + 1] = pixelValue; // Green
                    rowBuffer[x * 3 + 2] = pixelValue; // Red
                }
            }

            // Write row
            bmpFile.write(reinterpret_cast<const char *>(rowBuffer.data()), paddedRowSize);
        }

        bmpFile.close();
        return true;
    }

    // Generate a test pattern (checkerboard)
    static std::vector<uint8_t> generateTestPattern(int width, int height)
    {
        int dataSize = (width * height + 7) / 8; // Ceiling division by 8
        std::vector<uint8_t> testData(dataSize, 0);

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                // Create a checkerboard pattern
                bool isWhite = ((x / 8) + (y / 8)) % 2 == 0;

                // Set the corresponding bit
                int bitPos = y * width + x;
                int bytePos = bitPos / 8;
                int bitOffset = 7 - (bitPos % 8); // MSB first

                if (bytePos < dataSize)
                {
                    if (isWhite)
                    {
                        testData[bytePos] |= (1 << bitOffset);
                    }
                }
            }
        }

        return testData;
    }
};

// Example usage
int main(int argc, char *argv[])
{
    const int width = 800;
    const int height = 480;

    // Default render mode is "weather"
    std::string renderMode = "weather";
    if (argc > 1)
    {
        renderMode = argv[1];
    }

    uint16_t imageSize = ((width % 8 == 0) ? (width / 8) : (width / 8 + 1)) * height;
    uint8_t *imageData;
    if ((imageData = (uint8_t *)malloc(imageSize)) == NULL)
    {
        return 1;
    }

    // Create renderer and call appropriate render method
    auto renderer = std::make_unique<WeatherRenderer>(imageData);

    if (renderMode == "night-mode")
    {
        std::cout << "Rendering night mode indicator" << std::endl;
        renderer->renderNightModeIndicator();
    }
    else if (renderMode == "network-error")
    {
        std::cout << "Rendering network error" << std::endl;
        renderer->renderNetworkError();
    }
    else
    {
        // Default: render weather
        std::cout << "Rendering weather" << std::endl;
        auto now = std::chrono::system_clock::now();
        auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        // Generate sample pressure history (24 hours of data)
        PressureHistory pressureHistory;
        float samplePressures[] = {
            1007.5, 1007.6, 1007.8, 1007.6, 1007.9, 1008.2,
            1007.1, 1006.5, 1005.9, 1004.9, 1004.2, 1003.8,
            1003.5, 1002.9, 1002.5, 1002.3, 1001.6, 1001.3,
            1000.5, 999.9, 1000.8, 1001.9, 1003.3, 1004.5};
        for (int i = 0; i < 24; i++)
        {
            pressureHistory.addReading(nowSec - (23 - i) * 3600, samplePressures[i]);
        }

        renderer->renderWeather(WeatherData{
                                    .internal = {
                                        .temperature = 22.5,
                                        .humidity = 55,
                                        .pressure = 1004.5,
                                        .noise = 30,
                                        .co2 = 520},
                                    .external = {.temperature = -20.1, .humidity = 60},
                                    .data_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()),
                                    .retrieval_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())},
                                pressureHistory);
    }

    std::vector<uint8_t> testData(imageData, imageData + imageSize);

    // Convert to BMP
    ImageConverter::monochromeBytesToBmp(testData, width, height, "output.bmp");

    return 0;
}