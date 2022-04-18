#pragma once
#include <cstdint>

#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "utils.h"

/**
 * Dvpp抠图粘贴
 */

class DvppCropAndPaste {
    public:
    /**
    * @brief 构造函数
    * @param [in] stream: stream
    * @param [in] dvppChannelDesc: dvpp 通道描述
    * @param [in] lt_horz,lt_vert，rb_horz,rb_vert: 抠图区域
    */
    DvppCropAndPaste(aclrtStream & stream, acldvppChannelDesc * dvppChannelDesc,
    uint32_t lt_horz, uint32_t lt_vert, uint32_t rb_horz, uint32_t rb_vert);

    /**
    * @brief 析构函数
    */
    ~DvppCropAndPaste();

    /**
    * @brief dvpp 处理程序
    */
    Result Process(ImageData& resizedImage, ImageData& srcImage);

    private:
    Result InitCropAndPasteResource(ImageData& inputImage);
    Result InitCropAndPasteInputDesc(ImageData& inputImage);
    Result InitCropAndPasteOutputDesc();

    void DestroyCropAndPasteResource();

    private:
    aclrtStream stream_;
    acldvppChannelDesc * dvppChannelDesc_;

    // 输入输出描述
    acldvppPicDesc * vpcInputDesc_;
    acldvppPicDesc * vpcOutputDesc_;

    acldvppRoiConfig * cropArea_;
    acldvppRoiConfig * pasteArea_;

    // 输出缓存
    void * vpcOutBufferDev_;
    uint32_t vpcOutBufferSize_;

    Resolution size_;

    uint32_t lt_horz_;
    uint32_t rb_horz_;
    uint32_t lt_vert_;
    uint32_t rb_vert_;
};

