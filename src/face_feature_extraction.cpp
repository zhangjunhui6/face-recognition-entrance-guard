#include "face_feature_extraction.h"

#include <cstdint>
#include <unistd.h>
#include <memory>
#include <sstream>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/video.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"

#include "resource_load.h"
using cv::Mat;
using namespace std;
using namespace cv;

namespace {
    const float kResizeWidth = 96.0;
    const float kResizeHeight = 112.0;

    // 人脸对齐的目标坐标
    const float kLeftEyeX = 30.2946;
    const float kLeftEyeY = 51.6963;
    const float kRightEyeX = 65.5318;
    const float kRightEyeY = 51.6963;
    const float kNoseX = 48.0252;
    const float kNoseY = 71.7366;
    const float kLeftMouthCornerX = 33.5493;
    const float kLeftMouthCornerY = 92.3655;
    const float kRightMouthCornerX = 62.7299;
    const float kRightMouthCornerY = 92.3655;

    // wapAffine 估计检查 cols(=3) 和 rows(=2)
    const int32_t kEstimateRows = 2;
    const int32_t kEstimateCols = 3;

    // 翻转人脸
    // OpenCV 的水平翻转
    const int32_t kHorizontallyFlip = 1;
    // OpenCV 的垂直和水平翻转
    const int32_t kVerticallyAndHorizontallyFlip = -1;

    const int32_t kBatchImgCount = 2;
    // 人脸推理结果应包含1024维向量(float)
    const int32_t kEveryFaceResVecCount = 1024;
}

/* 图片缩放 */
Result FaceFeatureExtraction::Resize(const FaceImage & face_img, ImageData & resized_image) {
    // 输入大小小于零，返回失败
    if (face_img.image.size <= 0) {
        ERROR_LOG("图像大小小于或等于零，大小=%d", face_img.image.size);
        return FAILED;
    }

    Result ret = ResourceLoad::GetInstance().GetDvpp().Resize(resized_image, const_cast < ImageData & > (face_img.image), kResizeWidth, kResizeHeight);
    if (ret == FAILED) {
        ERROR_LOG("图像缩放失败\n");
        return FAILED;
    }

    return SUCCESS;
}

/* 格式转换 */
Result FaceFeatureExtraction::YUV420SPToBgr(const ImageData & resized_image, Mat & dst) {
    // 将智能指针数据类型转换为Mat类型数据，以便调用 OpenCV 接口
    Mat src(resized_image.height * kNv12SizeMolecule / kNv12SizeDenominator, resized_image.width, CV_8UC1);

    // 要复制的图像大小，yuv图片大小为宽*高的1.5倍；
    int copy_size = resized_image.width * resized_image.height * kNv12SizeMolecule / kNv12SizeDenominator;

    // 目标缓冲区的大小
    int destination_size = src.cols * src.rows * src.elemSize();

    // 将输入的图像数据复制到 Mat 矩阵中进行图像转换。
    int ret = Utils::memcpy_s(src.data, destination_size, resized_image.data.get(), copy_size);
    CHECK_MEM_OPERATOR_RESULTS(ret);

    // 调用OpenCV接口将yuv420sp图像转换为bgr图像
    cvtColor(src, dst, CV_YUV2BGR_NV12);

    return SUCCESS;
}

/* 检查转换的Mat格式,坐标映射 */
Result FaceFeatureExtraction::checkTransformMat(const Mat & mat) {
    // 1. 类型为 CV_32F 或 CV_64F
    // 2. rows 必须为 2
    // 3. cols 必须为 3
    if ((mat.type() == CV_32F || mat.type() == CV_64F)
    && mat.rows == kEstimateRows && mat.cols == kEstimateCols) {
        return SUCCESS;
    }

    INFO_LOG("转换矩阵不匹配，矩阵类型：%d，行：%d，列：%d",
    mat.type(), mat.rows, mat.cols);
    return FAILED;
}

