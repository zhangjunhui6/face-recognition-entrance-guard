#pragma once
#include <cstdint>

#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "utils.h"


/**
 * Dvpp JPEG解码
 */
class DvppJpegD {
    public:
    DvppJpegD(aclrtStream & stream, acldvppChannelDesc * dvppChannelDesc);

    ~DvppJpegD();

    Result InitDecodeOutputDesc(ImageData& inputImage);

    Result Process(ImageData& dest, ImageData& src);

    private:
    void DestroyDecodeResource();

    private:
    aclrtStream stream_;
    acldvppChannelDesc * dvppChannelDesc_;

    acldvppPicDesc * decodeOutputDesc_; //decode output desc

    void* decodeOutBufferDev_; // decode output buffer
};

