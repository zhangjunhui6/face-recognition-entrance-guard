#include "face_key_point_detection.h"
#include "face_feature_train_mean.h"
#include "face_feature_train_std.h"
#include "resource_load.h"
using namespace cv;

namespace {
    const int32_t kResizedWidth = 40;
    const int32_t kResizedHeight = 40;

}

Result FaceKeyPointDetection::Init() {
    // 加载平均值，构造函数法(行，列，数据类型，数组指针)
    Mat train_mean_value(kResizedWidth, kResizedHeight, CV_32FC3, (void *)kTrainMean);
    train_mean_ = train_mean_value;
    if (train_mean_.empty()) {
        ERROR_LOG("加载平均值失败!");
        return FAILED;
    }

    // 加载标准值数据
    Mat train_std_value(kResizedWidth, kResizedHeight, CV_32FC3, (void *)kTrainStd);
    train_std_ = train_std_value;
    if (train_std_.empty()) {
        ERROR_LOG("加载标准值失败!");
        return FAILED;
    }

    return SUCCESS;
}

/* 抠图:抠出人脸框区域 */
Result FaceKeyPointDetection::Crop(const ImageData & org_img, vector < FaceImage > & face_imgs) {
    for (vector < FaceImage >::iterator face_img = face_imgs.begin(); face_img != face_imgs.end(); ++face_img) {
        u_int32_t lt_x = ((face_img->rectangle.lt.x) >> 1) << 1;
        u_int32_t lt_y = ((face_img->rectangle.lt.y) >> 1) << 1;

        u_int32_t rb_x = (((face_img->rectangle.rb.x) >> 1) << 1) - 1;
        u_int32_t rb_y = (((face_img->rectangle.rb.y) >> 1) << 1) - 1;

        Result ret = ResourceLoad::GetInstance().GetDvpp().CropAndPasteImage(face_img->image, const_cast < ImageData & > (org_img),
        lt_x, lt_y, rb_x, rb_y);
        if (ret == FAILED) {
            ERROR_LOG("抠图失败\n");
            return FAILED;
        }
    }

    return SUCCESS;
}

/* 缩放：缩放至40*40*/
Result FaceKeyPointDetection::Resize(vector < FaceImage > & face_imgs, vector < ImageData > & resized_imgs) {
    for (vector < FaceImage >::iterator face_img = face_imgs.begin(); face_img != face_imgs.end(); ++face_img) {
        int32_t img_size = face_img->image.size;
        if (img_size <= 0) {
            ERROR_LOG("图像大小小于或等于零，大小为%d", img_size);
            return FAILED;
        }

        ImageData resized_image;
        Result ret = ResourceLoad::GetInstance().GetDvpp().Resize(resized_image, const_cast < ImageData & > (face_img->image), kResizedWidth * 2, kResizedHeight * 2);
        if (ret == FAILED) {
            ERROR_LOG("图像缩放失败\n");
            return FAILED;
        }

        resized_imgs.push_back(resized_image);
    }

    return SUCCESS;
}

/* 图片格式转换: YUV -> BGR */
Result FaceKeyPointDetection::ImageYUV2BGR(vector < ImageData > & resized_imgs, vector < Mat > & bgr_imgs) {
    for (vector < ImageData >::iterator resized_img = resized_imgs.begin(); resized_img != resized_imgs.end(); ++resized_img) {
        int img_width = resized_img->width;
        int img_height = resized_img->height;

        // 将 resized_img->data 复制到 src
        Mat src(img_height * 3 / 2, img_width, CV_8UC1);
        int copy_size = img_width * img_height * 3 / 2;

        int destination_size = src.cols * src.rows * src.elemSize();
        int ret = Utils::memcpy_s(src.data, destination_size, resized_img->data.get(), copy_size);
        CHECK_MEM_OPERATOR_RESULTS(ret);

        // YUV 到 BGR，然后缩放
        Mat dst_temp;
        cvtColor(src, dst_temp, CV_YUV2BGR_NV12);
        Size dsize = Size(kResizedHeight, kResizedWidth);
        Mat dst = Mat(dsize, CV_32S);
        resize(dst_temp, dst, dsize);
        Mat dst32;
        dst.convertTo(dst32, CV_32FC3);
        bgr_imgs.push_back(dst32);
    }

    return SUCCESS;
}

