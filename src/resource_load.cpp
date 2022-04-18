#include <iostream>

#include "acl/acl.h"
#include "model_process.h"
#include "utils.h"
#include "resource_load.h"
#include "face_feature_train_mean.h"
#include "face_feature_train_std.h"

using namespace std;
FaceDetection ResourceLoad::faceDetection;
FaceAntiSpoofing ResourceLoad::faceAntiSpoofing;
FaceKeyPointDetection ResourceLoad::faceKeyPointDetection;
FaceFeatureExtraction ResourceLoad::faceFeatureExtraction;
FacePostProcess ResourceLoad::facePostProcess;

/* 初始化ACL资源 */
Result ResourceLoad::InitResource() {
    // ACL init
    const char * aclConfigPath = "../src/acl.json";
    aclError ret = aclInit(aclConfigPath);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl初始化失败\n");
        return FAILED;
    }

    // 打开设备
    ret = aclrtSetDevice(deviceId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl 打开设备%d失败\n", deviceId_);
        return FAILED;
    }

    // 创建context
    ret = aclrtCreateContext(&context_, deviceId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl 创建context失败\n");
        return FAILED;
    }

    // 创建stream
    ret = aclrtCreateStream(&stream_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl 创建stream失败\n");
        return FAILED;
    }

    //获取运行模式
    ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl 获取运行模式失败\n");
        return FAILED;
    }

    return SUCCESS;
}

/* 初始化模型参数 */
void ResourceLoad::InitModelInfo(const ModelInfoParams& param) {
    modelPath1_ = param.modelPath1;
    modelPath2_ = param.modelPath2;
    modelPath3_ = param.modelPath3;
    modelPath4_ = param.modelPath4;
}

/* 初始化模型 */
Result ResourceLoad::InitModel(const char* omModelPath1, const char* omModelPath2,
const char* omModelPath3, const char* omModelPath4) {
    /* Model1*/
    /* 加载模型 */
    Result ret = model1_.LoadModelFromFileWithMem(omModelPath1);
    if (ret != SUCCESS) {
        ERROR_LOG("从文件中加载模型1失败\n");
        return FAILED;
    }

    /* 创建模型描述 */
    ret = model1_.CreateDesc();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型1的描述失败\n");
        return FAILED;
    }

    /* 创建模型输出 */
    ret = model1_.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型1的输出失败\n");
        return FAILED;
    }

    /* Model2 */
    ret = model2_.LoadModelFromFileWithMem(omModelPath2);
    if (ret != SUCCESS) {
        ERROR_LOG("从文件中加载模型2失败\n");
        return FAILED;
    }

    ret = model2_.CreateDesc();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型2的描述失败\n");
        return FAILED;
    }

    ret = model2_.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型2的输出失败\n");
        return FAILED;
    }

    /* Model3 */
    ret = model3_.LoadModelFromFileWithMem(omModelPath3);
    if (ret != SUCCESS) {
        ERROR_LOG("从文件中加载模型3失败\n");
        return FAILED;
    }

    ret = model3_.CreateDesc();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型3的描述失败\n");
        return FAILED;
    }

    ret = model3_.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型3的输出失败\n");
        return FAILED;
    }

    /* Model4 */
    ret = model4_.LoadModelFromFileWithMem(omModelPath4);
    if (ret != SUCCESS) {
        ERROR_LOG("从文件中加载模型4失败\n");
        return FAILED;
    }

    ret = model4_.CreateDesc();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型4的描述失败\n");
        return FAILED;
    }

    ret = model4_.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型4的输出失败\n");
        return FAILED;
    }
    return SUCCESS;
}

/* 打开Presenter通道,创建人脸注册通道  */
Result ResourceLoad::OpenPresenterChannel() {
    const char* configFile = "./param.conf";
    map < string, string > config;
    PresenterChannels::GetInstance().ReadConfig(config, configFile);

    PresenterServerParams register_param;
    register_param.app_type = "facial_recognition";
    map < string, string >::const_iterator mIter = config.begin();
    for (; mIter != config.end(); ++mIter) {
        if (mIter->first == "presenter_server_ip") {
            register_param.host_ip = mIter->second;
        }
        else if (mIter->first == "presenter_server_port") {
            register_param.port = std::stoi(mIter->second);
        }
        else if (mIter->first == "channel_name") {
            register_param.app_id = mIter->second;
        }
    }

    INFO_LOG("Socket服务器IP:%s,Port:%d, app名称:%s, app类型:%s",
    register_param.host_ip.c_str(), register_param.port,
    register_param.app_id.c_str(), register_param.app_type.c_str());

    // 通过配置初始化类PresenterChannels
    PresenterChannels::GetInstance().Init(register_param);

    // 创建人脸注册通道
    Channel * channel = PresenterChannels::GetInstance().GetChannel();
    if (channel == nullptr) {
        ERROR_LOG("创建人脸注册通道失败。");
        return FAILED;
    }

    return SUCCESS;
}

