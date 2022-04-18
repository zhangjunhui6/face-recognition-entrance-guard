#include <vector>
#include <sstream>
#include <unistd.h>
#include "face_anti_spoofing.h"
#include "dvpp_process.h"
#include <fstream>
#include "resource_load.h"
#include<algorithm>


using namespace std;

namespace {
    const int32_t kResizeWidth = 80;
    const int32_t kResizeHeight = 80;

    const int kResultBufId = 1;
}

FaceAntiSpoofing::FaceAntiSpoofing() {
    confidence_ = -1.0;
}

/* 初始化，设置置信度(阈值) */
Result FaceAntiSpoofing::Init() {
    confidence_ = 0.9;
    return SUCCESS;
}

/* 根据人脸检测结果计算需要裁剪的区域 */
Result FaceAntiSpoofing::GetNewBox(u_int32_t src_w, u_int32_t src_h, vector < FaceImage > face_imgs, vector < FaceRectangle > & new_boxs) {
    for (vector < FaceImage >::iterator face_img_iter = face_imgs.begin(); face_img_iter != face_imgs.end(); ++face_img_iter) {
        u_int32_t x = face_img_iter->rectangle.lt.x;
        u_int32_t y = face_img_iter->rectangle.lt.y;
        u_int32_t box_w = face_img_iter->rectangle.rb.x - x;
        u_int32_t box_h = face_img_iter->rectangle.rb.y - y;

        float scale = min(1.0 * (src_h - 1) / box_h, min(1.0 * (src_w - 1) / box_w, 2.7));

        u_int32_t new_width = (u_int32_t)1.0 * box_w * scale;
        u_int32_t new_height = (u_int32_t)1.0 * box_h * scale;
        u_int32_t center_x = (u_int32_t)1.0 * box_w / 2 + x;
        u_int32_t center_y = (u_int32_t)1.0 * box_h / 2 + y;

        /* 出现过错误，不能使用无符号数u_int32_t */
        int32_t left_top_x = center_x - new_width / 2;
        int32_t left_top_y = center_y - new_height / 2;
        int32_t right_bottom_x = center_x + new_width / 2;
        int32_t right_bottom_y = center_y + new_height / 2;

        if (left_top_x < 0) {
            right_bottom_x -= left_top_x;
            left_top_x = 0;
        }

        if (left_top_y < 0) {
            right_bottom_y -= left_top_y;
            left_top_y = 0;
        }

        if (right_bottom_x > src_w - 1) {
            if (left_top_x - right_bottom_x + src_w - 1 >= 0) {
                left_top_x -= right_bottom_x - src_w + 1;
            }
            right_bottom_x = src_w - 1;
        }

        if (right_bottom_y > src_h - 1) {
            if (left_top_y - right_bottom_y + src_h - 1 >= 0) {
                left_top_y -= right_bottom_y - src_h + 1;
            }
            right_bottom_y = src_h - 1;
        }

        FaceRectangle new_box;
        new_box.lt.x = left_top_x;
        new_box.lt.y = left_top_y;
        new_box.rb.x = right_bottom_x;
        new_box.rb.y = right_bottom_y;

        new_boxs.push_back(new_box);
    }

    return SUCCESS;
}

/* 抠图出需要的那块区域 */
Result FaceAntiSpoofing::Crop(ImageData org_img, vector < FaceRectangle > new_boxs, vector < FaceImage > & face_imgs) {
    if (face_imgs.size() != new_boxs.size()) {
        return FAILED;
    }

    int i = 0;
    for (vector < FaceImage >::iterator face_img = face_imgs.begin(); face_img != face_imgs.end(); ++face_img, ++i) {
        FaceRectangle new_box = new_boxs[i];
        u_int32_t lt_x = new_box.lt.x % 2 == 0 ? new_box.lt.x : new_box.lt.x - 1;
        u_int32_t lt_y = new_box.lt.y % 2 == 0 ? new_box.lt.y : new_box.lt.y - 1;
        u_int32_t rb_x = new_box.rb.x % 2 == 1 ? new_box.rb.x : new_box.rb.x - 1;
        u_int32_t rb_y = new_box.rb.y % 2 == 1 ? new_box.rb.y : new_box.rb.y - 1;

        Result ret = ResourceLoad::GetInstance().GetDvpp().CropAndPasteImage(face_img->image, const_cast < ImageData & > (org_img),
        lt_x, lt_y, rb_x, rb_y);
        if (ret == FAILED) {
            ERROR_LOG("抠图失败\n");
            return FAILED;
        }
    }

    return SUCCESS;
}

/* 缩放到80*80 */
Result FaceAntiSpoofing::Resize(vector < FaceImage > face_imgs, vector < ImageData > & resized_imgs) {
    for (vector < FaceImage >::iterator face_img = face_imgs.begin(); face_img != face_imgs.end(); ++face_img) {
        int32_t img_size = face_img->image.size;
        if (img_size <= 0) {
            ERROR_LOG("图像大小小于或等于零，大小为%d", img_size);
            return FAILED;
        }

        ImageData resized_image;
        Result ret = ResourceLoad::GetInstance().GetDvpp().Resize(resized_image, const_cast < ImageData & > (face_img->image), kResizeWidth, kResizeHeight);
        if (ret == FAILED) {
            ERROR_LOG("图片缩放失败\n");
            return FAILED;
        }

        resized_imgs.push_back(resized_image);
    }

    return SUCCESS;
}

