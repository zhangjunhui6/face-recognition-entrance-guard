#include <iostream>
#include "acl/acl.h"
#include "dvpp_jpegd.h"
#include "utils.h"
using namespace std;


/* 参数初始化 */
DvppJpegD::DvppJpegD(aclrtStream& stream, acldvppChannelDesc * dvppChannelDesc)
: stream_(stream), dvppChannelDesc_(dvppChannelDesc),
decodeOutBufferDev_(nullptr), decodeOutputDesc_(nullptr)
{
}

DvppJpegD::~DvppJpegD()
{
    DestroyDecodeResource();
}

/* 初始化输出描述 */
Result DvppJpegD::InitDecodeOutputDesc(ImageData& inputImage)
{
    uint32_t decodeOutWidthStride = ALIGN_UP128(inputImage.width);
    uint32_t decodeOutHeightStride = ALIGN_UP16(inputImage.height);
    if (decodeOutWidthStride == 0 || decodeOutHeightStride == 0) {
        ERROR_LOG("初始化解码输出描述失败");
        return FAILED;
    }

    /* 预测输出图片大小 */
    uint32_t decodeOutBufferSize = 0;
    acldvppJpegPredictDecSize(inputImage.data.get(), inputImage.size, PIXEL_FORMAT_YUV_SEMIPLANAR_420, &decodeOutBufferSize);

    /* 申请内存存放输出图片 */
    aclError aclRet = acldvppMalloc(&decodeOutBufferDev_, decodeOutBufferSize);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("申请dvpp内存失败，错误码为:%d", aclRet);
        return FAILED;
    }

    /* 创建输出图片描述信息 */
    decodeOutputDesc_ = acldvppCreatePicDesc();
    if (decodeOutputDesc_ == nullptr) {
        ERROR_LOG("创建JPEG解码输出图片描述失败");
        return FAILED;
    }

    /* 设置输出图片描述属性 */
    acldvppSetPicDescData(decodeOutputDesc_, decodeOutBufferDev_);
    acldvppSetPicDescFormat(decodeOutputDesc_, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    acldvppSetPicDescWidth(decodeOutputDesc_, inputImage.width);
    acldvppSetPicDescHeight(decodeOutputDesc_, inputImage.height);
    acldvppSetPicDescWidthStride(decodeOutputDesc_, decodeOutWidthStride);
    acldvppSetPicDescHeightStride(decodeOutputDesc_, decodeOutHeightStride);
    acldvppSetPicDescSize(decodeOutputDesc_, decodeOutBufferSize);
    return SUCCESS;
}

/* Dvpp Jpeg 解码程序 */
Result DvppJpegD::Process(ImageData& dest, ImageData& src)
{

    /* 准备输出 */
    int ret = InitDecodeOutputDesc(src);
    if (ret != SUCCESS) {
        ERROR_LOG("JPEG图片解码输出准备失败!");
        return FAILED;
    }

    /* 执行异步解码 */
    aclError aclRet = acldvppJpegDecodeAsync(dvppChannelDesc_,
    reinterpret_cast < void * > (src.data.get()),
    src.size, decodeOutputDesc_, stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp异步JPEG图片解码失败，错误码为:%d", aclRet);
        return FAILED;
    }

    /* 同步等待 */
    aclRet = aclrtSynchronizeStream(stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp JPEG图片解码同步等待失败，错误码为:%d", aclRet);
        return FAILED;
    }

    dest.width = ALIGN_UP128(src.width);
    dest.height = ALIGN_UP16(src.height);
    dest.alignWidth = ALIGN_UP128(src.width);
    dest.alignHeight = ALIGN_UP16(src.height);
    dest.size = YUV420SP_SIZE(dest.alignWidth, dest.alignHeight);
    dest.data = SHARED_PRT_DVPP_BUF(decodeOutBufferDev_);

    return SUCCESS;
}

/* 销毁输出描述 */
void DvppJpegD::DestroyDecodeResource()
{
    if (decodeOutputDesc_ != nullptr) {
        acldvppDestroyPicDesc(decodeOutputDesc_);
        decodeOutputDesc_ = nullptr;
    }
}