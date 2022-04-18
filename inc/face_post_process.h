#ifndef FACE_POST_PROCESS_H_
#define FACE_POST_PROCESS_H_
#include "face_recognition_params.h"

#include <vector>
#include <cstdint>

#include "facial_recognition_message.pb.h"
#include "ascenddk/presenter/agent/presenter_channel.h"
#include "presenter_channels.h"

using namespace ascend::presenter;


class FacePostProcess {
    public:

    /**
     * @brief: 后处理程序
     * @param [in]: info 图像处理数据
     */
    Result Process(const shared_ptr < FaceRecognitionInfo > & info);

    private:

    /**
     * @brief: 检查发送消息到服务器的响应结果
     * @param [in]: 发送消息响应结果
     * @return: Result
     */
    Result CheckSendMessageRes(const ascend::presenter::PresenterErrorCode & error_code);

    /**
     * @brief: 发送人脸识别结果到Socket服务器
     * @param [in]: 图像处理数据
     * @return: Result
     */
    Result SendFeature(const shared_ptr < FaceRecognitionInfo > & info);

    /**
     * @brief: 响应人脸注册结果给Socket服务器
     * @param [in]: 图像处理数据
     * @return: Result
     */
    Result ReplyFeature(const shared_ptr < FaceRecognitionInfo > & info);


    Result GetOriginPic(const shared_ptr < FaceRecognitionInfo > & image_handle,
    ImageData & jpgImage, facial_recognition::FrameInfo & frame_info);

};

#endif