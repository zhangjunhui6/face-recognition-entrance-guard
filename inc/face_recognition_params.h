#ifndef FACE_RECOGNITION_PARAMS_H_
#define FACE_RECOGNITION_PARAMS_H_

#include "utils.h"
#include <iostream>
#include <mutex>
#include <unistd.h>

#include "acl/acl.h"
#include "model_process.h"
#include "dvpp_process.h"

#include "face_feature_train_mean.h"
#include "face_feature_train_std.h"
#include "opencv2/opencv.hpp"

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"
#include<opencv2/core/core.hpp>

#define CHECK_MEM_OPERATOR_RESULTS(ret) \
if (ret != SUCCESS) { \
    ERROR_LOG("memory operation failed, error=%d", ret); \
    return FAILED; \
}

// NV12(YUV420SP:YYYYYYYY UVUV) 图像的转换参数
const int32_t kNv12SizeMolecule = 3;
const int32_t kNv12SizeDenominator = 2;


/**
 * @brief: 人脸识别APP错误码定义
 */
enum class AppErrorCode {
    // 成功，没有错误
    kNone = 0,

    // 注册失败
    kRegister,

    // 人脸检测失败
    kDetection,

    // 活体检测失败
    kAntiSpoofing,

    // 关键点检测失败
    kFeatureMask,

    // 特征提取失败
    kRecognition
};

/**
 * @brief: 错误信息
 */
struct ErrorInfo {
    AppErrorCode err_code = AppErrorCode::kNone;
    string err_msg = "";
};

/**
 * @brief: 帧信息
 */
struct FrameInfo {
    uint32_t frame_id = 0;      // 帧id
    uint32_t channel_id = 0;    // 摄像头通道id
    uint32_t timestamp = 0;     // 当前帧的时间戳
    uint32_t image_source = 0;  // 0：相机 1：人脸注册
    string face_id = "";        // 注册用户人脸ID
    // org_img的原始图像格式
    acldvppPixelFormat  org_img_format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    bool img_aligned = false;   // 原始图像是否已对齐
    unsigned char * original_jpeg_pic_buffer;    // 输出缓冲器
    unsigned int original_jpeg_pic_size;        // 输出缓冲区大小
};

/**
 * @brief: 人脸矩形框
 */
struct FaceRectangle {
    cv::Point lt;
    cv::Point rb;
};


/**
 * @brief: 面部特征
 */
struct FaceFeature {
    bool flag;
    cv::Point left_eye;
    cv::Point right_eye;
    cv::Point nose;
    cv::Point left_mouth;
    cv::Point right_mouth;
};

/**
 * @brief: 人脸照片
 */
struct FaceImage {
    ImageData image;  // 从原始图像裁剪图像
    FaceRectangle rectangle;  // 人脸矩形框
    float alive_score;      // 活体指数
    FaceFeature feature_mask;  // 人脸关键点
    vector < float > feature_vector;  // 人脸特征矩阵
};

/**
 * @brief: 人脸识别信息
 */
struct FaceRecognitionInfo {
    FrameInfo frame;        // 帧信息
    ErrorInfo err_info;     // 错误信息
    ImageData org_img;      // 原图
    vector < FaceImage > face_imgs;  // 人脸识别图像集合
};

#endif