/* 数据归一化 */
Result FaceKeyPointDetection::NormalizeData(vector < Mat > & bgr_imgs) {
    Result flag = SUCCESS;

    for (vector < Mat >::iterator iter = bgr_imgs.begin(); iter != bgr_imgs.end(); ++iter) {
        // 减去平均值并除以标准差
        *iter = *iter - train_mean_;
        *iter = *iter / train_std_;

        if ((*iter).empty()) {
            flag = FAILED;
        }
    }
    return flag;
}

/* 预处理 */
Result FaceKeyPointDetection::PreProcess(shared_ptr < FaceRecognitionInfo > & image_handle, vector < cv::Mat > & bgr_imgs, string & err_msg) {
    // 1.抠出人脸
    if (!Crop(image_handle->org_img, image_handle->face_imgs)) {
        err_msg = "Dvpp抠图失败";
        return FAILED;
    }

    // 2.缩放到模型输入需要的大小
    vector < ImageData > resized_imgs;
    if (!Resize(image_handle->face_imgs, resized_imgs)) {
        err_msg = "Dvpp缩放失败";
        return FAILED;
    }

    // 3.图片格式转换
    if (!ImageYUV2BGR(resized_imgs, bgr_imgs)) {
        err_msg = "OpenCV图像格式转换失败";
        return FAILED;
    }

    // 4.数据归一化
    if (!NormalizeData(bgr_imgs)) {
        err_msg = "OpenCV图像数据归一化失败";
        return FAILED;
    }
    return SUCCESS;
}

/* 复制Mat数据到float数组中 */
int FaceKeyPointDetection::CopyDataToBuffer(vector < Mat > & bgr_imgs, int i, float * tensor_buffer) {
    int last_size = 0;

    // 用opencv分割归一化数据
    vector < Mat > temp_splited_image;
    split(bgr_imgs[i], temp_splited_image);

    // 将数据复制到缓冲区
    for (vector < Mat >::iterator splited_data_iter = temp_splited_image.begin(); splited_data_iter != temp_splited_image.end(); ++splited_data_iter) {
        // 将数据类型更改为float
        (*splited_data_iter).convertTo(*splited_data_iter, CV_32FC3);
        int splited_data_length = (*splited_data_iter).rows * (*splited_data_iter).cols;
        int total_size = splited_data_length * sizeof(float);
        int ret = Utils::memcpy_s(tensor_buffer + last_size, total_size, (*splited_data_iter).ptr < float > (0), total_size);
        if (ret != SUCCESS) {
            ERROR_LOG("复制数据到内存缓冲区失败,错误码为:%d", ret);
            return 0;
        }

        // 计算内存的结束位置
        last_size += splited_data_length;
    }

    return last_size;
}