/* 预处理 */
Result FaceAntiSpoofing::PreProcess(shared_ptr < FaceRecognitionInfo > & image_handle,
vector < ImageData > & resized_imgs, string & err_msg) {
    ImageData org_img = image_handle->org_img;
    u_int32_t src_w = org_img.width;
    u_int32_t src_h = org_img.height;

    // 第一步，根据人脸检测结果获取要截取的图片区域；
    vector < FaceRectangle > new_boxs;
    if (!GetNewBox(src_w, src_h, image_handle->face_imgs, new_boxs)) {
        err_msg = "计算裁剪区域失败";
        return FAILED;
    }

    // 第二步，抠图
    if (!Crop(image_handle->org_img, new_boxs, image_handle->face_imgs)) {
        err_msg = "抠图失败";
        return FAILED;
    }

    // 第三步，缩放
    if (!Resize(image_handle->face_imgs, resized_imgs)) {
        err_msg = "图片缩放失败";
        return FAILED;
    }
    return SUCCESS;
}

/* 推理 */
Result FaceAntiSpoofing::Inference(vector < FaceImage > & face_imgs,
vector < ImageData > & resized_imgs) {
    int i = 0;
    for (vector < ImageData >::iterator resized_img = resized_imgs.begin(); resized_img != resized_imgs.end(); ++resized_img, ++i) {
        Result ret = ResourceLoad::GetInstance().GetModel(2).CreateInput(resized_img->data.get(), resized_img->size);
        if (ret != SUCCESS) {
            ERROR_LOG("创建模式输入数据集失败\n");
            return FAILED;
        }

        /* 模型推理 */
        ret = ResourceLoad::GetInstance().GetModel(2).Execute();
        if (ret != SUCCESS) {
            ResourceLoad::GetInstance().GetModel(2).DestroyInput();
            ERROR_LOG("执行模型推理失败\n");
            return FAILED;
        }

        /* 销毁输入 */
        ResourceLoad::GetInstance().GetModel(2).DestroyInput();

        /* 获取模型输出数据 */
        aclmdlDataset* anti_spoofing_inference = ResourceLoad::GetInstance().GetModel(2).GetModelOutputData();

        /* 分析推理结果 */
        uint32_t result_tensor_size = 0;
        float* inference_result = (float *)ResourceLoad::GetInstance().GetInferenceOutputItem(result_tensor_size, anti_spoofing_inference, 0);

        float score = inference_result[kResultBufId];
        face_imgs[i].alive_score = score;

        INFO_LOG("静默活体检测得到的活体分数为 %f", score);
    }

    return SUCCESS;
}

/* 错误处理 */
void FaceAntiSpoofing::HandleErrors(AppErrorCode err_code, const string & err_msg,
shared_ptr < FaceRecognitionInfo > & image_handle) {
    image_handle->err_info.err_code = err_code;
    image_handle->err_info.err_msg = err_msg;

    SendResult(image_handle);
}

/* 发送结果到下一步骤 */
void FaceAntiSpoofing::SendResult(shared_ptr < FaceRecognitionInfo > & image_handle) {
    ResourceLoad::GetInstance().SendNextModelProcess("FaceAntiSpoofing", image_handle);
}

static struct timespec time1 = {
    0, 0
};
static struct timespec time2 = {
    0, 0
};
Result FaceAntiSpoofing::Process(shared_ptr < FaceRecognitionInfo >& image_handle) {
    if (image_handle->err_info.err_code != AppErrorCode::kNone) {
        ERROR_LOG("前面推理出现错误, 错误代码为:%d, 错误消息为:%s",
        int(image_handle->err_info.err_code), image_handle->err_info.err_msg.c_str());
        SendResult(image_handle);
        return FAILED;
    }

    clock_gettime(CLOCK_REALTIME, &time1);
    string err_msg = "";
    vector < ImageData > resized_imgs;
    /* 预处理 */
    if (PreProcess(image_handle, resized_imgs, err_msg) == FAILED) {
        err_msg = "静默活体检测预处理： " + err_msg;
        HandleErrors(AppErrorCode::kAntiSpoofing, err_msg, image_handle);
        return FAILED;
    }

    /* 推理 */
    Result inference_flag = Inference(image_handle->face_imgs, resized_imgs);
    if (!inference_flag) {
        err_msg = "静默活体检测模型推理失败!";
        HandleErrors(AppErrorCode::kAntiSpoofing, err_msg, image_handle);
        return FAILED;
    }

    clock_gettime(CLOCK_REALTIME, &time2);

    if(!image_handle->face_imgs.empty()){
        INFO_LOG("静默活体检测耗时 %ld 毫秒\n",
        (time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_nsec - time1.tv_nsec) / 1000000);
    }

    /* 发送结果 */
    SendResult(image_handle);

    return SUCCESS;
}
