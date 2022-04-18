#ifndef FACE_REGISTER_ENGINE_H_
#define FACE_REGISTER_ENGINE_H_

#include "face_recognition_params.h"
#include "presenter_channels.h"
#include <iostream>
#include <string>
#include <dirent.h>
#include <memory>
#include <unistd.h>
#include <vector>
#include <cstdint>

const int32_t kMaxFaceIdLength = 50;

// sleep interval when queue full (unit:microseconds)
const __useconds_t kSleepInterval = 200000;

// function of dvpp returns success
const int kDvppOperationOk = 0;

// IP regular expression
const string kIpRegularExpression =
"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\."
"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$";

// port number range
const int32_t kPortMinNumber = 1024;
const int32_t kPortMaxNumber = 49151;

// app name regular expression
const string kAppNameRegularExpression = "[a-zA-Z0-9_]{3,20}";

// presenter server ip
const string kPresenterServerIP = "presenter_server_ip";

// presenter server port
const string kPresenterServerPort = "presenter_server_port";

// app name
const string kAppName = "app_name";

// app type
const string kAppType = "facial_recognition";

class FaceRegister {
    public:
    FaceRegister();

    ~FaceRegister() = default;

    bool Process();

    private:
    bool Init();

    bool DoRegisterProcess();
};

#endif