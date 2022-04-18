#ifndef FACE_DETECTION_ENGINE_H_
#define FACE_DETECTION_ENGINE_H_

#include "face_recognition_params.h"
#include "opencv2/opencv.hpp"


class FaceDetection {
    public:
    FaceDetection();

    ~FaceDetection() = default;

    Result Init();
    Result Process(shared_ptr < FaceRecognitionInfo >& image_handle);

    private:
    /**
     * @brief: 校正率
     * param [in]: ratio
     * @return: ratio in [0, 1]
     */
    float CorrectionRatio(float ratio);

    /**
     * @brief: 检查推理结果是否有效
     * param [in]: attribute
     * param [in]: score
     * param [in]: face rectangle
     * @return: SUCCESS or FAILED
     */
    Result IsValidResults(float attr, float score, const FaceRectangle & rectangle);

    /**
     * @brief: 预处理
     * param [in]: image_handle: 原图
     * param [out]: resized_image: dvpp缩放后图像
     * @return: SUCCESS or FAILED
     */
    Result PreProcess(const shared_ptr < FaceRecognitionInfo > & image_handle,
    ImageData & resized_image);

    /**
     * @brief: 推理
     * param [in]: resized_image: dvpp缩放后图像
     * param [out]: detection_inference: 推理输出
     * @return: SUCCESS or FAILED
     */
    Result Inference(const ImageData & resized_image,
    aclmdlDataset*& detection_inference);

    /**
     * @brief: 后处理
     * param [out]: image_handle: 图像处理数据
     * param [in]: detection_inference: 推理输出
     * @return: SUCCESS or FAILED
     */
    Result PostProcess(shared_ptr < FaceRecognitionInfo > & image_handle,
    aclmdlDataset* detection_inference);

    /**
     * @brief: 处理错误场景
     * param [in]: err_code: 错误代码
     * param [in]: err_msg: 错误信息
     * param [out]: image_handle: 图像处理数据
     */
    void HandleErrors(AppErrorCode err_code, const string & err_msg,
    shared_ptr < FaceRecognitionInfo > & image_handle);

    /**
     * @brief: 发送结果
     * param [out]: image_handle: 图像处理数据
     */
    void SendResult(shared_ptr < FaceRecognitionInfo > & image_handle);

    private:
    float confidence_;
};
#endif

