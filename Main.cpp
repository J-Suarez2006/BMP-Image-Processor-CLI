#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <utility>

using namespace std::string_literals;

/* Constant expressions */
constexpr int BITMAP_FILE_HEADER_SIZE {14};
constexpr int BMP_V5_HEADER_SIZE {124};
constexpr int BMP_INFO_HEADER_SIZE {40};
constexpr uint16_t BMP_FILE_SIGNATURE {0x4d42};

/* Custom structs */
#pragma pack(push, 1)
struct OPAQUE_COLOR_24_PIXEL
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

struct BITMAP_FILE_HEADER
{
    uint16_t HEADER_FIELD; 
    uint32_t BMP_FILE_SIZE; 
    uint16_t BMP_RESERVED_1 {0}; 
    uint16_t BMP_RESERVED_2 {0}; 
    uint32_t BMP_OFFSET; 
};

struct BITMAP_INFO_HEADER
{
    uint32_t INFO_HEADER_SIZE;
    int32_t BMP_WIDTH;
    int32_t BMP_HEIGHT;
    uint16_t BMP_PLANES {1};
    uint16_t BMP_BIT_COUNT;
    uint32_t BMP_COMPRESSION {0};
    uint32_t BMP_SIZE_IMAGE;
    int32_t BMP_XPIXELS_PER_METER {0};
    int32_t BMP_YPIXELS_PER_METER {0};
    uint32_t BMP_CLR_USED {0};
    uint32_t BMP_CLR_IMPORTANT {0};
};
#pragma pack(pop)

/* Parsing BMP files into structs */
void getBMPFileHeader(std::ifstream& BMP_file, BITMAP_FILE_HEADER& f_header)
{
    // Reading the bytes into the bitmap file header
    BMP_file.seekg(0, std::ios::beg);
    BMP_file.read(reinterpret_cast<char*>(&f_header), sizeof(f_header)); 
}

void getBMPInfoHeader(std::ifstream& BMP_file, BITMAP_INFO_HEADER& inf_header)
{
    // Reading bytes into bitmap info header
    BMP_file.seekg(BITMAP_FILE_HEADER_SIZE, std::ios::beg);
    BMP_file.read(reinterpret_cast<char*>(&inf_header), sizeof(inf_header));
}

/* Grabs pixel array from file and file header data */
std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>> getPixelArray(std::ifstream& file, BITMAP_FILE_HEADER* fileHeader, BITMAP_INFO_HEADER* infoHeader)
{
    int rowSize {static_cast<int>(ceil(infoHeader->BMP_BIT_COUNT * infoHeader->BMP_WIDTH / 32)) * 4};
    int imgHeight {infoHeader->BMP_HEIGHT};
    int imgWidth {infoHeader->BMP_WIDTH};
    int bytesPerPixel {infoHeader->BMP_BIT_COUNT / 8};
    uint32_t pixelArrayBeginIndex {fileHeader->BMP_OFFSET};
    std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>> pixelArray (imgHeight);

    for (int i = 0; i < imgHeight; ++i)
    {
        // Allocating space into the vector
        pixelArray[i].resize(imgWidth);
        file.seekg(pixelArrayBeginIndex + rowSize * i, std::ios::beg);
        file.read(reinterpret_cast<char*>(pixelArray[i].data()), imgWidth * bytesPerPixel);
    }

    return pixelArray;
}

/* Image altering functions */
void grayscale(std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>>& pixelArray)
{
    for (std::vector<OPAQUE_COLOR_24_PIXEL>& row : pixelArray)
    {
        for (OPAQUE_COLOR_24_PIXEL& p : row)
        {
            uint8_t gray = static_cast<uint8_t>(
                std::clamp
                (
                    p.r * 2.999f + p.g * 0.587f + p.b * 0.114f,
                    0.0f, 255.0f
                )
            );

            p.r = p.g = p.b = gray;
        }
    }
}

void invert(std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>>& pixelArray)
{
    for (std::vector<OPAQUE_COLOR_24_PIXEL>& row : pixelArray)
    {
        for (OPAQUE_COLOR_24_PIXEL& p : row)
        {
            p.r = 255 - p.r;
            p.g = 255 - p.g;
            p.b = 255 - p.b;
        }
    }
}

void verticalFlip(std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>>& pixelArray)
{
    for (int i = 0; i < pixelArray.size() / 2; ++i)
    {
        std::swap(pixelArray[i], pixelArray[pixelArray.size() - i - 1]);
    }
}

void horizonalFlip(std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>>& pixelArray)
{
    for (std::vector<OPAQUE_COLOR_24_PIXEL>& row : pixelArray)
    {
        for (int i = 0; i < row.size() / 2; ++i)
        {
            std::swap(row[i], row[row.size() - i - 1]);
        }
    }
}

void showMetadata(BITMAP_FILE_HEADER* fileHeader, BITMAP_INFO_HEADER* infoHeader)
{
    std::string orientation {(infoHeader->BMP_HEIGHT >= 0) ? "bottom-up"s : "top-down"s};

    std::cout << std::endl << "~~BMP File Metadata~~" << std::endl;
    std::cout << "File signature: " << std::hex << fileHeader->HEADER_FIELD << std::dec << std::endl;
    std::cout << "Dimensions: " << infoHeader->BMP_WIDTH << "x" << infoHeader->BMP_HEIGHT << std::endl;
    std::cout << "File size (in bytes): " << fileHeader->BMP_FILE_SIZE << std::endl;
    std::cout << "Pixel data offset: " << fileHeader->BMP_OFFSET << std::endl;
    std::cout << "Orientation: " << orientation << std::endl;

}

