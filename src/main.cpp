#include <iostream>
#include <cstdlib>
#include <dirent.h>
#include <thread>
#include <ctime>
#include <fstream>

#include "resource_load.h"
#include "face_detection.h"
#include "face_post_process.h"
#include "utils.h"
#include "face_register.h"
#include "mind_camera.h"
#include <thread>
using namespace std;
extern "C" {
#include "driver/peripheral_api.h"

}

namespace{
    const char* kModelPath1 = "../model/face_detection.om";
    const char* kModelPath2 = "../model/2.7_80x80_MiniFASNetV2.om";
    const char* kModelPath3 = "../model/vanillacnn.om";
    const char* kModelPath4 = "../model/sphereface.om";
}

void CreateRegisterTask(aclrtContext context)
{
    aclrtSetCurrentContext(context);
    FaceRegister faceRegister;
    faceRegister.Process();
}

int main(int argc, char* argv[])
{
    ModelInfoParams param;
    param.modelPath1 = kModelPath1;
    param.modelPath2 = kModelPath2;
    param.modelPath3 = kModelPath3;
    param.modelPath4 = kModelPath4;

    INFO_LOG("开始初始化资源");
    Result ret = ResourceLoad::GetInstance().Init(param);
    if (ret != SUCCESS) {
        ERROR_LOG("资源初始化失败\n");
        return FAILED;
    }

    aclrtContext context;
    aclrtGetCurrentContext (& context);
    thread task1(CreateRegisterTask, ref(context));
    task1.detach();

    MindCamera mindCamera;
    mindCamera.Process();

    ResourceLoad::GetInstance().DestroyResource();
    return 0;
}