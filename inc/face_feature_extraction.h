#ifndef access_control_system_face_feature_extraction_H
#define access_control_system_face_feature_extraction_H

#include "face_recognition_params.h"
#include <vector>

struct AlignedFace1 {
    // 人脸索引
    int32_t face_index;
    // 对齐的人脸数据
    cv::Mat aligned_face;
    // 对齐后并翻转的人脸数据
    cv::Mat aligned_flip_face;
};


class FaceFeatureExtraction {
    public:
    Result Process(shared_ptr < FaceRecognitionInfo > & image_handle);

    private:
    /**
     * @brief: 图像缩放
     * param [in]: face_img: 裁剪的人脸图像
     * param [out]: resized_image: 调用 dvpp 输出图像
     * @return: SUCCESS or FAILED
     */
    Result Resize(const FaceImage & face_img, ImageData & resized_image);

    /**
     * @brief 图像格式转换，调用OpenCV接口进行图像转换，从YUV420SP到BGR
     * @param [in] resized_image: 源图像
     * @param [out] bgr: 转换后的图像，Mat类型
     * @return: SUCCESS or FAILED
     */
    Result YUV420SPToBgr(const ImageData & resized_image, cv::Mat & bgr);

    /**
     * @brief 检查 openCV的变换矩阵
     * @param [in] mat: 变换矩阵
     * @return SUCCESS or FAILED
     */
    Result checkTransformMat(const cv::Mat & mat);

    /**
     * @brief: 对齐和翻转人脸
     * param [in]: face_img: 裁剪的人脸图像
     * param [in]: resized_image: 调用dvpp输出图像
     * param [in]: index: 图像索引
     * param [out]: aligned_imgs: 结果图像
     * @return:
     */
    Result AlignedAndFlipFace(const FaceImage & face_img, const ImageData & resized_image,
    int32_t index, vector < AlignedFace1 > & aligned_imgs);

    /**
     * @brief:预处理
     * param [in]: face_imgs: 人脸图像
     * param [out]: aligned_imgs: 对齐的输出图像 (RGB)
     */
    void PreProcess(const vector < FaceImage > & face_imgs,
    vector < AlignedFace1 > & aligned_imgs);

    /**
     * @brief:准备缓冲
     * param [in]: index
     * param [out]: batch_buffer: 批处理缓冲区
     * param [in]: buffer_size: 批处理缓冲区总大小
     * param [in]: each_img_size: 每个人脸图像大小
     * param [in]: aligned_imgs: 对齐和翻转的人脸图像
     * @return:
     */
    Result PrepareBuffer(int32_t index,
    shared_ptr < uint8_t > & batch_buffer,
    uint32_t buffer_size, uint32_t each_img_size,
    const vector < AlignedFace1 > & aligned_imgs);

    /**
     * @brief: 分析结果
     * param [in]: index
     * param [in]: inference_result: 推理输出数据
     * param [in]: inference_size
     * param [in]: aligned_imgs: 对齐和翻转人脸图像
     * param [out]: face_imgs: 源人脸图像
     * @return:
     */
    Result ArrangeResult(int32_t index,
    float* inference_result,
    const uint32_t inference_size,
    const vector < AlignedFace1 > & aligned_imgs,
    vector < FaceImage > & face_imgs);

    /**
     * @brief:推理
     * param [in]: aligned_imgs
     * param [out]: face_imgs
     */
    Result Inference(const vector < AlignedFace1 > & aligned_imgs,
    vector < FaceImage > & face_imgs);

    /**
     * @brief: 发送结果
     * param [out]: image_handle
     */
    void SendResult(shared_ptr < FaceRecognitionInfo > & image_handle);
};

#endif
