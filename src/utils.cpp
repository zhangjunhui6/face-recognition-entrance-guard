#include "utils.h"

#include <map>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"

namespace {
    const string kImagePathSeparator = ",";
    const int kStatSuccess = 0;
    const string kFileSperator = "/";
    const string kPathSeparator = "/";
    // 输出图像前缀
    const string kOutputFilePrefix = "out_";
}

/* 判断是否是目录 */
bool Utils::IsDirectory(const string & path) {
    // get path stat stat结构体是文件(夹)信息的结构体
    struct stat buf;
    if (stat(path.c_str(), &buf) != kStatSuccess) {
        return false;
    }

    // check
    return S_ISDIR(buf.st_mode);
}

/* 判断路径是否存在 */
bool Utils::IsPathExist(const string & path) {
    ifstream file(path);
    //以读模式打开文件
    if (!file) {
        return false;
    }
    return true;
}

/* 以','为分隔符剪切路径字符串 */
void Utils::SplitPath(const string & path, vector < string > & path_vec) {
    char * char_path = const_cast < char* > (path.c_str());
    const char * char_split = kImagePathSeparator.c_str();
    char * tmp_path = strtok(char_path, char_split);
    while (tmp_path) {
        path_vec.emplace_back(tmp_path);
        tmp_path = strtok(nullptr, char_split);
    }
}

/* 根据通道名称获取通道ID */
void Utils::GetChannelID(const string & channelName, int & channelID) {
    channelID = 0xFF;
    if ("Channel-1" == channelName)
        channelID = 1;

    if ("Channel-0" == channelName)
        channelID = 0;
    return ;
}

/* 获取路径中的所有文件名 */
void Utils::GetPathFiles(const string & path, vector < string > & file_vec) {
    struct dirent * dirent_ptr = nullptr;
    DIR * dir = nullptr;
    if (IsDirectory(path)) {
        dir = opendir(path.c_str());
        while ((dirent_ptr = readdir(dir)) != nullptr) {
            // 跳过 .
            if (dirent_ptr->d_name[0] == '.') {
                continue;
            }

            // 文件路径
            string full_path = path + kPathSeparator + dirent_ptr->d_name;
            // 目录需要递归
            if (IsDirectory(full_path)) {
                GetPathFiles(full_path, file_vec);
            } else {
                file_vec.emplace_back(full_path);
            }
        }
    }
    else {
        file_vec.emplace_back(path);
    }
}

void* Utils::CopyDataDeviceToLocal(void* deviceData, uint32_t dataSize) {
    uint8_t* buffer = new uint8_t[dataSize];
    if (buffer == nullptr) {
        ERROR_LOG("New数组失败\n");
        return nullptr;
    }

    aclError aclRet = aclrtMemcpy(buffer, dataSize, deviceData, dataSize, ACL_MEMCPY_DEVICE_TO_DEVICE);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("复制设备数据到本地失败，aclRet是:%d\n", aclRet);
        delete[](buffer);
        return nullptr;
    }

    return (void*)buffer;
}

void* Utils::CopyDataToDevice(void* data, uint32_t dataSize, aclrtMemcpyKind policy) {
    void* buffer = nullptr;
    aclError aclRet = aclrtMalloc(&buffer, dataSize, ACL_MEM_MALLOC_HUGE_FIRST);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("申请设备数据缓冲区失败，aclRet为 %d\n", aclRet);
        return nullptr;
    }

    aclRet = aclrtMemcpy(buffer, dataSize, data, dataSize, policy);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("将数据复制到设备失败，aclRet为 %d\n", aclRet);
        (void)aclrtFree(buffer);
        return nullptr;
    }

    return buffer;
}

/* 将数据保存为二进制文件 */
void Utils::SaveBinFile(const char* filename, void* data, uint32_t size) {
    FILE * outFileFp = fopen(filename, "wb+");
    if (outFileFp == nullptr) {
        ERROR_LOG("由于打开文件错误，保存文件 %s 失败\n", filename);
        return ;
    }
    fwrite(data, 1, size, outFileFp);

    fflush(outFileFp);
    fclose(outFileFp);
}