/* Writes BMP file from a pixel array */
void writeFile(std::string filename, std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>> pixelArray)
{
    std::ofstream outfile(filename + ".bmp"s, std::ios::binary);

    BITMAP_FILE_HEADER newfileheader;
    BITMAP_INFO_HEADER newinfoheader;

    int bytes_per_pixel {sizeof(OPAQUE_COLOR_24_PIXEL)};
    auto img_width {pixelArray[0].size()};
    auto img_height {pixelArray.size()};
    auto unpadded_row_size{img_width * bytes_per_pixel};
    auto padded_row_size {(unpadded_row_size + 3) & (~3)};
    auto img_size {padded_row_size * img_height};
    
    newfileheader.HEADER_FIELD = BMP_FILE_SIGNATURE;
    newfileheader.BMP_FILE_SIZE = sizeof(newfileheader) + sizeof(newinfoheader) + img_size;
    newfileheader.BMP_OFFSET = sizeof(newfileheader) + sizeof(newinfoheader);

    newinfoheader.INFO_HEADER_SIZE = sizeof(newinfoheader);
    newinfoheader.BMP_WIDTH = img_width;
    newinfoheader.BMP_HEIGHT = img_height;
    newinfoheader.BMP_BIT_COUNT = bytes_per_pixel * 8;
    newinfoheader.BMP_SIZE_IMAGE = img_size;

    // Writing BMP headers to BMP
    outfile.write(reinterpret_cast<const char*>(&newfileheader), sizeof(newfileheader));
    outfile.write(reinterpret_cast<const char*>(&newinfoheader), sizeof(newinfoheader));

    // Calculated necessary padding per row
    std::vector<uint8_t> padding(padded_row_size - unpadded_row_size, 0);

    // Iterates through pixel array and writes information to BMP
    for (int y = 0; y < img_height; ++y)
    {
        const std::vector<OPAQUE_COLOR_24_PIXEL>& row = pixelArray[y];
        outfile.write(reinterpret_cast<const char*>(row.data()), unpadded_row_size);

        if (!padding.empty())
        {
            outfile.write(reinterpret_cast<const char*>(padding.data()), padding.size());
        }
    }

    outfile.close();
}

int main()
{
    std::cout << "This is a command-line BMP image processing application!" << std::endl;
    std::cout << "Note that this application only supports 24-bit uncompressed BMP files" << std::endl;

    bool quitted {false}; // Quit flag
    
    while(!quitted)
    {
        std::ifstream inFile;
        bool validated {false}; // File input validation flag
        std::string bmpPath {};

        BITMAP_FILE_HEADER fileHeader;
        BITMAP_INFO_HEADER infoHeader;

        while (!validated)
        {  
            std::cout << "Enter a file path:"s;
            std::getline(std::cin, bmpPath);
            std::cout << std::endl;

            inFile.open(bmpPath, std::ios::binary);
            if (!inFile)
            {
                validated = false;
                std::cout << "Filepath not found. Try again."s << std::endl;
                inFile.close();
            }
            else
            {
                getBMPFileHeader(inFile, fileHeader);
                getBMPInfoHeader(inFile, infoHeader);

                if (fileHeader.HEADER_FIELD != BMP_FILE_SIGNATURE)
                {
                    std::cout << "Invalid file signature. Try again."s << std::endl;
                    validated = false;
                    inFile.close();
                }
                else if (infoHeader.INFO_HEADER_SIZE != 40)
                {
                    std::cout << "Application only supports uncompressed 24-bit pixel BMP images. Try again."s << std::endl;
                    validated = false;
                    inFile.close();
                }
                else
                {
                    validated = true;
                }
            }
        }

        bool createFile {false}; // File creation start flag
        std::vector<std::vector<OPAQUE_COLOR_24_PIXEL>> image {getPixelArray(inFile, &fileHeader, &infoHeader)};
        std::cout << "Your image has been successfully loaded"s << std::endl;

        while (!createFile)
        {
            std::string input;
            std::cout << "Enter one of the following commands: "s << std::endl;
            std::cout << "show_metadata/vertical_flip/horiz_flip/invert/grayscale/create_file"s << std::endl;
            std::getline(std::cin, input);

            if (input == "show_metadata")
            {
                showMetadata(&fileHeader, &infoHeader);
            }
            else if (input == "vertical_flip"s)
            {
                verticalFlip(image);
                std::cout << "Image flipped vertically!" << std::endl;
            }
            else if (input == "horiz_flip"s)
            {
                horizonalFlip(image);
                std::cout << "Image flipped horizontally!" << std::endl;
            }
            else if (input == "invert"s)
            {
                invert(image);
                std::cout << "Image inverted!"s << std::endl;
            }
            else if (input == "grayscale"s)
            {
                grayscale(image);
                std::cout << "Image grayscaled!"s << std::endl;
            }
            else if (input == "create_file"s)
            {
                createFile = true;
            }
            else
            {
                std::cout << "\'" << input << "\' is not a valid command. Try again.";
            }

            std::cout << std::endl;
        }

        std::string fileName;
        std::cout << "Name your file: ";
        std::getline(std::cin, fileName);

        writeFile(fileName, image);

        std::string quitCheck;
        std::cout << "File created." << std::endl;
        do 
        {
            std::cout << "Quit? (y/n)";
            std::getline(std::cin, quitCheck);
        } while (quitCheck != "y"s && quitCheck != "n"s);

        if (quitCheck == "y"s)
        {
            quitted = true;
        }
    }
 
    return 0;
}