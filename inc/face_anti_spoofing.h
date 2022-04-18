#ifndef access_control_system_face_anti_spoofing_H
#define access_control_system_face_anti_spoofing_H

#include "face_recognition_params.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"
using namespace cv;

class FaceAntiSpoofing {
    public:
    FaceAntiSpoofing();

    ~FaceAntiSpoofing() = default;

    Result Init();
    Result Process(shared_ptr < FaceRecognitionInfo >& image_handle);

    private:
    /**
     * @brief: 得到新的矩形区域
     * param [in]: src_w、src_h： 源图像的宽度和高度
     * param [in]: face_imgs： 人脸检测结果，矩形框
     * param [out]: box： 计算需要裁剪的矩形区域
     * @return: SUCCESS or FAILED
     */
    Result GetNewBox(u_int32_t src_w, u_int32_t src_h, vector < FaceImage > face_imgs, vector < FaceRectangle > & box);

    /*
    * @brief: 从原始图像中裁剪人脸
    * @param [in]: org_img: 原始图像信息
    * @param [in]: new_boxs: 新矩形框
    * @param [out]: face_imgs->image Face:裁剪后的人脸图像数据，NV12格式
    * @return: SUCCESS or FAILED
    */
    Result Crop(ImageData org_img, vector < FaceRectangle > new_boxs, vector < FaceImage > & face_imgs);

    /*
    * @brief: 将图像缩放至80*80
    * @param [in]: face_imgs->image : 裁剪后的图像数据
    * @param [out]: resized_imgs : 缩放后的图片信息
    * @return: SUCCESS or FAILED
    */
    Result Resize(vector < FaceImage > face_imgs, vector < ImageData > & resized_imgs);

    /**
     * @brief: 预处理
     * param [in]: image_handle: 原图
     * param [out]: resized_imgs: 输出图像
     * param [out]: err_msg: 错误信息
     * @return: SUCCESS or FAILED
     */
    Result PreProcess(shared_ptr < FaceRecognitionInfo > & image_handle,
    vector < ImageData > & resized_imgs, string & err_msg);

    /**
     * @brief: 推理
     * param [in/out]: face_imgs:更改 is_alive 属性
     * param [in]: resize_imgs
     * @return: SUCCESS or FAILED
     */
    Result Inference(vector < FaceImage > & face_imgs,
    vector < ImageData > & resized_imgs);

    /**
     * @brief: 处理错误场景
     * param [in]: err_code: 错误代码
     * param [in]: err_msg: 错误信息
     * param [out]: image_handle
     */
    void HandleErrors(AppErrorCode err_code, const string & err_msg,
    shared_ptr < FaceRecognitionInfo > & image_handle);

    /**
     * @brief: 发送结果
     * param [out]: image_handle
     */
    void SendResult(shared_ptr < FaceRecognitionInfo > & image_handle);

    private:
    float confidence_;

};

#endif
