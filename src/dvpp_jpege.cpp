#include <iostream>
#include "acl/acl.h"
#include "dvpp_jpege.h"
using namespace std;

static int memcount = 0;
#define SHARED_PRT_JPEGE_DVPP_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { memcount--;  acldvppFree(p); }))


/* 初始化参数 */
DvppJpegE::DvppJpegE(aclrtStream& stream, acldvppChannelDesc* dvppChannelDesc)
: stream_(stream), dvppChannelDesc_(dvppChannelDesc), jpegeConfig_(nullptr), encodeOutBufferSize_(0),
encodeOutBufferDev_(nullptr), encodeInputDesc_(nullptr){

}

DvppJpegE::~DvppJpegE() {
    DestroyJpegEResource();
}

/* 准备输入 */
Result DvppJpegE::InitEncodeInputDesc(ImageData& inputImage)
{
    uint32_t alignWidth = ALIGN_UP16(inputImage.width);
    uint32_t alignHeight = ALIGN_UP2(inputImage.height);
    if (alignWidth == 0 || alignHeight == 0) {
        ERROR_LOG("Dvpp JPEG图片解码输入准备失败，原因是原图宽:%d,高:%d,但对齐宽:%d,高:%d",
        inputImage.width, inputImage.height, alignWidth, alignHeight);
        return FAILED;
    }

    // 创建输入图片描述
    encodeInputDesc_ = acldvppCreatePicDesc();
    if (encodeInputDesc_ == nullptr) {
        ERROR_LOG("Dvpp JPEG图片编码创建输入图片描述失败");
    }

    // 设置输入图片描述属性
    acldvppSetPicDescData(encodeInputDesc_, reinterpret_cast < void * > (inputImage.data.get()));
    acldvppSetPicDescFormat(encodeInputDesc_, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    acldvppSetPicDescWidth(encodeInputDesc_, inputImage.width);
    acldvppSetPicDescHeight(encodeInputDesc_, inputImage.height);
    acldvppSetPicDescWidthStride(encodeInputDesc_, alignWidth);
    acldvppSetPicDescHeightStride(encodeInputDesc_, alignHeight);
    uint32_t inputBufferSize = YUV420SP_SIZE(alignWidth, alignHeight);
    acldvppSetPicDescSize(encodeInputDesc_, inputBufferSize);

    return SUCCESS;
}

/* 资源初始化 */
Result DvppJpegE::InitJpegEResource(ImageData& inputImage) {
    /* 创建编码配置数据类型，并设置编码配置数据 */
    uint32_t encodeLevel = 50;
    jpegeConfig_ = acldvppCreateJpegeConfig();
    acldvppSetJpegeConfigLevel(jpegeConfig_, encodeLevel);

    /* 准备输入 */
    if (SUCCESS != InitEncodeInputDesc(inputImage)) {
        ERROR_LOG("JPEG图片编码输入准备失败");
        return FAILED;
    }

    /* 预估输出图片大小 */
    aclError aclRet = acldvppJpegPredictEncSize(encodeInputDesc_, jpegeConfig_, &encodeOutBufferSize_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("JPEG图片编码预估输出图片大小失败，错误码为:%d", aclRet);
        return FAILED;
    }

    /* 申请dvpp内存存放输出图片 */
    memcount++;
    aclRet = acldvppMalloc(&encodeOutBufferDev_, encodeOutBufferSize_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("JPEG图片编码申请内存存放输出图片失败，错误码为:%d", aclRet);
        return FAILED;
    }
    return SUCCESS;
}

/* Dvpp JPEG图片编码程序 */
Result DvppJpegE::Process(ImageData& destJpegImage, ImageData& srcYuvImage)
{
    /* 资源初始化 */
    if (SUCCESS != InitJpegEResource(srcYuvImage)) {
        ERROR_LOG("JPEG图片编码资源初始化失败!");
        return FAILED;
    }

    /* 执行异步JPEG编码推理 */
    aclError aclRet = acldvppJpegEncodeAsync(dvppChannelDesc_, encodeInputDesc_,
    encodeOutBufferDev_, &encodeOutBufferSize_, jpegeConfig_, stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("JPEG图片编码异步推理失败，错误码为:%d", aclRet);
        return FAILED;
    }

    /* 同步等待 */
    aclRet = aclrtSynchronizeStream(stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("JPEG图片编码同步等待失败，错误码为:%d\n", aclRet);
        return FAILED;
    }

    destJpegImage.width = srcYuvImage.width;
    destJpegImage.height = srcYuvImage.height;
    destJpegImage.size = encodeOutBufferSize_;
    destJpegImage.data = SHARED_PRT_JPEGE_DVPP_BUF(encodeOutBufferDev_);

    return SUCCESS;
}

/* 销毁资源 */
void DvppJpegE::DestroyJpegEResource() {
    /* 销毁图片编码配置数据 */
    if (jpegeConfig_ != nullptr) {
        (void)acldvppDestroyJpegeConfig(jpegeConfig_);
        jpegeConfig_ = nullptr;
    }

    /* 销毁图片输入描述 */
    if (encodeInputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(encodeInputDesc_);
        encodeInputDesc_ = nullptr;
    }
}