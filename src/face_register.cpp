#include "face_register.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <regex>

#include<iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"
#include<opencv2/core/core.hpp>

#include "face_recognition_params.h"
#include "resource_load.h"

using namespace cv;
using namespace std;
using namespace ascend::presenter;
using namespace ascend::presenter::facial_recognition;

FaceRegister::FaceRegister() {
}

/* 初始化参数，注册通道 */
bool FaceRegister::Init() {
    // 获取人脸注册通道
    Channel* agent_channel = PresenterChannels::GetInstance().GetChannel();
    if (agent_channel == nullptr) {
        ERROR_LOG("创建/获取人脸注册通道失败.");
        return false;
    }

    return true;
}

/* 监听注册通道是否有信息，并处理注册信息 */
static struct timespec time1 = {
    0, 0
};
static struct timespec time2 = {
    0, 0
};
bool FaceRegister::DoRegisterProcess() {
    // 获取人脸注册通道
    Channel* agent_channel = PresenterChannels::GetInstance().GetChannel();
    if (agent_channel == nullptr) {
        ERROR_LOG("获取人脸注册通道失败.");
        return false;
    }

    bool ret = true;
    while (1) {
        // 构造注册响应消息和读取消息
        unique_ptr < google::protobuf::Message > response_rec;
        PresenterErrorCode agent_ret = agent_channel->ReceiveMessage(response_rec);
        if (agent_ret == PresenterErrorCode::kNone) {
            FaceInfo* face_register_req = (FaceInfo*)(response_rec.get());
            if (face_register_req == nullptr) {
                ERROR_LOG("[DoRegisterProcess]人脸注册请求为空");
                continue;
            }

            clock_gettime(CLOCK_REALTIME, &time1);

            // 获取用户ID 和图像
            int face_id_size = face_register_req->id().size();
            uint32_t face_image_size = face_register_req->image().size();
            const char* face_image_buffer = face_register_req->image().c_str();
            INFO_LOG("用户ID:%s,图片大小:%d", face_register_req->id().c_str(), face_image_size);

            // 准备将注册信息发送到下一个步骤
            shared_ptr < FaceRecognitionInfo > pobj = make_shared < FaceRecognitionInfo >();

            // 使用 dvpp 将 jpg 图像转换为 yuv图像
            ImageData jpgImage;
            int32_t ch = 0;
            void * buffer = nullptr;
            acldvppJpegGetImageInfo(face_image_buffer, face_image_size, &(jpgImage.width), &(jpgImage.height), &ch);

            aclError aclRet = acldvppMalloc(&buffer, face_image_size * 2);
            jpgImage.data.reset((uint8_t*)buffer, [](uint8_t* p) {
                acldvppFree((void *)p);
            });
            jpgImage.size = face_image_size;
            aclrtMemcpy(jpgImage.data.get(), face_image_size, face_image_buffer, face_image_size, ACL_MEMCPY_DEVICE_TO_DEVICE);

            ret = ResourceLoad::GetInstance().GetDvpp().CvtJpegToYuv420sp(pobj->org_img, jpgImage);
            // 失败，无需发送给服务器
            if (ret == FAILED)
            {
                ERROR_LOG("[DoRegisterProcess]无法将jpg格式转换为yuv格式");
                pobj->err_info.err_code = AppErrorCode::kRegister;
                pobj->err_info.err_msg = "无法将jpg格式转换为yuv格式";
                // 将图像处理数据发送到下一个步骤
                ResourceLoad::GetInstance().SendNextModelProcess("FaceRegister", pobj);
                continue;
            }

            // 1 保存注册的图像数据
            pobj->frame.image_source = 1;
            pobj->frame.face_id = face_register_req->id();
            pobj->frame.org_img_format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            // true 表示图像已对齐
            pobj->frame.img_aligned = true;
            pobj->err_info.err_code = AppErrorCode::kNone;

            INFO_LOG("[DoRegisterProcess]jpg图片宽：%d,高：%d；YUV图片对齐宽：%d，高：%d",
            pobj->org_img.width, pobj->org_img.height, pobj->org_img.alignWidth, pobj->org_img.alignHeight);

            ResourceLoad::GetInstance().SendNextModelProcess("FaceRegister", pobj);
            clock_gettime(CLOCK_REALTIME, &time2);
            INFO_LOG("人脸注册获取输入耗时：%ld毫秒\n",
            (time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_nsec - time1.tv_nsec) / 1000000);
        } else {
            usleep(kSleepInterval);
        }
    }
}

bool FaceRegister::Process()
{
    if (!Init()) {
        ERROR_LOG("初始化失败.");
    }
    if (!DoRegisterProcess()) {
        ERROR_LOG("获取注册输入失败.");
    }
    return true;
}
