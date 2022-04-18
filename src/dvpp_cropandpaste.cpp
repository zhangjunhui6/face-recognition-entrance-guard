#include <iostream>
#include "acl/acl.h"
#include "utils.h"
#include "dvpp_cropandpaste.h"
using namespace std;

static int memcount = 0;
#define SHARED_PRT_CROP_DVPP_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { memcount--; acldvppFree(p); }))

/* 构造函数：初始化参数 */
DvppCropAndPaste::DvppCropAndPaste(aclrtStream& stream, acldvppChannelDesc * dvppChannelDesc,
uint32_t lt_horz, uint32_t lt_vert, uint32_t rb_horz, uint32_t rb_vert)
: stream_(stream), dvppChannelDesc_(dvppChannelDesc),
vpcInputDesc_(nullptr), vpcOutputDesc_(nullptr),
vpcOutBufferDev_(nullptr), vpcOutBufferSize_(0){
    size_.width = rb_horz - lt_horz;
    size_.height = rb_vert - lt_vert;
    lt_horz_ = lt_horz;
    rb_horz_ = rb_horz;
    lt_vert_ = lt_vert;
    rb_vert_ = rb_vert;
}

/* 析构函数：销毁资源 */
DvppCropAndPaste::~DvppCropAndPaste()
{
    DestroyCropAndPasteResource();
}

