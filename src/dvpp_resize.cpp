#include <iostream>
#include "acl/acl.h"
#include "utils.h"
#include "dvpp_resize.h"
using namespace std;

static int memcount = 0;
#define SHARED_PRT_SIZE_DVPP_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { memcount--;  acldvppFree(p); }))


/* 初始化参数 */
DvppResize::DvppResize(aclrtStream& stream, acldvppChannelDesc * dvppChannelDesc, uint32_t width, uint32_t height)
: stream_(stream), dvppChannelDesc_(dvppChannelDesc),
resizeConfig_(nullptr), vpcInputDesc_(nullptr), vpcOutputDesc_(nullptr),
vpcOutBufferDev_(nullptr), vpcOutBufferSize_(0){
    size_.width = width;
    size_.height = height;
}

/* 析构函数:释放资源 */
DvppResize::~DvppResize()
{
    DestroyResizeResource();
}

/* 准备输入 */
Result DvppResize::InitResizeInputDesc(ImageData& inputImage)
{
    uint32_t alignWidth = ALIGN_UP16(inputImage.width);
    uint32_t alignHeight = ALIGN_UP2(inputImage.height);

    if (alignWidth == 0 || alignHeight == 0) {
        ERROR_LOG("Dvpp图片放缩输入准备失败，原因是原图宽:%d,高:%d,但对齐宽:%d,高:%d",
        inputImage.width, inputImage.height, alignWidth, alignHeight);
        return FAILED;
    }

    /* 创建输入图片描述 */
    vpcInputDesc_ = acldvppCreatePicDesc();
    if (vpcInputDesc_ == nullptr) {
        ERROR_LOG("Dvpp创建输出图片描述失败");
        return FAILED;
    }

    /* 设置输入图片描述属性 */
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

/* 准备输出 */
Result DvppResize::InitResizeOutputDesc()
{
    int resizeOutWidth = size_.width;
    int resizeOutHeight = size_.height;
    int resizeOutWidthStride = ALIGN_UP16(resizeOutWidth);
    int resizeOutHeightStride = ALIGN_UP2(resizeOutHeight);
    if (resizeOutWidthStride == 0 || resizeOutHeightStride == 0) {
        ERROR_LOG("Dvpp输出准备失败，图片对齐宽或高为0");
        return FAILED;
    }

    // 申请dvpp内存存放输出图片
    vpcOutBufferSize_ = YUV420SP_SIZE(resizeOutWidthStride, resizeOutHeightStride);
    aclError aclRet = acldvppMalloc(&vpcOutBufferDev_, vpcOutBufferSize_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp申请内存存放缩放输出图片失败，错误码为:%d\n", aclRet);
        return FAILED;
    }
    memcount++;

    // 创建输出图片描述
    vpcOutputDesc_ = acldvppCreatePicDesc();
    if (vpcOutputDesc_ == nullptr) {
        ERROR_LOG("Dvpp创建输出图片描述失败");
        return FAILED;
    }

    // 设置输出图片描述属性
    acldvppSetPicDescData(vpcOutputDesc_, vpcOutBufferDev_);
    acldvppSetPicDescFormat(vpcOutputDesc_, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    acldvppSetPicDescWidth(vpcOutputDesc_, resizeOutWidth);
    acldvppSetPicDescHeight(vpcOutputDesc_, resizeOutHeight);
    acldvppSetPicDescWidthStride(vpcOutputDesc_, resizeOutWidthStride);
    acldvppSetPicDescHeightStride(vpcOutputDesc_, resizeOutHeightStride);
    acldvppSetPicDescSize(vpcOutputDesc_, vpcOutBufferSize_);

    return SUCCESS;
}

/* 初始化图片缩放资源 */
Result DvppResize::InitResizeResource(ImageData& inputImage) {
    if (SUCCESS != InitResizeInputDesc(inputImage)) {
        ERROR_LOG("准备缩放输入失败");
        return FAILED;
    }

    if (SUCCESS != InitResizeOutputDesc()) {
        ERROR_LOG("准备缩放输出失败");
        return FAILED;
    }

    // 创建缩放配置
    resizeConfig_ = acldvppCreateResizeConfig();
    if (resizeConfig_ == nullptr) {
        ERROR_LOG("Dvpp创建缩放配置失败");
    }

    return SUCCESS;
}

/* 销毁资源 */
void DvppResize::DestroyResizeResource()
{
    if (vpcInputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(vpcInputDesc_);
        vpcInputDesc_ = nullptr;
    }

    if (vpcOutputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(vpcOutputDesc_);
        vpcOutputDesc_ = nullptr;
    }

    if (resizeConfig_ != nullptr) {
        (void)acldvppDestroyResizeConfig(resizeConfig_);
        resizeConfig_ = nullptr;
    }
}

/* Dvpp缩放程序*/
Result DvppResize::Process(ImageData& resizedImage, ImageData& srcImage)
{
    // 资源初始化
    if (SUCCESS != InitResizeResource(srcImage)) {
        ERROR_LOG("Dvpp图片缩放资源初始化失败");
        return FAILED;
    }

    // 执行异步缩放推理
    aclError aclRet = acldvppVpcResizeAsync(dvppChannelDesc_, vpcInputDesc_,
    vpcOutputDesc_, resizeConfig_, stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp执行异步缩放推理失败，错误码为:%d\n", aclRet);
        return FAILED;
    }

    // 同步等待
    aclRet = aclrtSynchronizeStream(stream_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp图片缩放同步等待失败，错误码为:%d\n", aclRet);
        return FAILED;
    }

    resizedImage.width = size_.width;
    resizedImage.height = size_.height;
    resizedImage.alignWidth = ALIGN_UP16(size_.width);
    resizedImage.alignHeight = ALIGN_UP2(size_.height);
    resizedImage.size = vpcOutBufferSize_;
    resizedImage.data = SHARED_PRT_SIZE_DVPP_BUF(vpcOutBufferDev_);

    return SUCCESS;
}
