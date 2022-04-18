#pragma once
#include <iostream>
#include "utils.h"
#include "acl/acl.h"


/**
* Model推理程序
*/
class ModelProcess {
    public:
    /**
    * @brief 构造函数
    */
    ModelProcess();

    /**
    * @brief 析构函数
    */
    ~ModelProcess();

    /**
    * @brief 从本地文件中加载模型到内存
    * @param [in] modelPath: 模型所在路径
    */
    Result LoadModelFromFileWithMem(const char * modelPath);

    /**
    * @brief 模型卸载
    */
    void Unload();

    /**
    * @brief 创建模型描述
    */
    Result CreateDesc();

    /**
    * @brief 销毁模型描述
    */
    void DestroyDesc();

    /**
    * @brief 创建模型输入
    * @param [in] inputDataBuffer: 输入数据指针
    * @param [in] bufferSize: 输入数据长度
    */
    Result CreateInput(void * input1, size_t input1size);

    /**
    * @brief 销毁模型输入
    */
    void DestroyInput();

    /**
    * @brief 创建模型输出
    */
    Result CreateOutput();

    /**
    * @brief 销毁模型输出
    */
    void DestroyOutput();

    /**
    * @brief 模型执行
    */
    Result Execute();


    void DestroyResource();

    /**
    * @brief 获取模型输出数据
    * @return 输出数据，dataset类型
    */
    aclmdlDataset * GetModelOutputData();

    private:
    bool loadFlag_;  // 模型加载标志
    uint32_t modelId_;

    void * modelMemPtr_;
    size_t modelMemSize_;
    void * modelWeightPtr_;
    size_t modelWeightSize_;

    aclmdlDesc * modelDesc_;
    aclmdlDataset * input_;
    aclmdlDataset * output_;
};
