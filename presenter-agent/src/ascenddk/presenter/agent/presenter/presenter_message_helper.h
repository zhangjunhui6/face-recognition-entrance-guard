#ifndef SRC_CHANNEL_PRESENTERMESSAGEHELPER_H_
#define SRC_CHANNEL_PRESENTERMESSAGEHELPER_H_

#include <memory>

#include "ascenddk/presenter/agent/errors.h"
#include "ascenddk/presenter/agent/presenter_types.h"
#include "proto/presenter_message.pb.h"

namespace ascend {
    namespace presenter {

        /**
         * Helper class for Presenter Messages
         */
        class PresenterMessageHelper {
            public:
                // helper class, constructor/destructor is not needed
                PresenterMessageHelper() = delete;
                ~PresenterMessageHelper() = delete;

                /**
                * @brief create OpenChannelRequest
                * @param [out] request         request to set the properties
                * @param [in] channelName      channel name
                * @param [in] contentType      content type
                * @return Shared pointer of OpenChannelRequest. nullptr is returned if
                *         any of the parameters is invalid
                */
                static PresenterErrorCode CreateOpenChannelRequest(
                  proto::OpenChannelRequest& request, const std::string& channel_name,
                  ContentType content_type);

                /**
                * @brief create PresentImageRequest
                * @param [out] request         request to set the properties
                * @param [in] image            image
                * @return true: success, false: failure
                */
                static bool InitPresentImageRequest(proto::PresentImageRequest& request,
                                                  const ImageFrame& image);

                /**
                * @brief Check OpenChannelResponse
                * @param [in] msg              Open Channel Response
                * @return PresenterErrorCode
                */
                static PresenterErrorCode CheckOpenChannelResponse(
                  const ::google::protobuf::Message& msg);

                /**
                * @brief Check PresentImageResponse
                * @param [in] msg              Present Image Response
                * @return PresenterErrorCode
                */
                static PresenterErrorCode CheckPresentImageResponse(
                  const ::google::protobuf::Message& msg);

            private:
                /**
                * @brief Translate OpenChannelErrorCode
                * @param [in] errorCode        Error code
                * @return PresenterErrorCode
                */
                static PresenterErrorCode TranslateErrorCode(
                  proto::OpenChannelErrorCode error_code);

                /**
                * @brief Translate PresentDataErrorCode
                * @param [in] errorCode        Error code
                * @return PresenterErrorCode
                */
                static PresenterErrorCode TranslateErrorCode(
                  proto::PresentDataErrorCode error_code);
        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* SRC_CHANNEL_PRESENTERMESSAGEHELPER_H_ */