/* 人脸对齐和翻转 */
Result FaceFeatureExtraction::AlignedAndFlipFace(const FaceImage & face_img, const ImageData & resized_image,
int32_t index, vector < AlignedFace1 > & aligned_imgs) {
    // 步骤 1：将 NV12 转换为 BGR
    Mat bgr_img;
    if (!YUV420SPToBgr(resized_image, bgr_img)) {
        return FAILED;
    }

    // 步骤2：对齐人脸
    vector < cv::Point2f > dst_points;
    dst_points.emplace_back(cv::Point2f(kLeftEyeX, kLeftEyeY));
    dst_points.emplace_back(cv::Point2f(kRightEyeX, kRightEyeY));
    dst_points.emplace_back(cv::Point2f(kNoseX, kNoseY));
    dst_points.emplace_back(cv::Point2f(kLeftMouthCornerX, kLeftMouthCornerY));
    dst_points.emplace_back(cv::Point2f(kRightMouthCornerX, kRightMouthCornerY));

    // 关键点坐标应改为缩放后的图像的坐标
    vector < cv::Point2f > src_points;
    float face_width = (float)face_img.image.width;
    float face_height = (float)face_img.image.height;
    // 左眼
    float left_eye_x = face_img.feature_mask.left_eye.x / face_width * kResizeWidth;
    float left_eye_y = face_img.feature_mask.left_eye.y / face_height * kResizeHeight;
    src_points.emplace_back(left_eye_x, left_eye_y);
    // 右眼
    float right_eye_x = face_img.feature_mask.right_eye.x / face_width * kResizeWidth;
    float right_eye_y = face_img.feature_mask.right_eye.y / face_height * kResizeHeight;
    src_points.emplace_back(right_eye_x, right_eye_y);
    // 鼻子
    float nose_x = face_img.feature_mask.nose.x / face_width * kResizeWidth;
    float nose_y = face_img.feature_mask.nose.y / face_height * kResizeHeight;
    src_points.emplace_back(nose_x, nose_y);
    // 左嘴巴
    float left_mouth_x = face_img.feature_mask.left_mouth.x / face_width * kResizeWidth;
    float left_mouth_y = face_img.feature_mask.left_mouth.y / face_height * kResizeHeight;
    src_points.emplace_back(left_mouth_x, left_mouth_y);
    // 右嘴巴
    float right_mouth_x = face_img.feature_mask.right_mouth.x / face_width * kResizeWidth;
    float right_mouth_y = face_img.feature_mask.right_mouth.y / face_height * kResizeHeight;
    src_points.emplace_back(right_mouth_x, right_mouth_y);

    // 调用 OpenCV 进行对齐
    Mat point_estimate = estimateAffinePartial2D(src_points, dst_points);

    // 变换矩阵不匹配 warpAffine，设置成功
    if (!checkTransformMat(point_estimate)) {
        INFO_LOG("使用partialAffine估计RigidTransform失败，尝试使用fullAffine");
        point_estimate = estimateAffine2D(src_points, dst_points);
    }

    // 再次检查，如果不匹配，返回 FAILED
    if (!checkTransformMat(point_estimate)) {
        INFO_LOG("使用partialAffine 和fullAffine 的estimateRigidTransform都失败了。跳过这个图像。");
        INFO_LOG("left_eye=(%f, %f)", left_eye_x, left_eye_y);
        INFO_LOG("right_eye=(%f, %f)", right_eye_x, right_eye_y);
        INFO_LOG("nose=(%f, %f)", nose_x, nose_y);
        INFO_LOG("left_mouth=(%f, %f)", left_mouth_x, left_mouth_y);
        INFO_LOG("right_mouth=(%f, %f)", right_mouth_x, right_mouth_y);
        return FAILED;
    }

    cv::Size img_size(kResizeWidth, kResizeHeight);
    Mat aligned_img;
    warpAffine(bgr_img, aligned_img, point_estimate, img_size);

    // 步骤3：翻转图像（调用OpenCV）
    // 水平翻转
    Mat h_flip;
    flip(aligned_img, h_flip, kHorizontallyFlip);
    // 垂直和水平翻转
    Mat hv_flip;
    flip(h_flip, hv_flip, kVerticallyAndHorizontallyFlip);

    // 步骤4：改成RGB（因为AIPP需要这种格式）
    cvtColor(aligned_img, aligned_img, cv::COLOR_BGR2RGB);
    cvtColor(hv_flip, hv_flip, cv::COLOR_BGR2RGB);

    // Step5：返回对齐、翻转图像
    AlignedFace1 result;
    result.face_index = index;
    result.aligned_face = aligned_img;
    result.aligned_flip_face = hv_flip;
    aligned_imgs.emplace_back(result);
    return SUCCESS;
}

/* 预处理 */
void FaceFeatureExtraction::PreProcess(const vector < FaceImage > & face_imgs,
vector <AlignedFace1> & aligned_imgs) {
    // 循环每个裁剪的人脸图像
    for (int32_t index = 0; index < face_imgs.size(); ++index) {
        // 检查标志，如果 FAILED则contine
        if (!face_imgs[index].feature_mask.flag) {
            INFO_LOG("标志失败，跳过它");
            continue;
        }

        // 图像缩放
        ImageData resized_image;
        if (!Resize(face_imgs[index], resized_image)) {
            continue;
        }

        // 图片对齐和翻转
        if (!AlignedAndFlipFace(face_imgs[index], resized_image,
        index, aligned_imgs)) {
            continue;
        }
    }
}

