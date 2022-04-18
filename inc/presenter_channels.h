#ifndef PRESENTER_CHANNELS_H
#define PRESENTER_CHANNELS_H

#include "facial_recognition_message.pb.h"
#include "ascenddk/presenter/agent/presenter_types.h"
#include "ascenddk/presenter/agent/channel.h"
#include "ascenddk/presenter/agent/presenter_channel.h"
#include "utils.h"
#include <mutex>
#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>

#define COMMENT_CHAR '#'
#define EQUALS_CHAR  '='
#define BLANK_SPACE_CHAR ' '
#define TABLE_CHAR '\t'

struct PresenterServerParams {
    // Socket服务器的IP地址
    string host_ip;
    // Socket服务器的端口
    uint16_t port;
    // 注册的app名称
    string app_id;
    // 注册的app类型
    string app_type;
};

class PresenterChannels {
    public:

    static PresenterChannels& GetInstance() {
        static PresenterChannels instance;
        return instance;
    }

    void Init(const PresenterServerParams& param) {
        param_ = param;
    }

    bool IsSpace(char c) {
        return (BLANK_SPACE_CHAR == c || TABLE_CHAR == c);
    }

    void Trim(string& str) {
        if (str.empty()) {
            return ;
        }
        uint32_t i, start_pos, end_pos;
        for (i = 0; i < str.size(); ++i) {
            if (!IsSpace(str[i])) {
                break;
            }
        }
        if (i == str.size()) {
            str = "";
            return ;
        }

        start_pos = i;

        for (i = str.size() - 1; i >= 0; --i) {
            if (!IsSpace(str[i])) {
                break;
            }
        }
        end_pos = i;

        str = str.substr(start_pos, end_pos - start_pos + 1);
    }

    // 分析param.conf信息，提取key=value信息
    bool AnalyseLine(const string& line, string& key, string& value) {
        if (line.empty())
            return false;
        int start_pos = 0, end_pos = line.size() - 1, pos;
        if ((pos = line.find(COMMENT_CHAR)) != -1) {
            if (0 == pos) {
                //第一个字符是 #
                return false;
            }
            end_pos = pos - 1;
        }
        string new_line = line.substr(start_pos, start_pos + 1 - end_pos);
        // 没有 =
        if ((pos = new_line.find(EQUALS_CHAR)) == -1)
            return false;

        key = new_line.substr(0, pos);
        value = new_line.substr(pos + 1, end_pos + 1 - (pos + 1));
        Trim(key);
        if (key.empty()) {
            return false;
        }
        Trim(value);
        return true;
    }

    // 读取 configFile ，获取 key=value 并将其保存到配置映射
    bool ReadConfig(map < string, string >& config, const char* configFile) {
        config.clear();
        ifstream infile(configFile);
        if (!infile) {
            cout << "配置文件打开失败" << endl;
            return false;
        }
        string line, key, value;
        while (getline(infile, line)) {
            if (AnalyseLine(line, key, value)) {
                config[key] = value;
            }
        }

        infile.close();
        return true;
    }

    // 获取人脸注册通道
    ascend::presenter::Channel* GetChannel() {
        if (intf_channel_ != nullptr) {
            return intf_channel_.get();
        }

        // 通过Socket服务器IP和端口号创建人脸注册通道
        ascend::presenter::ChannelFactory channel_factory;
        ascend::presenter::Channel * agent_channel = channel_factory.NewChannel(param_.host_ip, param_.port);

        //打开Socket通道
        ascend::presenter::PresenterErrorCode present_open_err = agent_channel->Open();
        if (present_open_err != ascend::presenter::PresenterErrorCode::kNone) {
            return nullptr;
        }

        // 构造注册app请求消息
        ascend::presenter::facial_recognition::RegisterApp app_register;
        app_register.set_id(param_.app_id);
        app_register.set_type(param_.app_type);

        // 构造响应消息
        unique_ptr < google::protobuf::Message > response;

        // 向服务器发送注册app请求
        ascend::presenter::PresenterErrorCode present_register_err = agent_channel
        ->SendMessage(app_register, response);
        if (present_register_err != ascend::presenter::PresenterErrorCode::kNone) {
            ERROR_LOG("注册app失败！");
            return nullptr;
        }

        // 得到响应消息并处理
        ascend::presenter::facial_recognition::CommonResponse* register_response =
        dynamic_cast < ascend::presenter::facial_recognition::CommonResponse* > (response.get());
        if (register_response == nullptr) {
            ERROR_LOG("注册app失败！");
            return nullptr;
        }
        ascend::presenter::facial_recognition::ErrorCode register_err = register_response->ret();
        if (register_err != ascend::presenter::facial_recognition::kErrorNone) {
            ERROR_LOG("注册app失败！");
            return nullptr;
        }
        intf_channel_.reset(agent_channel);
        return intf_channel_.get();
    }

    /* 获取人脸识别通道 */
    ascend::presenter::Channel* GetPresenterChannel() {
        // 通道已存在
        if (presenter_channel_ != nullptr) {
            return presenter_channel_.get();
        }

        // 通道不存在
        ascend::presenter::Channel * ch = nullptr;
        ascend::presenter::OpenChannelParam param;
        param.host_ip = param_.host_ip;
        param.port = param_.port;
        param.channel_name = param_.app_id;
        param.content_type = ascend::presenter::ContentType::kVideo;

        // 打开通道
        ascend::presenter::PresenterErrorCode error_code =
        ascend::presenter::OpenChannel(ch, param);

        // 打开通道失败
        if (error_code != ascend::presenter::PresenterErrorCode::kNone) {
            ERROR_LOG("打开通道失败! 错误码:%d", (int)error_code);
            return nullptr;
        }

        // 打开通道成功，设置为私有参数
        presenter_channel_.reset(ch);
        return presenter_channel_.get();
    }

    private:
    // 人脸注册通道
    unique_ptr < ascend::presenter::Channel > intf_channel_;

    // 人脸识别通道
    unique_ptr < ascend::presenter::Channel > presenter_channel_;

    // 通道参数
    PresenterServerParams param_;
};
#endif
