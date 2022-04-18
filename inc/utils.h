#pragma once
#include <iostream>
#include <vector>
#include "acl/acl.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"
#include <memory>

using namespace std;


#define INFO_LOG(fmt, args...) fprintf(stdout, "[INFO]  " fmt "\n", ##args)
#define WARN_LOG(fmt, args...) fprintf(stdout, "[WARN]  " fmt "\n", ##args)
#define ERROR_LOG(fmt, args...) fprintf(stdout, "[ERROR]  " fmt "\n", ##args)

#define RGBU8_IMAGE_SIZE(width, height) ((width) * (height) * 3)
#define YUV420SP_SIZE(width, height) ((width) * (height) * 3 / 2)

#define ALIGN_UP(num, align) (((num) + (align) - 1) & ~((align) - 1))
#define ALIGN_UP2(num) ALIGN_UP(num, 2)
#define ALIGN_UP16(num) ALIGN_UP(num, 16)
#define ALIGN_UP128(num) ALIGN_UP(num, 128)

#define SHARED_PRT_DVPP_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { acldvppFree(p); }))
#define SHARED_PRT_U8_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { delete[](p); }))


template < class Type >
shared_ptr < Type > MakeSharedNoThrow() {
    try {
        return make_shared < Type >();
    }
    catch (...) {
        return nullptr;
    }
    return nullptr;
}

#define MAKE_SHARED_NO_THROW(memory, memory_type) \
    do { \
            memory = MakeSharedNoThrow<memory_type>(); \
    }while(0);

typedef enum Result {
    FAILED = 0,
    SUCCESS = 1
} Result;

/* 相机分辨率 */
struct Resolution {
    uint32_t width = 0;
    uint32_t height = 0;
};

struct ImageData {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t alignWidth = 0;
    uint32_t alignHeight = 0;
    uint32_t size = 0;
    uint32_t input = 0;
    shared_ptr < uint8_t > data;
};

struct Rect {
    uint32_t ltX;
    uint32_t ltY;
    uint32_t rbX;
    uint32_t rbY;
};

struct BBox {
    Rect rect;
    uint32_t score;
    string text;
};

struct QueueNode
{
    uint32_t workType;
    void * image;
};

/**
 * Utils
 */

class Utils {
    public:
    static bool IsDirectory(const string & path);

    static bool IsPathExist(const string & path);

    static void SplitPath(const string & path, vector < string > & path_vec);

    static void GetAllFiles(const string & path, vector < string > & file_vec);

    static void GetPathFiles(const string & path, vector < string > & file_vec);

    static void* CopyDataToDevice(void* data, uint32_t dataSize, aclrtMemcpyKind policy);
    static void* CopyDataDeviceToLocal(void* deviceData, uint32_t dataSize);
    static void* CopyDataHostToDevice(void* deviceData, uint32_t dataSize);
    static void* CopyDataDeviceToDevice(void* deviceData, uint32_t dataSize);
    static void* CopyDataToDVPP(void* data, uint32_t dataSize);
    static Result CopyImageDataToDVPP(ImageData& imageDevice, ImageData srcImage, aclrtRunMode mode);
    static int ReadImageFile(ImageData& image, string fileName);
    static Result CopyImageDataToDevice(ImageData& imageDevice, ImageData srcImage, aclrtRunMode mode);
    static void GetChannelID(const string & channelName, int & channelID);
    static void* SaveJpegImage(ImageData& image);
    static void SaveBinFile(const char* filename, void* data, uint32_t size);
    static Result CopyDeviceToLocal(ImageData& imageout, ImageData srcImage, aclrtRunMode mode);
    static void* GetInferenceOutputItem(uint32_t& itemDataSize, aclmdlDataset* inferenceOutput, uint32_t idx);
    static int memcpy_s(void * det, size_t detSize, const void * src, size_t srcSize);

    static bool WriteToFile(const char * fileName, const void * dataDev, uint32_t dataSize);
};