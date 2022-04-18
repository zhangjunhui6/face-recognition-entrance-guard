#include "ascenddk/presenter/agent/presenter/presenter_channel_init_handler.h"

#include "ascenddk/presenter/agent/presenter/presenter_message_helper.h"
#include "ascenddk/presenter/agent/util/logging.h"

using google::protobuf::Message;

namespace ascend {
    namespace presenter {

        PresentChannelInitHandler::PresentChannelInitHandler(
            const OpenChannelParam& param)
            : param_(param) {
        }

        google::protobuf::Message* PresentChannelInitHandler::CreateInitRequest() {
            proto::OpenChannelRequest *req =
                new (std::nothrow) proto::OpenChannelRequest();
                if (req != nullptr) {
                    error_code_ = PresenterMessageHelper::CreateOpenChannelRequest(
                        *req, param_.channel_name, param_.content_type);

                    if (error_code_ != PresenterErrorCode::kNone) {
                        delete req;
                        return nullptr;
                    }
                }

            return req;
        }

        bool PresentChannelInitHandler::CheckInitResponse(const Message& response) {
            error_code_ = PresenterMessageHelper::CheckOpenChannelResponse(response);
            if (error_code_ != PresenterErrorCode::kNone) {
                AGENT_LOG_ERROR("OpenChannel failed, error = %d", error_code_);
            }
            return error_code_ == PresenterErrorCode::kNone;
        }

        PresenterErrorCode PresentChannelInitHandler::GetErrorCode() const {
            return error_code_;
        }

    } /* namespace presenter */
} /* namespace ascend */
