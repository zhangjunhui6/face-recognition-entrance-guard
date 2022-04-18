#pragma once

#include <iostream>
#include <mutex>
#include <unistd.h>
#include "presenter/agent/presenter_channel.h"
#include "presenter/agent/presenter_types.h"
#include "acl/acl.h"
#include "utils.h"
#include "dvpp_process.h"
#include "model_process.h"
#include "face_register.h"
#include "mind_camera.h"
#include "face_detection.h"
#include "face_anti_spoofing.h"
#include "face_key_point_detection.h"
#include "face_feature_extraction.h"
#include "face_post_process.h"
#include "peripheral_ctrl.h"

using namespace std;
using namespace ascend::presenter;


struct ModelInfoParams {
    const char* modelPath1;
    const char* modelPath2;
    const char* modelPath3;
    const char* modelPath4;
};


class ResourceLoad {
    public:
    static FaceDetection faceDetection;
    static FaceAntiSpoofing faceAntiSpoofing;
    static FaceKeyPointDetection faceKeyPointDetection;
    static FaceFeatureExtraction faceFeatureExtraction;
    static FacePostProcess facePostProcess;


    static ResourceLoad& GetInstance() {
        static ResourceLoad instance;
        return instance;
    }

    ~ResourceLoad();

    Result Init(const ModelInfoParams& param);

    void* GetInferenceOutputItem(uint32_t& itemDataSize, aclmdlDataset* inferenceOutput, uint32_t idx);

    ModelProcess& GetModel(int model);

    DvppProcess& GetDvpp();

    Result SendNextModelProcess(const string objStr, shared_ptr < FaceRecognitionInfo > & image_handle);

    void DestroyResource();

    private:
    void InitModelInfo(const ModelInfoParams& param);
    Result InitResource();
    Result OpenPresenterChannel();
    Result InitModel(const char* omModelPath1, const char* omModelPath2, const char* omModelPath3,
    const char* omModelPath4);
    Result InitComponent();
    Result InitI2cAndGpio();

    private:
    /* 运行管理资源相关参数 */
    int32_t deviceId_;
    aclrtContext context_;
    aclrtStream stream_;
    aclrtRunMode runMode_;

    /* dvpp */
    DvppProcess dvpp_;

    /* 模型相关参数 */
    const char* modelPath1_;
    uint32_t modelWidth1_;
    uint32_t modelHeight1_;
    uint32_t inputDataSize1_;

    const char* modelPath2_;
    uint32_t modelWidth2_;
    uint32_t modelHeight2_;
    uint32_t inputDataSize2_;

    const char* modelPath3_;
    uint32_t modelWidth3_;
    uint32_t modelHeight3_;
    uint32_t inputDataSize3_;

    const char* modelPath4_;
    uint32_t modelWidth4_;
    uint32_t modelHeight4_;
    uint32_t inputDataSize4_;

    /* 四个模型 */
    ModelProcess model1_;
    ModelProcess model2_;
    ModelProcess model3_;
    ModelProcess model4_;

    bool isInited_;
};

