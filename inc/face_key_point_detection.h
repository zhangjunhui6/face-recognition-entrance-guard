#ifndef access_control_system_face_key_point_dentection_H
#define access_control_system_face_key_point_dentection_H

#include "face_recognition_params.h"
#include <iostream>
#include <string>
#include <dirent.h>
#include <memory>
#include <unistd.h>
#include <vector>
#include <cstdint>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/types_c.h"

using namespace cv;

class FaceKeyPointDetection {
    public:
    Result Init();

    Result  Process(shared_ptr < FaceRecognitionInfo > & image_handle);

    private:
    /*
     * @brief: 根据人脸坐标从原始图像中裁剪人脸
     * @param [in]: org_img 原始图像信息
     * @param [in/out]: face_imgs->rectangle 基于原始图像的人脸框，
     *              face_imgs->images 裁剪后的人脸图像数据，NV12
     * @return: SUCCESS or FAILED
     */
    Result Crop(const ImageData & org_img, vector < FaceImage > & face_imgs);

    /*
     * @brief: 将裁剪后的图像缩放至40*40
     * @param [in]: face_imgs->image 裁剪后的图像数据
     * @param [in]: resized_imgs 缩放后的图像信息
     * @return: SUCCESS or FAILED
     */
    Result Resize(vector < FaceImage > & face_imgs, vector < ImageData > & resized_imgs);

    /*
     * @brief: 将图像从缩放后的YUV格式转换为 BGR格式
     * @param [in]: resized_imgs 缩放后的YUV 图像
     * @param [in]: bgr_imgs 转换后的 BGR 图像
     * @return: SUCCESS or FAILED
     */
    Result ImageYUV2BGR (vector < ImageData > & resized_imgs, vector < Mat > & bgr_imgs);

    /*
     * @brief: 图像归一化
     * @param [in]: bgr_imgs BGR图像数据，介于(0,255)之间
     * @param [in]: bgr_img 归一化后的数据
     * @return: SUCCESS or FAILED
     */
    Result NormalizeData (vector < Mat > & bgr_imgs);

    /*
     * @brief: 预处理
     * param [in]: image_handle: 原图
     * param [out]: BGR图片
     * @return: SUCCESS or FAILED
     */
    Result PreProcess(shared_ptr < FaceRecognitionInfo > & image_handle, vector < cv::Mat > & bgr_imgs, string & err_msg);

    /*
     * @brief: 将数据从 Mat 复制到内存缓冲区
     * @param [in]: bgr_imgs 归一化后的数据
     * @param [in]: i
     * @param [in]: tensor_buffer 推理缓冲区
     */
    int CopyDataToBuffer(vector < Mat > & bgr_imgs, int i, float * tensor_buffer);

    /*
    * @brief: 模型推理
    * @param [in]: bgr_imgs 归一化后的数据
    * @param [in]: face_imgs->feature_mask 推理结果
    * @return: SUCCESS or FAILED
    */
    Result Inference(vector < Mat > & bgr_imgs,
    vector < FaceImage > & face_imgs);

    /**
     * @brief: 处理错误场景
     * param [in]: err_code: 错误代码
     * param [in]: err_msg: 错误信息
     * param [out]: face_recognition_info
     */
    void HandleErrors(AppErrorCode err_code, const string & err_msg,
    shared_ptr < FaceRecognitionInfo > & image_handle);

    /**
     * @brief: 发送结果
     * param [out]: image_handle
     */
    void SendResult(shared_ptr < FaceRecognitionInfo > & image_handle);

    private:
    // 训练后的平均值
    cv::Mat train_mean_;

    // 训练后的标准值
    cv::Mat train_std_;

    enum FaceFeaturePos {
        kLeftEyeX,
        kLeftEyeY,
        kRightEyeX,
        kRightEyeY,
        kNoseX,
        kNoseY,
        kLeftMouthX,
        kLeftMouthY,
        kRightMouthX,
        kRightMouthY
    };
};

#endif

