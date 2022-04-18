#pragma once
#include <cstdint>

#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "utils.h"

/**
 * Dvpp图片缩放
 */
class DvppResize {
    public:
    /**
    * @brief 构造函数
    * @param [in] stream: stream
    * @param [in] dvppChannelDesc: dvpp 通道描述
    * @param [in] width:宽
    * @param [in] height:高
    */
    DvppResize(aclrtStream & stream, acldvppChannelDesc * dvppChannelDesc, uint32_t width, uint32_t height);

    /**
    * @brief 析构函数
    */
    ~DvppResize();

    /**
    * @brief 缩放处理程序
    */
    Result Process(ImageData& resizedImage, ImageData& srcImage);

    private:
    Result InitResizeResource(ImageData& inputImage);
    Result InitResizeInputDesc(ImageData& inputImage);
    Result InitResizeOutputDesc();

    void DestroyResizeResource();

    private:
    aclrtStream stream_;
    acldvppChannelDesc * dvppChannelDesc_;

    Resolution size_;
    acldvppResizeConfig * resizeConfig_;

    acldvppPicDesc * vpcInputDesc_;
    acldvppPicDesc * vpcOutputDesc_;

    uint32_t vpcOutBufferSize_;
    void * vpcOutBufferDev_;

};