/* 准备模型输入的buffer */
Result FaceFeatureExtraction::PrepareBuffer(int32_t index,
shared_ptr < uint8_t > & buffer,
uint32_t buffer_size, uint32_t each_img_size,
const vector < AlignedFace1 > & aligned_imgs) {
    uint32_t last_size = 0;

    AlignedFace1 face_img = aligned_imgs[index];

    // 复制对齐的人脸图像
    int ret = Utils::memcpy_s(buffer.get() + last_size,
    buffer_size - last_size,
    face_img.aligned_face.ptr <uint8_t>(), each_img_size);
    CHECK_MEM_OPERATOR_RESULTS(ret);
    last_size += each_img_size;

    // 复制对齐后翻转的人脸图像
    ret = Utils::memcpy_s(buffer.get() + last_size, buffer_size - last_size,
    face_img.aligned_flip_face.ptr < uint8_t > (), each_img_size);
    CHECK_MEM_OPERATOR_RESULTS(ret);
    last_size += each_img_size;

    return SUCCESS;
}

/* 分析结果 */
Result FaceFeatureExtraction::ArrangeResult(int32_t index,
float* inference_result,
const uint32_t inference_size,
const vector < AlignedFace1 > & aligned_imgs, vector < FaceImage > & face_imgs) {
    // 每个图像需要1024维向量
    if (kEveryFaceResVecCount > inference_size) {
        INFO_LOG("推理结果大小不正确。 得到的特征长度为:%d，需要的长度为:%d",
        inference_size, kEveryFaceResVecCount);
        return FAILED;
    }

    // 复制结果
    float result[inference_size];
    int ret = Utils::memcpy_s(result, sizeof(result), inference_result, sizeof(result));
    CHECK_MEM_OPERATOR_RESULTS(ret);

    int32_t org_crop_index = aligned_imgs[index].face_index;
    for (int j = 0; j < kEveryFaceResVecCount; ++j) {
        face_imgs[org_crop_index].feature_vector.emplace_back(result[j]);
    }

    return SUCCESS;
}

/* 模型推理 */
Result FaceFeatureExtraction::Inference(const vector < AlignedFace1 > & aligned_imgs, vector < FaceImage > & face_imgs) {
    // 初始化缓冲区
    uint32_t each_img_size = aligned_imgs[0].aligned_face.total()
    * aligned_imgs[0].aligned_face.channels();
    uint32_t buffer_size = each_img_size * kBatchImgCount;

    shared_ptr < uint8_t > buffer = shared_ptr<uint8_t>(new uint8_t[buffer_size], default_delete < uint8_t[]>());

    for (int32_t index = 0; index < aligned_imgs.size(); ++index) {
        // 1. 准备输入缓冲区
        if (!PrepareBuffer(index, buffer, buffer_size, each_img_size, aligned_imgs)) {
            ERROR_LOG("准备输入缓冲区失败");
            continue;
        }

        // 2. 准备输入数据
        Result ret = ResourceLoad::GetInstance().GetModel(4).CreateInput(buffer.get(), buffer_size);

        // 3. 推理
        ret = ResourceLoad::GetInstance().GetModel(4).Execute();
        if (ret != SUCCESS) {
            ResourceLoad::GetInstance().GetModel(4).DestroyInput();
            ERROR_LOG("运行模型推理失败\n");
            return FAILED;
        }
        ResourceLoad::GetInstance().GetModel(4).DestroyInput();

        // 4. 获取输出数据
        aclmdlDataset* recognition_inference = ResourceLoad::GetInstance().GetModel(4).GetModelOutputData();
        uint32_t inference_size = 0;
        float* inference_result = (float *)ResourceLoad::GetInstance().GetInferenceOutputItem(inference_size, recognition_inference, 0);

        // 5. 推理结果分析
        if (!ArrangeResult(index, inference_result, inference_size, aligned_imgs, face_imgs)) {
            ERROR_LOG("分析推理结果失败");
            continue;
        }
    }
    return SUCCESS;
}

void FaceFeatureExtraction::SendResult(shared_ptr < FaceRecognitionInfo > & image_handle) {
    ResourceLoad::GetInstance().SendNextModelProcess("FaceFeatureExtraction", image_handle);
}


static struct timespec time1 = {
    0, 0
};
static struct timespec time2 = {
    0, 0
};
Result FaceFeatureExtraction::Process(shared_ptr < FaceRecognitionInfo > & image_handle) {
    if (image_handle->err_info.err_code != AppErrorCode::kNone) {
        INFO_LOG("前面推理处理错误, 错误代码为:%d, 错误消息为:%s",
        (int)image_handle->err_info.err_code, image_handle->err_info.err_msg.c_str());
        SendResult(image_handle);
        return FAILED;
    }

    clock_gettime(CLOCK_REALTIME, &time1);

    // 预处理
    vector <AlignedFace1> aligned_imgs;
    PreProcess(image_handle->face_imgs, aligned_imgs);

    // 推理以及分析
    if (!aligned_imgs.empty()) {
        Inference(aligned_imgs, image_handle->face_imgs);
    }

    clock_gettime(CLOCK_REALTIME, &time2);

    if(!image_handle->face_imgs.empty()){
        INFO_LOG("人脸特征提取耗时 %ld 毫秒\n",
        (time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_nsec - time1.tv_nsec) / 1000000);
    }

    SendResult(image_handle);
    return SUCCESS;
}