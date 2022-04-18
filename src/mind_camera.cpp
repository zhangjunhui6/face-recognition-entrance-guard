#include "mind_camera.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstring>
#include <chrono>
#include "resource_load.h"

extern "C" {
#include "driver/peripheral_api.h"

}


namespace {
    const uint32_t kInitFrameId = 0;
}

string MindCamera::CameraDatasetsConfig::ToString() const {
    stringstream log_info_stream("");
    log_info_stream << "fps:" << this->fps << ", camera:" << this->channel_id
    << ", image_format:" << this->image_format
    << ", resolution_width:" << this->resolution_width
    << ", resolution_height:" << this->resolution_height;

    return log_info_stream.str();
}

/* 初始化参数 */
MindCamera::MindCamera() {
    config_ = nullptr;
    frame_id_ = kInitFrameId;
    exit_flag_ = CAMERADATASETS_INIT;
}

MindCamera::~MindCamera() {
}

/* 初始化CameraDataSets，设置config_参数 */
bool MindCamera::Init() {
    if (config_ == nullptr) {
        config_ = make_shared < CameraDatasetsConfig >();
    }
    config_->fps = 10;
    config_->image_format = CAMERA_IMAGE_YUV420_SP;
    config_->channel_id = (int)CAMERAL_1;
    config_->resolution_width = 1280;
    config_->resolution_height = 720;

    bool ret = true;
    bool failed_flag = (config_->image_format == PARSEPARAM_FAIL
    || config_->channel_id == PARSEPARAM_FAIL
    || config_->resolution_width == 0 || config_->resolution_height == 0);
    if (failed_flag) {
        string msg = config_->ToString();
        msg.append("配置数据失败");
        ret = false;
    }

    return ret;
}

bool MindCamera::Process() {
    Init();
    DoCapProcess();
    return true;
}

/* 创建图片处理数据 */
shared_ptr < FaceRecognitionInfo > MindCamera::CreateBatchImageParaObj() {
    shared_ptr < FaceRecognitionInfo > pObj = make_shared < FaceRecognitionInfo >();
    pObj->frame.image_source = 0;
    // 每次处理一个图片流
    pObj->frame.channel_id = config_->channel_id;
    pObj->frame.frame_id = frame_id_++;
    pObj->frame.timestamp = time(nullptr);
    pObj->org_img.width = config_->resolution_width;
    pObj->org_img.height = config_->resolution_height;
    pObj->org_img.alignWidth = ALIGN_UP16(config_->resolution_width);
    pObj->org_img.alignHeight = ALIGN_UP2(config_->resolution_height);
    pObj->org_img.size = config_->resolution_width * config_->resolution_height * 3 / 2;

    void * buffer = nullptr;
    aclError aclRet = acldvppMalloc(&buffer, pObj->org_img.size);
    pObj->org_img.data.reset((uint8_t*)buffer, [](uint8_t* p) {
        acldvppFree((void *)p);
    });
    return pObj;
}

/* 获取exit_flag_ */
int MindCamera::GetExitFlag() {
    TLock lock(mutex_);
    return exit_flag_;
}

/* 设置exit_flag_ */
void MindCamera::SetExitFlag(int flag) {
    TLock lock(mutex_);
    exit_flag_ = flag;
}

/* 预处理 */
MindCamera::CameraOperationCode MindCamera::PreCapProcess() {
    MediaLibInit();

    // 查询相机状态
    CameraStatus status = QueryCameraStatus(config_->channel_id);
    if (status != CAMERA_STATUS_CLOSED) {
        ERROR_LOG("查询摄像头状态 {状态:%d} 失败.", status);
        return kCameraNotClosed;
    }

    // 打开摄像头
    int ret = OpenCamera(config_->channel_id);
    if (ret == 0) {
        ERROR_LOG("打开相机 {%d} 失败.", config_->channel_id);
        return kCameraOpenFailed;
    }

    // 设置 fps
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_FPS, &(config_->fps));
    if (ret == 0) {
        ERROR_LOG("设置fps:%d 失败.", config_->fps);
        return kCameraSetPropertyFailed;
    }

    // 设置图像格式
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_IMAGE_FORMAT, &(config_->image_format));
    if (ret == 0) {
        ERROR_LOG("设置图像格式 :%d 失败.", config_->image_format);
        return kCameraSetPropertyFailed;
    }

    // 设置图像分辨率
    CameraResolution resolution;
    resolution.width = config_->resolution_width;
    resolution.height = config_->resolution_height;
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_RESOLUTION, &resolution);
    if (ret == 0) {
        ERROR_LOG("设置分辨率 {宽:%d, 高:%d} 失败.", config_->resolution_width, config_->resolution_height);
        return kCameraSetPropertyFailed;
    }

    // 设置工作模式
    CameraCapMode mode = CAMERA_CAP_ACTIVE;
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_CAP_MODE, &mode);
    if (ret == 0) {
        ERROR_LOG("设置相机工作模式:%d失败。", mode);
        return kCameraSetPropertyFailed;
    }

    return kCameraOk;
}

/* 摄像头数据采集 */
bool MindCamera::DoCapProcess() {
    CameraOperationCode ret_code = PreCapProcess();
    if (ret_code == kCameraSetPropertyFailed) {
        CloseCamera(config_->channel_id);
        ERROR_LOG("摄像头预处理失败");
        return false;
    } else
        if ((ret_code == kCameraOpenFailed) || (ret_code == kCameraNotClosed)) {
            ERROR_LOG("摄像头预处理失败");
            return false;
        }

    // 设置程序正在运行
    SetExitFlag(CAMERADATASETS_RUN);

    while (GetExitFlag() == CAMERADATASETS_RUN) {
        shared_ptr < FaceRecognitionInfo > p_obj = CreateBatchImageParaObj();
        uint8_t* p_data = p_obj->org_img.data.get();
        int read_size = (int)p_obj->org_img.size;

        // 从相机读取帧，调用时可能会更改readSize
        int read_ret = ReadFrameFromCamera(config_->channel_id, (void*)p_data, &read_size);

        // 当 readRet 为 1 时表示失败
        int read_flag = ((read_ret == 1) && (read_size == (int)p_obj->org_img.size));
        if (!read_flag) {
            ERROR_LOG("从摄像头读取帧失败 {摄像头ID:%d, 读取结果:%d, 读取长度:%d,原图片大小:%d} ",
            config_->channel_id, read_ret, read_size, (int)p_obj->org_img.size);
            break;
        }

        INFO_LOG("获取摄像头输入，图片宽:%d,高:%d,对齐宽:%d,对齐高:%d,大小:%d\n",
        p_obj->org_img.width, p_obj->org_img.height, p_obj->org_img.alignWidth, p_obj->org_img.alignHeight, p_obj->org_img.size);
        ResourceLoad::GetInstance().SendNextModelProcess("MindCamera", p_obj);
    }

    // 关闭摄像头
    CloseCamera(config_->channel_id);

    return true;
}