#include "model_process.h"
#include <iostream>
#include "utils.h"
using namespace std;

/* 初始化参数 */
ModelProcess::ModelProcess():loadFlag_(false), modelId_(0), modelMemPtr_(nullptr), modelMemSize_(0),
modelWeightPtr_(nullptr), modelWeightSize_(0), modelDesc_(nullptr), input_(nullptr), output_(nullptr)
{
}

ModelProcess::~ModelProcess() {
}

/* 模型加载 */
Result ModelProcess::LoadModelFromFileWithMem(const char * modelPath)
{
    if (loadFlag_) {
        ERROR_LOG("模型已加载");
        return FAILED;
    }

    // 获取模型的权值内存大小和工作内存大小
    aclError ret = aclmdlQuerySize(modelPath, &modelMemSize_, &modelWeightSize_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("计算模型权值内存大小和工作内存大小失败，模型所在路径为:%s\n", modelPath);
        return FAILED;
    }

    // 申请内存
    ret = aclrtMalloc(&modelMemPtr_, modelMemSize_, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("申请模型工作内存失败，申请大小为:%zu\n", modelMemSize_);
        return FAILED;
    }

    ret = aclrtMalloc(&modelWeightPtr_, modelWeightSize_, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("申请模型权值内存失败，申请大小为:%zu\n", modelWeightSize_);
        return FAILED;
    }

    // 加载本地文件模型到内存中
    ret = aclmdlLoadFromFileWithMem(modelPath, &modelId_, modelMemPtr_, modelMemSize_, modelWeightPtr_, modelWeightSize_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("从本地文件中加载模型失败，模型所在路径为:%s\n", modelPath);
        return FAILED;
    }

    loadFlag_ = true;
    return SUCCESS;
}

/* 模型卸载 */
void ModelProcess::Unload()
{
    if (!loadFlag_) {
        WARN_LOG("模型还没有被加载，卸载失败");
        return ;
    }

    // 释放内存
    if (modelMemPtr_ != nullptr) {
        aclrtFree(modelMemPtr_);
        modelMemPtr_ = nullptr;
        modelMemSize_ = 0;
    }

    if (modelWeightPtr_ != nullptr) {
        aclrtFree(modelWeightPtr_);
        modelWeightPtr_ = nullptr;
        modelWeightSize_ = 0;
    }

    loadFlag_ = false;
}

/* 创建模型描述信息 */
Result ModelProcess::CreateDesc()
{
    // 创建模型描述
    modelDesc_ = aclmdlCreateDesc();
    if (modelDesc_ == nullptr) {
        ERROR_LOG("创建模型描述失败");
        return FAILED;
    }

    // 根据模型ID获取模型描述信息
    aclError ret = aclmdlGetDesc(modelDesc_, modelId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("获取模型描述信息失败");
        return FAILED;
    }

    return SUCCESS;
}

/* 销毁模型描述 */
void ModelProcess::DestroyDesc()
{
    if (modelDesc_ != nullptr) {
        (void)aclmdlDestroyDesc(modelDesc_);
        modelDesc_ = nullptr;
    }
}

/* 创建模型输入 */
Result ModelProcess::CreateInput(void * input1, size_t input1size)
{
    // 创建模型输入Dataset
    input_ = aclmdlCreateDataset();
    if (input_ == nullptr) {
        ERROR_LOG("创建模型输入dataset失败");
        return FAILED;
    }

    // 创建模型输入dataBuffer
    aclDataBuffer* inputData = aclCreateDataBuffer(input1, input1size);
    if (inputData == nullptr) {
        ERROR_LOG("创建模型输入dataBuffer失败");
        return FAILED;
    }

    // 添加dataBuffer到dataset中
    aclError ret = aclmdlAddDatasetBuffer(input_, inputData);
    if (inputData == nullptr) {
        ERROR_LOG("模型输入dataBuffer为空，添加它到dataset失败");
        aclDestroyDataBuffer(inputData);
        inputData = nullptr;
        return FAILED;
    }

    return SUCCESS;
}

/* 销毁模型输出 */
void ModelProcess::DestroyInput()
{
    if (input_ == nullptr) {
        return ;
    }

    // 获取dataset中dataBuffer的数量，并释放每一个dataBuffer
    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(input_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(input_, i);
        aclDestroyDataBuffer(dataBuffer);
    }

    // 销毁dataset
    aclmdlDestroyDataset(input_);
    input_ = nullptr;
}

/* 创建模型输出 */
Result ModelProcess::CreateOutput()
{
    if (modelDesc_ == nullptr) {
        ERROR_LOG("模型描述不存在，创建模型输出失败!");
        return FAILED;
    }

    // 创建模型输出Dataset
    output_ = aclmdlCreateDataset();
    if (output_ == nullptr) {
        ERROR_LOG("创建模型输出Dataset失败");
        return FAILED;
    }

    // 获取模型输出数量
    size_t outputSize = aclmdlGetNumOutputs(modelDesc_);
    for (size_t i = 0; i < outputSize; ++i) {
        // 获取每一个输出的大小
        size_t buffer_size = aclmdlGetOutputSizeByIndex(modelDesc_, i);

        // 申请内存存放每一个输出
        void * outputBuffer = nullptr;
        aclError ret = aclrtMalloc(&outputBuffer, buffer_size, ACL_MEM_MALLOC_NORMAL_ONLY);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("申请内存失败,申请的大小为:%zu,创建模型输出失败", buffer_size);
            return FAILED;
        }

        // 创建模型输出dataBuffer
        aclDataBuffer* outputData = aclCreateDataBuffer(outputBuffer, buffer_size);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("创建模型输出dataBuffer失败");
            aclrtFree(outputBuffer);
            return FAILED;
        }

        // 添加DataBuffer到Dataset中
        ret = aclmdlAddDatasetBuffer(output_, outputData);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("模型输入dataBuffer为空，添加它到dataset失败");
            aclrtFree(outputBuffer);
            aclDestroyDataBuffer(outputData);
            return FAILED;
        }
    }

    return SUCCESS;
}

/* 销毁模型输出 */
void ModelProcess::DestroyOutput()
{
    if (output_ == nullptr) {
        return ;
    }

    // 释放每个DataBuffer和其中存储的内容
    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(output_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(output_, i);
        void* data = aclGetDataBufferAddr(dataBuffer);
        (void)aclrtFree(data);
        (void)aclDestroyDataBuffer(dataBuffer);
    }

    // 销毁DataSet
    (void)aclmdlDestroyDataset(output_);

    output_ = nullptr;
}

/* 模型执行（同步） */
Result ModelProcess::Execute()
{
    aclError ret = aclmdlExecute(modelId_, input_, output_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("执行模型失败，模型ID是:%u\n", modelId_);
        return FAILED;
    }

    return SUCCESS;
}

/* 销毁资源 */
void ModelProcess::DestroyResource()
{
    Unload();
    DestroyDesc();
    DestroyInput();
    DestroyOutput();
}

/* 获取模型输出数据 */
aclmdlDataset * ModelProcess::GetModelOutputData()
{
    return output_;
}