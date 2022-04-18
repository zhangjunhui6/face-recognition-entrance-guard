#pragma once
#include <cstdint>

#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "utils.h"

/**
 * Dvpp 图像预处理程序
 */
class DvppProcess {
    public:
    DvppProcess();
    ~DvppProcess();

    Result InitResource(aclrtStream& stream);

    Result Resize(ImageData& dest, ImageData& src, uint32_t width, uint32_t height);
    Result CropAndPasteImage(ImageData& dest, ImageData& src, uint32_t lt_horz, uint32_t lt_vert, uint32_t rb_horz, uint32_t rb_vert);
    Result CvtYuv420spToJpeg(ImageData& dest, ImageData& src);
    Result CvtJpegToYuv420sp(ImageData& dest, ImageData& src);

    protected:
    int isInitOk_;
    aclrtStream stream_;
    acldvppChannelDesc * dvppChannelDesc_;

    private:
    void DestroyResource();
};

