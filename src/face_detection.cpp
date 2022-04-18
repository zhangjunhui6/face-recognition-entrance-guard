#include <vector>
#include <sstream>
#include <unistd.h>
#include "face_detection.h"
#include "dvpp_process.h"
#include <fstream>
#include "resource_load.h"

using namespace std;

namespace {
    const int32_t kResizeWidth = 300;
    const int32_t kResizeHeight = 300;

    // 推理输出数据缓冲区ID
    const int32_t kResultBufId = 1;
    // 每个结果大小（7 float）
    const int32_t kEachResultSize = 7;

    // 属性索引
    const int32_t kAttributeIndex = 1;
    // 分数索引
    const int32_t kScoreIndex = 2;
    // 人脸框坐标索引
    const int32_t kLeftTopXaxisIndex = 3;
    const int32_t kLeftTopYaxisIndex = 4;
    const int32_t kRightBottomXaxisIndex = 5;
    const int32_t kRightBottomYaxisIndex = 6;

    // 矫正率
    const float kMinRatio = 0.0;
    const float kMaxRatio = 1.0;
}

FaceDetection::FaceDetection() {
    confidence_ = -1.0;
}

/* 初始化，设置置信度(阈值) */
Result FaceDetection::Init() {
    confidence_ = 0.9;
    return SUCCESS;
}

/* 矫正比率 */
float FaceDetection::CorrectionRatio(float ratio) {
    float tmp = (ratio < kMinRatio) ? kMinRatio : ratio;
    return (tmp > kMaxRatio) ? kMaxRatio : tmp;
}

/* 判断检测结果的合理性 */
Result FaceDetection::IsValidResults(float attr, float score, const FaceRectangle & rectangle) {
    // 属性不是人脸（背景）
    if (abs(attr - 1.0) > 0.00001) {
        return FAILED;
    }

    // 置信度检查
    if ((score < 0.0) || (score > 1.0) || (score < confidence_)) {
        return FAILED;
    }

    // 人脸框位置检查
    if ((rectangle.lt.x == rectangle.rb.x)
    && (rectangle.lt.y == rectangle.rb.y)) {
        return FAILED;
    }

    return SUCCESS;
}

/* 预处理,图片缩放至300*300 */
Result FaceDetection::PreProcess(const shared_ptr < FaceRecognitionInfo > & image_handle, ImageData & resized_image) {
    // 输入大小小于零，返回失败
    int32_t img_size = image_handle->org_img.size;
    if (img_size <= 0)
    {
        ERROR_LOG("原始图像大小小于或等于零，大小为%d", img_size);
        return FAILED;
    }

    // 调用 dvpp 缩放图像
    Result ret = ResourceLoad::GetInstance().GetDvpp().Resize(resized_image, image_handle->org_img, kResizeWidth, kResizeHeight);
    if (ret == FAILED) {
        ERROR_LOG("图片放缩失败\n");
        return FAILED;
    }

    return SUCCESS;
}

/* 模型推理 */
Result FaceDetection::Inference(const ImageData & resized_image, aclmdlDataset*& detection_inference) {
    // 创建模型输入，指定输入数据
    Result ret = ResourceLoad::GetInstance().GetModel(1).CreateInput(resized_image.data.get(), resized_image.size);
    if (ret != SUCCESS) {
        ERROR_LOG("创建模型输入数据集失败\n");
        return FAILED;
    }

    // 模型执行
    ret = ResourceLoad::GetInstance().GetModel(1).Execute();
    if (ret != SUCCESS) {
        ResourceLoad::GetInstance().GetModel(1).DestroyInput();
        ERROR_LOG("执行模型推理失败\n");
        return FAILED;
    }

    // 销毁输入
    ResourceLoad::GetInstance().GetModel(1).DestroyInput();

    // 获取模型输出数据
    detection_inference = ResourceLoad::GetInstance().GetModel(1).GetModelOutputData();

    return SUCCESS;
}