/* 将 Jpg 图片数据保存到文件 */
void* Utils::SaveJpegImage(ImageData& image)
{
    static uint32_t yuv_cnt = 0;
    char filename[32];

    snprintf(filename, 32, "%d.yuv", ++yuv_cnt);

    void* buffer = CopyDataDeviceToDevice(image.data.get(), image.size);
    SaveBinFile(filename, (char*)buffer, image.size);
    aclrtFree(buffer);

    return nullptr;
}

void* Utils::CopyDataToDVPP(void* data, uint32_t dataSize) {
    void* buffer = nullptr;
    INFO_LOG("复制数据到Dvpp开始 \n");
    aclError aclRet = acldvppMalloc(&buffer, dataSize);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("申请设备数据缓冲区失败，aclRet 为 %d\n", aclRet);
        return nullptr;
    }

    aclRet = aclrtMemcpy(buffer, dataSize, data, dataSize, ACL_MEMCPY_DEVICE_TO_DEVICE);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("将数据复制到设备失败，aclRet 为 %d\n", aclRet);
        (void)aclrtFree(buffer);
        return nullptr;
    }

    return buffer;
}

Result Utils::CopyImageDataToDVPP(ImageData& imageDevice, ImageData srcImage, aclrtRunMode mode) {
    void * buffer;
    if (mode == ACL_HOST)
        buffer = Utils::CopyDataToDVPP(srcImage.data.get(), srcImage.size);
    else
        buffer = Utils::CopyDataToDVPP(srcImage.data.get(), srcImage.size);

    if (buffer == nullptr) {
        ERROR_LOG("将图像复制到设备失败\n");
        return FAILED;
    }

    imageDevice.width = srcImage.width;
    imageDevice.height = srcImage.height;
    imageDevice.size = srcImage.size;
    imageDevice.data.reset((uint8_t*)buffer, [](uint8_t* p) {
        aclrtFree((void *)p);
    });

    return SUCCESS;
}

Result Utils::CopyDeviceToLocal(ImageData& imageout, ImageData srcImage, aclrtRunMode mode) {
    void * buffer;
    if (mode == ACL_HOST)
        buffer = Utils::CopyDataDeviceToLocal(srcImage.data.get(), srcImage.size);
    else
        buffer = Utils::CopyDataDeviceToLocal(srcImage.data.get(), srcImage.size);

    if (buffer == nullptr) {
        ERROR_LOG("复制设备数据到本地失败\n");
        return FAILED;
    }

    imageout.width = srcImage.width;
    imageout.height = srcImage.height;
    imageout.size = srcImage.size;
    imageout.data.reset((uint8_t*)buffer, [](uint8_t* p) {
        delete[](p);
    });

    return SUCCESS;
}

void* Utils::CopyDataDeviceToDevice(void* deviceData, uint32_t dataSize) {
    return CopyDataToDevice(deviceData, dataSize, ACL_MEMCPY_DEVICE_TO_DEVICE);
}

void* Utils::CopyDataHostToDevice(void* deviceData, uint32_t dataSize) {
    return CopyDataToDevice(deviceData, dataSize, ACL_MEMCPY_HOST_TO_DEVICE);
}

Result Utils::CopyImageDataToDevice(ImageData& imageDevice, ImageData srcImage, aclrtRunMode mode) {
    void * buffer;
    if (mode == ACL_HOST)
        buffer = Utils::CopyDataHostToDevice(srcImage.data.get(), srcImage.size);
    else
        buffer = Utils::CopyDataDeviceToDevice(srcImage.data.get(), srcImage.size);

    if (buffer == nullptr) {
        ERROR_LOG("将图像数据复制到设备失败\n");
        return FAILED;
    }

    imageDevice.width = srcImage.width;
    imageDevice.height = srcImage.height;
    imageDevice.size = srcImage.size;
    imageDevice.data.reset((uint8_t*)buffer, [](uint8_t* p) {
        aclrtFree((void *)p);
    });

    return SUCCESS;
}

