#ifndef MIND_CAMERA_H
#define MIND_CAMERA_H
#include "face_recognition_params.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>

#define CAMERAL_1 (0)
#define CAMERAL_2 (1)

#define CAMERADATASETS_INIT (0)
#define CAMERADATASETS_RUN  (1)
#define CAMERADATASETS_STOP (2)
#define CAMERADATASETS_EXIT (3)

#define PARSEPARAM_FAIL (-1)


class MindCamera {
    public:
    struct CameraDatasetsConfig {
        int fps;
        int channel_id;
        int image_format;
        int resolution_width;
        int resolution_height;
        string ToString() const;
    };

    enum CameraOperationCode {
        kCameraOk = 0,
        kCameraNotClosed = -1,
        kCameraOpenFailed = -2,
        kCameraSetPropertyFailed = -3,
    };

    MindCamera();

    ~MindCamera();

    bool Init();

    bool Process();


    private:
    shared_ptr < FaceRecognitionInfo > CreateBatchImageParaObj();

    CameraOperationCode PreCapProcess();

    bool DoCapProcess();

    int GetExitFlag();

    void SetExitFlag(int flag = CAMERADATASETS_STOP);


    private:
    typedef unique_lock < mutex > TLock;
    mutex mutex_;
    int exit_flag_;
    shared_ptr < CameraDatasetsConfig > config_;
    uint32_t frame_id_;
};

#endif