/* 初始化 train_mean_ 和 train_std_ */
Result ResourceLoad::InitComponent() {
    ResourceLoad::faceKeyPointDetection.Init();
    return SUCCESS;
}

/* 初始化I2C和GPIO */
Result ResourceLoad::InitI2cAndGpio() {
    PeripheralCtrl::GetInstance().Init();
    return SUCCESS;
}

ResourceLoad::~ResourceLoad() {
    DestroyResource();
}

/* 资源初始化 */
Result ResourceLoad::Init(const ModelInfoParams& param) {
    if (isInited_) {
        return SUCCESS;
    }

    Result ret = FAILED;
    /*1. 初始化 ACL 资源 */
    ret = InitResource();
    if (ret != SUCCESS) {
        ERROR_LOG("初始化 acl 资源失败\n");
        return FAILED;
    }

    /*2. 初始化模型信息 */
    InitModelInfo(param);

    /*3. 初始化模型 */
    ret = InitModel(modelPath1_, modelPath2_, modelPath3_, modelPath4_);
    if (ret != SUCCESS) {
        ERROR_LOG("初始化模型失败\n");
        return FAILED;
    }

    /*4. 初始化 Dvpp 资源，创建Dvpp通道 */
    ret = dvpp_.InitResource(stream_);
    if (ret != SUCCESS) {
        ERROR_LOG("初始化 dvpp 失败\n");
        return FAILED;
    }

    /*5. 创建人脸注册通道 */
    ret = OpenPresenterChannel();
    if (ret != SUCCESS) {
        ERROR_LOG("创建人脸注册通道失败\n");
        return FAILED;
    }
    INFO_LOG("创建人脸注册通道成功");

    /*6. 初始化 train_mean_ 和 train_std_ */
    ret = InitComponent();
    if (ret != SUCCESS) {
        ERROR_LOG("初始化组件失败\n");
        return FAILED;
    }

    /*7. Init i2c and gpio */
    ret = InitI2cAndGpio();
    if (ret != SUCCESS) {
        ERROR_LOG("初始化 i2c 和 gpio 失败\n");
        return FAILED;
    }

    INFO_LOG("初始化资源加载成功\n");
    isInited_ = true;
    return SUCCESS;
}

/* 获取模型输出项 */
void* ResourceLoad::GetInferenceOutputItem(uint32_t& itemDataSize,
aclmdlDataset* inferenceOutput, uint32_t idx) {
    aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(inferenceOutput, idx);
    if (dataBuffer == nullptr) {
        ERROR_LOG("从模型推理输出中获取第 %d 个数据集缓冲区失败\n", idx);
        return nullptr;
    }

    void* dataBufferDev = aclGetDataBufferAddr(dataBuffer);
    if (dataBufferDev == nullptr) {
        ERROR_LOG("从模型推理输出中获取第 %d 个数据集缓冲区地址失败\n", idx);
        return nullptr;
    }

    size_t bufferSize = aclGetDataBufferSizeV2(dataBuffer);
    if (bufferSize == 0) {
        ERROR_LOG("模型推理输出的第 %d 个数据集缓冲区大小为 0\n", idx);
        return nullptr;
    }

    void* data = nullptr;
    data = dataBufferDev;

    itemDataSize = bufferSize;
    return data;
}

/* 获取模型实例 */
ModelProcess& ResourceLoad::GetModel(int model) {
    switch (model) {
        case 1:
        return model1_;
        case 2:
        return model2_;
        case 3:
        return model3_;
        case 4:
        return model4_;
        default:
        ERROR_LOG("获取模型失败\n");
        return model1_;
    }
}

/* 获取dvpp实例 */
DvppProcess& ResourceLoad::GetDvpp() {
    return dvpp_;
}

/* 发送数据到下一流程函数 */
Result ResourceLoad::SendNextModelProcess(const string objStr, shared_ptr < FaceRecognitionInfo > & image_handle) {
    if (objStr == "FaceRegister" || objStr == "MindCamera") {
        faceDetection.Process(image_handle);
    } else if (objStr == "FaceDetection") {
        faceAntiSpoofing.Process(image_handle);
    } else if (objStr == "FaceAntiSpoofing") {
        faceKeyPointDetection.Process(image_handle);
    } else if (objStr == "FaceKeyPointDetection") {
        faceFeatureExtraction.Process(image_handle);
    } else if (objStr == "FaceFeatureExtraction") {
        facePostProcess.Process(image_handle);
    }  else {
        ERROR_LOG("发送下一模型处理程序失败! ");
        return FAILED;
    }
    return SUCCESS;
}

/* 销毁资源 */
void ResourceLoad::DestroyResource()
{
    model1_.DestroyResource();
    model2_.DestroyResource();
    model3_.DestroyResource();
    model4_.DestroyResource();

    aclError ret = aclrtResetDevice(deviceId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("重置设备失败\n");
    }

    ret = aclFinalize();
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG(" acl去初始化失败\n");
    }
}