/* 初始化输入描述：创建输入描述数据结构，设置其属性 */
Result DvppCropAndPaste::InitCropAndPasteInputDesc(ImageData& inputImage)
{
    uint32_t alignWidth = ALIGN_UP128(inputImage.width);
    uint32_t alignHeight = ALIGN_UP16(inputImage.height);
    if (alignWidth == 0 || alignHeight == 0) {
        ERROR_LOG("初始化抠图输出描述失败，原因是图片宽:%d,高:%d;对齐后宽:%d,高:%d",
        inputImage.width, inputImage.height, alignWidth, alignHeight);
        return FAILED;
    }

    // 创建输入图片描述
    vpcInputDesc_ = acldvppCreatePicDesc();
    if (vpcInputDesc_ == nullptr) {
        ERROR_LOG("Dvpp 创建输入图片描述失败!");
        return FAILED;
    }

    // 设置输入图片描述属性
    acldvppSetPicDescData(vpcInputDesc_, inputImage.data.get());
    acldvppSetPicDescFormat(vpcInputDesc_, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    acldvppSetPicDescWidth(vpcInputDesc_, inputImage.width);
    acldvppSetPicDescHeight(vpcInputDesc_, inputImage.height);
    acldvppSetPicDescWidthStride(vpcInputDesc_, alignWidth);
    acldvppSetPicDescHeightStride(vpcInputDesc_, alignHeight);
    uint32_t inputBufferSize = YUV420SP_SIZE(alignWidth, alignHeight);
    acldvppSetPicDescSize(vpcInputDesc_, inputBufferSize);

    return SUCCESS;
}

/* 初始化输出描述：申请内存，创建输出图片描述并设置其属性 */
Result DvppCropAndPaste::InitCropAndPasteOutputDesc()
{
    int resizeOutWidth = size_.width;
    int resizeOutHeight = size_.height;
    int resizeOutWidthStride = ALIGN_UP16(resizeOutWidth);
    int resizeOutHeightStride = ALIGN_UP2(resizeOutHeight);
    if (resizeOutWidthStride == 0 || resizeOutHeightStride == 0) {
        ERROR_LOG("初始化抠图输出描述失败，原因是输出的对齐宽或高为0");
        return FAILED;
    }

    // 申请内存以存储输出图片
    aclError aclRet = acldvppMalloc(&vpcOutBufferDev_, vpcOutBufferSize_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("申请dvpp内存失败,错误码为:%d", aclRet);
        return FAILED;
    }
    memcount++;

    // 创建输出图片描述
    vpcOutputDesc_ = acldvppCreatePicDesc();
    if (vpcOutputDesc_ == nullptr) {
        ERROR_LOG("创建输出图片描述失败");
        return FAILED;
    }

    // 设置输出图像描述属性
    acldvppSetPicDescData(vpcOutputDesc_, vpcOutBufferDev_);
    acldvppSetPicDescFormat(vpcOutputDesc_, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    acldvppSetPicDescWidth(vpcOutputDesc_, resizeOutWidth);
    acldvppSetPicDescHeight(vpcOutputDesc_, resizeOutHeight);
    acldvppSetPicDescWidthStride(vpcOutputDesc_, resizeOutWidthStride);
    acldvppSetPicDescHeightStride(vpcOutputDesc_, resizeOutHeightStride);
    vpcOutBufferSize_ = YUV420SP_SIZE(resizeOutWidthStride, resizeOutHeightStride);
    acldvppSetPicDescSize(vpcOutputDesc_, vpcOutBufferSize_);

    return SUCCESS;
}

/* 初始化输入/输出描述 */
Result DvppCropAndPaste::InitCropAndPasteResource(ImageData& inputImage) {
    if (SUCCESS != InitCropAndPasteInputDesc(inputImage)) {
        ERROR_LOG("初始化抠图输入失败!");
        return FAILED;
    }

    if (SUCCESS != InitCropAndPasteOutputDesc()) {
        ERROR_LOG("初始化抠图输出失败!");
        return FAILED;
    }

    return SUCCESS;
}

/* 销毁输入输出描述以及抠图、粘贴配置 */
void DvppCropAndPaste::DestroyCropAndPasteResource()
{
    // 销毁抠图&粘贴区域位置的配置
    if (cropArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(cropArea_);
        cropArea_ = nullptr;
    }

    if (pasteArea_ != nullptr) {
        (void)acldvppDestroyRoiConfig(pasteArea_);
        pasteArea_ = nullptr;
    }

    // 销毁图片描述
    if (vpcInputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(vpcInputDesc_);
        vpcInputDesc_ = nullptr;
    }

    if (vpcOutputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(vpcOutputDesc_);
        vpcOutputDesc_ = nullptr;
    }
}

/* Dvpp 抠图粘贴处理程序 */
Result DvppCropAndPaste::Process(ImageData& destImage, ImageData& srcImage)
{

    // 初始化资源
    if (SUCCESS != InitCropAndPasteResource(srcImage)) {
        ERROR_LOG("Dvpp抠图并粘贴初始化资源失败!");
        return FAILED;
    }

    uint32_t cropLeftOffset = lt_horz_;
    uint32_t cropTopOffset = lt_vert_;
    uint32_t cropRightOffset = rb_horz_;
    uint32_t cropBottomOffset = rb_vert_;

    /* 设置抠图区域配置 */
    cropArea_ = acldvppCreateRoiConfig(cropLeftOffset, cropRightOffset, cropTopOffset, cropBottomOffset);
    if (cropArea_ == nullptr) {
        ERROR_LOG("Dvpp创建抠图区域配置失败!");
        return FAILED;
    }

    uint32_t pasteLeftOffset = 0;
    uint32_t pasteTopOffset = 0;
    uint32_t pasteRightOffset = size_.width;
    uint32_t pasteBottomOffset = size_.height;

    /* 设置黏贴区域配置 */
    pasteArea_ = acldvppCreateRoiConfig(pasteLeftOffset, pasteRightOffset, pasteTopOffset, pasteBottomOffset);
    if (pasteArea_ == nullptr) {
        ERROR_LOG("Dvpp创建粘贴区域配置失败!");
        return FAILED;
    }

    /* 执行异步抠图处理 */
    aclError aclRet = acldvppVpcCropAndPasteAsync(dvppChannelDesc_, vpcInputDesc_,
    vpcOutputDesc_, cropArea_, pasteArea_, stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp执行异步抠图粘贴失败，错误码为:%d", aclRet);
        return FAILED;
    }

    /* 同步等待 */
    aclRet = aclrtSynchronizeStream(stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp抠图粘贴同步等待失败，错误码为:%d", aclRet);
        return FAILED;
    }

    destImage.width = size_.width;
    destImage.height = size_.height;
    destImage.alignWidth = ALIGN_UP16(size_.width);
    destImage.alignHeight = ALIGN_UP2(size_.height);
    destImage.size = vpcOutBufferSize_;
    destImage.data = SHARED_PRT_CROP_DVPP_BUF(vpcOutBufferDev_);

    return SUCCESS;
}