/* 读取文件内容*/
int Utils::ReadImageFile(ImageData& image, string fileName)
{
    struct stat sBuf;
    int fileStatus = stat(fileName.data(), &sBuf);
    if (fileStatus == -1) {
        ERROR_LOG("获取文件失败\n");
        return FAILED;
    }
    if (S_ISREG(sBuf.st_mode) == 0) {
        ERROR_LOG("%s不是文件，请输入文件", fileName.c_str());
        return FAILED;
    }
    /* 打开二进制文件 */
    ifstream binFile(fileName, ifstream::binary);
    if (binFile.is_open() == false) {
        ERROR_LOG("打开文件 %s 失败\n", fileName.c_str());
        return FAILED;
    }

    /* 获取文件内容长度 */
    binFile.seekg(0, binFile.end);
    uint32_t binFileBufferLen = binFile.tellg();
    if (binFileBufferLen == 0) {
        ERROR_LOG("二进制文件为空，文件名是 %s\n", fileName.c_str());
        binFile.close();
        return FAILED;
    }

    binFile.seekg(0, binFile.beg);
    /* 读取文件数据 */
    uint8_t* binFileBufferData = new(nothrow)uint8_t[binFileBufferLen];
    if (binFileBufferData == nullptr) {
        ERROR_LOG("申请二进制文件缓存失败\n");
        binFile.close();
        return FAILED;
    }
    binFile.read((char *)binFileBufferData, binFileBufferLen);
    binFile.close();

    /* 获取信息并将其保存在图像中 */
    int32_t ch = 0;
    acldvppJpegGetImageInfo(binFileBufferData, binFileBufferLen,
    &(image.width), &(image.height), &ch);
    image.data.reset(binFileBufferData, [](uint8_t* p) {
        delete[](p);
    });
    image.size = binFileBufferLen;

    return SUCCESS;
}

void* GetInferenceOutputItem(uint32_t& itemDataSize, aclmdlDataset* inferenceOutput, uint32_t idx) {
    aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(inferenceOutput, idx);
    if (dataBuffer == nullptr) {
        ERROR_LOG("从模型推理输出中获取第%d个数据集缓冲区失败\n", idx);
        return nullptr;
    }

    void* dataBufferDev = aclGetDataBufferAddr(dataBuffer);
    if (dataBufferDev == nullptr) {
        ERROR_LOG("从模型推理输出中获取第%d个数据集缓冲区地址失败\n", idx);
        return nullptr;
    }

    size_t bufferSize = aclGetDataBufferSizeV2(dataBuffer);
    if (bufferSize == 0) {
        ERROR_LOG("模型推理输出的第%d个数据集缓冲区大小为 0\n", idx);
        return nullptr;
    }

    void* data = nullptr;
    data = dataBufferDev;

    itemDataSize = bufferSize;
    return data;
}

int Utils::memcpy_s(void * det, size_t detSize, const void * src, size_t srcSize)
{
    uint8_t errorcode = 0;
    if (srcSize > detSize || src == NULL || det == NULL)
    {
        if (srcSize > detSize)
            errorcode = 1;
        else
            if (src == NULL)
                errorcode = 2;
            else
                if (det == NULL)
                    errorcode = 3;
        fflush(stdout);
        return FAILED;
    }
    else
        memcpy(det, src, srcSize);
    return SUCCESS;
}

bool Utils::WriteToFile(const char * fileName, const void * dataDev, uint32_t dataSize)
{

    FILE * outFileFp = fopen(fileName, "wb");

    bool ret = true;
    size_t writeRet = fwrite(dataDev, 1, dataSize, outFileFp);
    if (writeRet != dataSize) {
        ret = false;
    }
    fflush(outFileFp);
    fclose(outFileFp);
    return ret;
}
