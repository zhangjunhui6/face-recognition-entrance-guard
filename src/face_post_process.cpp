#include "face_post_process.h"
#include "utils.h"

#include <memory>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <string>
#include <ctime>
#include "resource_load.h"
#include "peripheral_ctrl.h"
#include "facial_recognition_message.pb.h"

using namespace ascend::presenter::facial_recognition;
using namespace std;
using namespace google::protobuf;

Result FacePostProcess::CheckSendMessageRes(const PresenterErrorCode & error_code) {
    if (error_code == PresenterErrorCode::kNone) {
        return SUCCESS;
    }

    INFO_LOG("发送数据到Socket服务器失败，错误码为:%d", int(error_code));
    return FAILED;
}

//获取源图像，并转换为JPG格式
Result FacePostProcess::GetOriginPic(const shared_ptr < FaceRecognitionInfo > & image_handle, ImageData & jpgImage, facial_recognition::FrameInfo & frame_info) {
    if (image_handle->frame.image_source == 0) {
        Result ret = ResourceLoad::GetInstance().GetDvpp().CvtYuv420spToJpeg(jpgImage, image_handle->org_img);
        if (ret == FAILED)
        {
            ERROR_LOG("未能将 NV12 转换为 JPEG，跳过此帧。");
            return FAILED;
        }

        frame_info.set_image(string(reinterpret_cast < char* > (jpgImage.data.get()), jpgImage.size));
    }
    return SUCCESS;
}
static int memcount = 0;

// 发送人脸识别结果
Result FacePostProcess::SendFeature(const shared_ptr < FaceRecognitionInfo > & info) {
    // 获取人脸识别通道
    Channel * channel = PresenterChannels::GetInstance().GetPresenterChannel();
    if (channel == nullptr) {
        ERROR_LOG("获取人脸识别通道失败.");
        return FAILED;
    }

    // 前面推理失败，则跳过此帧
    if (info->err_info.err_code != AppErrorCode::kNone) {
        INFO_LOG("前面推理失败，跳过此帧,错误码:%d,错误消息:%s",
        int(info->err_info.err_code), info->err_info.err_msg.c_str());
        return FAILED;
    }

    facial_recognition::FrameInfo frame_info;
    // 1. 设置源图像数据
    ImageData jpgImage;
    if (GetOriginPic(info, jpgImage, frame_info) != SUCCESS) {
        info->err_info.err_code = AppErrorCode::kRecognition;
        info->err_info.err_msg = "获取源图像失败";

        ERROR_LOG("推理失败, 错误代码为:%d,错误消息为:%s",
        int(info->err_info.err_code), info->err_info.err_msg.c_str());
    }

    // 2. 设置人脸特征信息
    vector < FaceImage > face_imgs = info->face_imgs;
    facial_recognition::FaceFeature * feature = nullptr;
    for (int i = 0; i < face_imgs.size(); i++) {
        feature = frame_info.add_feature();

        // 人脸框
        feature->mutable_box()->set_lt_x(face_imgs[i].rectangle.lt.x);
        feature->mutable_box()->set_lt_y(face_imgs[i].rectangle.lt.y);
        feature->mutable_box()->set_rb_x(face_imgs[i].rectangle.rb.x);
        feature->mutable_box()->set_rb_y(face_imgs[i].rectangle.rb.y);

        //活体指数
        feature->set_alive_score(face_imgs[i].alive_score);

        // 特征向量
        for (int j = 0; j < face_imgs[i].feature_vector.size(); j++) {
            feature->add_vector(face_imgs[i].feature_vector[j]);
        }
    }

    // 3. 设置体温值
    float temp = PeripheralCtrl::GetInstance().get_object_temp();
    frame_info.set_temp(temp);

    // 4. 发送帧数据到服务器
    unique_ptr < Message > resp;
    PresenterErrorCode error_code;
    error_code = channel->SendMessage(frame_info, resp);

    // 5. 处理响应
    string msg_name = resp->GetDescriptor()->full_name();
    CtrlInfo ctrl;
    string c_name = ctrl.GetDescriptor()->full_name();
    if (error_code == PresenterErrorCode::kNone) {
        if (c_name == msg_name) {
            CtrlInfo* ctrl_info = (CtrlInfo*)(resp.get());
            int buzzer = ctrl_info->buzzer();
            int open = ctrl_info->open();
            if (buzzer == 0) {
                PeripheralCtrl::GetInstance().set_buzzer(0);
            } else {
                PeripheralCtrl::GetInstance().set_buzzer(1);
            }
            if (open == 1) {
                PeripheralCtrl::GetInstance().set_light(0, 1, 0);
          } else {
                PeripheralCtrl::GetInstance().set_light(1, 0, 0);
            }
        }

    }

    return CheckSendMessageRes(error_code);
}

// 响应人脸注册请求
Result FacePostProcess::ReplyFeature(const shared_ptr < FaceRecognitionInfo > & info) {
    // 获取人脸注册通道
    Channel * channel = PresenterChannels::GetInstance().GetChannel();
    if (channel == nullptr) {
        ERROR_LOG("获取人脸注册通道失败.");
        return FAILED;
    }

    // 构造人脸注册结果
    facial_recognition::FaceResult result;
    result.set_id(info->frame.face_id);
    unique_ptr < Message > resp;
    INFO_LOG("后处理，开始响应人脸注册.");
    // 1. 前面推理处理失败，发送注册失败
    if (info->err_info.err_code != AppErrorCode::kNone) {
        INFO_LOG("前面推理处理失败，向服务器发送注册失败");
        result.mutable_response()->set_ret(facial_recognition::kErrorOther);
        result.mutable_response()->set_message(info->err_info.err_msg);

        PresenterErrorCode error_code = channel->SendMessage(result, resp);
        return CheckSendMessageRes(error_code);
    }

    // 2. 设置人脸特征信息
    result.mutable_response()->set_ret(facial_recognition::kErrorNone);
    vector < FaceImage > face_imgs = info->face_imgs;
    facial_recognition::FaceFeature * face_feature = nullptr;
    for (int i = 0; i < face_imgs.size(); i++) {
        face_feature = result.add_feature();

        // 人脸框
        face_feature->mutable_box()->set_lt_x(face_imgs[i].rectangle.lt.x);
        face_feature->mutable_box()->set_lt_y(face_imgs[i].rectangle.lt.y);
        face_feature->mutable_box()->set_rb_x(face_imgs[i].rectangle.rb.x);
        face_feature->mutable_box()->set_rb_y(face_imgs[i].rectangle.rb.y);

        // 活体指数
        face_feature->set_alive_score(face_imgs[i].alive_score);

        // 特征向量
        for (int j = 0; j < face_imgs[i].feature_vector.size(); j++) {
            face_feature->add_vector(face_imgs[i].feature_vector[j]);
        }
    }
    PresenterErrorCode error_code = channel->SendMessage(result, resp);
    INFO_LOG("后处理，人脸注册响应结束.");
    return CheckSendMessageRes(error_code);
}

Result FacePostProcess::Process(const shared_ptr < FaceRecognitionInfo > & image_handle) {

    // 人脸识别后处理
    if (image_handle->frame.image_source == 0) {
        return SendFeature(image_handle);
    }

    // 人脸检测后处理
    ReplyFeature(image_handle);
    return SUCCESS;
}
