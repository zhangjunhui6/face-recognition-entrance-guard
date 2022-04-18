#pragma once
#include <cstdint>

#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "utils.h"

/**
 * Dvpp JPEG 图片编码
 */
class DvppJpegE {
    public:
    /**
    * @brief 构造函数
    * @param [in] stream
    * @param [in] dvppChannelDesc：Dvpp 通道描述
    */
    DvppJpegE(aclrtStream & stream, acldvppChannelDesc* dvppChannelDesc);

    /**
    * @brief 析构函数
    */
    ~DvppJpegE();

    /**
    * @brief 编码处理程序
    */
    Result Process(ImageData& destJpegImage, ImageData& srcYuvImage);

    private:
    Result InitJpegEResource(ImageData& inputImage);
    Result InitEncodeInputDesc(ImageData& inputImage);

    void DestroyJpegEResource();
    private:
    aclrtStream stream_;
    acldvppChannelDesc* dvppChannelDesc_;

    acldvppJpegeConfig* jpegeConfig_;

    uint32_t encodeOutBufferSize_;
    void* encodeOutBufferDev_;

    acldvppPicDesc* encodeInputDesc_;
};

