#include <iostream>
#include "acl/acl.h"
#include "dvpp_resize.h"
#include "dvpp_jpegd.h"
#include "dvpp_jpege.h"
#include "dvpp_cropandpaste.h"
#include "dvpp_process.h"

using namespace std;

/* 初始化参数 */
DvppProcess::DvppProcess():isInitOk_(false), dvppChannelDesc_(nullptr)
{
}

DvppProcess::~DvppProcess()
{
    DestroyResource();
}

/* 资源初始化 */
Result DvppProcess::InitResource(aclrtStream& stream)
{
    aclError aclRet;
    // 创建通道描述
    dvppChannelDesc_ = acldvppCreateChannelDesc();
    if (dvppChannelDesc_ == nullptr) {
        ERROR_LOG("Dvpp创建通道描述失败");
        return FAILED;
    }

    // 创建通道
    aclRet = acldvppCreateChannel(dvppChannelDesc_);
    if (aclRet != ACL_ERROR_NONE) {
        ERROR_LOG("Dvpp创建通道失败，错误码为:%d\n", aclRet);
        return FAILED;
    }

    // 指定stream
    stream_ = stream;
    isInitOk_ = true;

    return SUCCESS;
}

/* 销毁资源 */
void DvppProcess::DestroyResource()
{
    aclError aclRet;
    if (dvppChannelDesc_ != nullptr) {
        // 销毁通道
        aclRet = acldvppDestroyChannel(dvppChannelDesc_);
        if (aclRet != ACL_ERROR_NONE) {
            ERROR_LOG("Dvpp销毁通道失败，错误码为:%d", aclRet);
        }

        // 销毁通道描述
        (void)acldvppDestroyChannelDesc(dvppChannelDesc_);
        dvppChannelDesc_ = nullptr;
    }
    INFO_LOG("Dvpp资源销毁成功");
}

/* 图片放缩 */
Result DvppProcess::Resize(ImageData& dest, ImageData& src, uint32_t width, uint32_t height) {
    DvppResize resizeOp(stream_, dvppChannelDesc_, width, height);
    return resizeOp.Process(dest, src);
}

/* 抠图并粘贴 */
Result DvppProcess::CropAndPasteImage(ImageData& dest, ImageData& src, uint32_t lt_horz, uint32_t lt_vert, uint32_t rb_horz, uint32_t rb_vert) {
    DvppCropAndPaste cropAndPaste(stream_, dvppChannelDesc_, lt_horz, lt_vert, rb_horz, rb_vert);
    return cropAndPaste.Process(dest, src);
}

/* JPEG图片编码 */
Result DvppProcess::CvtYuv420spToJpeg(ImageData& dest, ImageData& src) {
    DvppJpegE jpegE(stream_, dvppChannelDesc_);
    return jpegE.Process(dest, src);
}

/* JPEG图片解码 */
Result DvppProcess::CvtJpegToYuv420sp(ImageData& dest, ImageData& src) {
    DvppJpegD jpegD(stream_, dvppChannelDesc_);
    return jpegD.Process(dest, src);
}