/* 模型推理，并分析得到关键点坐标 */
Result FaceKeyPointDetection::Inference(vector < Mat > & bgr_imgs,
vector < FaceImage > & face_imgs) {
    int img_size = bgr_imgs.size();

    for (int i = 0; i < img_size; i++) {
        float * tensor_buffer = new(nothrow)float[kResizedWidth * kResizedHeight * 3];
        if (tensor_buffer == nullptr) {
            ERROR_LOG("新建float缓冲区失败.");
            return FAILED;
        }

        // 复制数据到 tensor_buffer
        int last_size = CopyDataToBuffer(bgr_imgs, i, tensor_buffer);
        if (last_size == -1) {
            return FAILED;
        }

        /* 创建模型输入 */
        Result ret = ResourceLoad::GetInstance().GetModel(3).CreateInput(tensor_buffer, last_size * sizeof(float));
        if (ret != SUCCESS) {
            ERROR_LOG("创建模型输入失败\n");
            return FAILED;
        }

        /* 模型推理 */
        ret = ResourceLoad::GetInstance().GetModel(3).Execute();
        if (ret != SUCCESS) {
            ResourceLoad::GetInstance().GetModel(3).DestroyInput();
            ERROR_LOG("执行模型推理失败\n");
            return FAILED;
        }

        /* 销毁输入 */
        ResourceLoad::GetInstance().GetModel(3).DestroyInput();

        /* 获取模型输出数据 */
        aclmdlDataset* feature_mask_inference = ResourceLoad::GetInstance().GetModel(3).GetModelOutputData();
        uint32_t result_tensor_size = 0;
        float* inference_result = (float *)ResourceLoad::GetInstance().GetInferenceOutputItem(result_tensor_size, feature_mask_inference, 0);

        /* 分析结果 */
        FaceImage * face_image;
        face_image = &(face_imgs[i]);

        int width = face_image->image.width;
        int height = face_image->image.height;

        // 测试代码
        /*for(int i=0;i<10;i++){
            cout << i << ":" << inference_result[i] << endl;
        }*/

        face_image->feature_mask.left_eye.x = (int)((inference_result[kLeftEyeX] + 0.5) * width);
        face_image->feature_mask.left_eye.y = (int)((inference_result[kLeftEyeY] + 0.5) * height);
        face_image->feature_mask.right_eye.x = (int)((inference_result[kRightEyeX] + 0.5) * width);
        face_image->feature_mask.right_eye.y = (int)((inference_result[kRightEyeY] + 0.5) * height);
        face_image->feature_mask.nose.x = (int)((inference_result[kNoseX] + 0.5) * width);
        face_image->feature_mask.nose.y = (int)((inference_result[kNoseY] + 0.5) * height);
        face_image->feature_mask.left_mouth.x = (int)((inference_result[kLeftMouthX] + 0.5) * width);
        face_image->feature_mask.left_mouth.y = (int)((inference_result[kLeftMouthY] + 0.5) * height);
        face_image->feature_mask.right_mouth.x = (int)((inference_result[kRightMouthX] + 0.5) * width);
        face_image->feature_mask.right_mouth.y = (int)((inference_result[kRightMouthY] + 0.5) * height);
        face_image->feature_mask.flag = SUCCESS;

        INFO_LOG("人脸关键点检测推理结果: 左眼(%d, %d),右眼(%d, %d),鼻子(%d, %d),左嘴(%d, %d),右嘴(%d, %d)",
        face_image->feature_mask.left_eye.x, face_image->feature_mask.left_eye.y,
        face_image->feature_mask.right_eye.x, face_image->feature_mask.right_eye.y,
        face_image->feature_mask.nose.x, face_image->feature_mask.nose.y,
        face_image->feature_mask.left_mouth.x, face_image->feature_mask.left_mouth.y,
        face_image->feature_mask.right_mouth.x, face_image->feature_mask.right_mouth.y);
    }
    return SUCCESS;
}

/* 错误处理 */
void FaceKeyPointDetection::HandleErrors(AppErrorCode err_code, const string & err_msg,
shared_ptr < FaceRecognitionInfo > & image_handle) {
    image_handle->err_info.err_code = err_code;
    image_handle->err_info.err_msg = err_msg;

    SendResult(image_handle);
}

/* 发送数据到下一步骤 */
void FaceKeyPointDetection::SendResult(shared_ptr < FaceRecognitionInfo > & image_handle) {
    ResourceLoad::GetInstance().SendNextModelProcess("FaceKeyPointDetection", image_handle);
}

static struct timespec time1 = {
    0, 0
};
static struct timespec time2 = {
    0, 0
};
Result FaceKeyPointDetection::Process(shared_ptr < FaceRecognitionInfo >& image_handle) {

    if (image_handle->err_info.err_code != AppErrorCode::kNone) {
        ERROR_LOG("前面推理出错, 错误代码为:%d, 错误信息为:%s",
        int(image_handle->err_info.err_code), image_handle->err_info.err_msg.c_str());
        SendResult(image_handle);
        return FAILED;
    }

    clock_gettime(CLOCK_REALTIME, &time1);
    string err_msg = "";
    vector < Mat > bgr_imgs;
    /* 预处理 */
    if (PreProcess(image_handle, bgr_imgs, err_msg) == FAILED) {
        err_msg = "人脸关键点检测预处理，" + err_msg;
        HandleErrors(AppErrorCode::kAntiSpoofing, err_msg, image_handle);
        return FAILED;
    }

    /* 推理 */
    Result inference_flag = Inference(bgr_imgs, image_handle->face_imgs);
    if (!inference_flag) {
        err_msg = "人脸关键点检测推理失败!";
        HandleErrors(AppErrorCode::kAntiSpoofing, err_msg, image_handle);
        return FAILED;
    }

    if(bgr_imgs.empty()) {
        SendResult(image_handle);
        return SUCCESS;
    }

    clock_gettime(CLOCK_REALTIME, &time2);


    if(!image_handle->face_imgs.empty()){
        INFO_LOG("人脸关键点检测耗时 %ld 毫秒\n",
        (time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_nsec - time1.tv_nsec) / 1000000);
    }

    SendResult(image_handle);

    return SUCCESS;
}