/* 后处理，保存人脸检测框(1个) */
Result FaceDetection::PostProcess(shared_ptr < FaceRecognitionInfo > & image_handle,
aclmdlDataset* detection_inference) {
    // 推理结果向量只需要得到第一个结果
    uint32_t dataSize = 0;
    float* detectData = (float *)ResourceLoad::GetInstance().GetInferenceOutputItem(dataSize, detection_inference, kResultBufId);

    if (detectData == nullptr)
        return FAILED;

    // 原始图像的宽度和高度
    uint32_t width = image_handle->org_img.width;
    uint32_t height = image_handle->org_img.height;

    // 分析检测数据
    float * ptr = detectData;
    for (int32_t i = 0; i < dataSize - kEachResultSize; i += kEachResultSize) {
        ptr = detectData + i;

        // 属性
        float attr = ptr[kAttributeIndex];

        // 置信度
        float score = ptr[kScoreIndex];

        // 若小于70则说明没有人脸，很关键
        if (uint32_t(score * 100) < 70.0)
            break;

        // 人脸框位置
        FaceRectangle rectangle;
        rectangle.lt.x = CorrectionRatio(ptr[kLeftTopXaxisIndex]) * width;
        rectangle.lt.y = CorrectionRatio(ptr[kLeftTopYaxisIndex]) * height;
        rectangle.rb.x = CorrectionRatio(ptr[kRightBottomXaxisIndex]) * width;
        rectangle.rb.y = CorrectionRatio(ptr[kRightBottomYaxisIndex]) * height;

        // 检查推理结果的有效性, 无效则跳过
        if (!IsValidResults(attr, score, rectangle)) {
            continue;
        }

        // 展示人脸检测结果
        INFO_LOG("人脸检测结果为：左上角(%d,%d), 右下角(%d,%d)",
        rectangle.lt.x, rectangle.lt.y, rectangle.rb.x, rectangle.rb.y);

        // 将人脸检测结果加入到image_handle
        FaceImage faceImage;
        faceImage.rectangle = rectangle;
        image_handle->face_imgs.emplace_back(faceImage);
    }
    return SUCCESS;
}

/* 错误处理 */
void FaceDetection::HandleErrors(AppErrorCode err_code, const string & err_msg,
shared_ptr < FaceRecognitionInfo > & image_handle) {
    // 设置错误信息
    image_handle->err_info.err_code = err_code;
    image_handle->err_info.err_msg = err_msg;

    // 发送数据
    SendResult(image_handle);
}

/* 发送结果到下一步骤 */
void FaceDetection::SendResult(shared_ptr < FaceRecognitionInfo > & image_handle) {
    ResourceLoad::GetInstance().SendNextModelProcess("FaceDetection", image_handle);
}

static struct timespec time1 = {
    0, 0
};
static struct timespec time2 = {
    0, 0
};
Result FaceDetection::Process(shared_ptr < FaceRecognitionInfo > & image_handle) {

    string err_msg = "";
    if (image_handle->err_info.err_code != AppErrorCode::kNone) {
        ERROR_LOG("前面推理处理失败, 错误代码为:%d, 错误消息为:%s",
        int(image_handle->err_info.err_code), image_handle->err_info.err_msg.c_str());
        SendResult(image_handle);
        return FAILED;
    }

    clock_gettime(CLOCK_REALTIME, &time1);
    // 预处理
    ImageData resized_image;
    if (PreProcess(image_handle, resized_image) == FAILED) {
        err_msg = "人脸检测预处理失败.";
        HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
        return FAILED;
    }

    // 推理
    aclmdlDataset * detection_inference;
    if (Inference(resized_image, detection_inference) == FAILED) {
        err_msg = "人脸检测推理失败.";
        HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
        return FAILED;
    }

    // 后处理
    if (PostProcess(image_handle, detection_inference) == FAILED) {
        err_msg = "人脸检测后处理失败.";
        HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
        return FAILED;
    }
    clock_gettime(CLOCK_REALTIME, &time2);

    if (!image_handle->face_imgs.empty()) {
        INFO_LOG("人脸检测耗时 %ld 毫秒\n",
        (time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_nsec - time1.tv_nsec) / 1000000);
    }

    // 发送结果
    SendResult(image_handle);
    return SUCCESS;
}