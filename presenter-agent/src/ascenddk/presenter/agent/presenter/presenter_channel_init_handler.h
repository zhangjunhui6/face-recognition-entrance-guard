#ifndef ASCENDDK_PRESENTER_AGENT_PRESENTER_PRESENTER_CHANNEL_INIT_HANDLER_H_
#define ASCENDDK_PRESENTER_AGENT_PRESENTER_PRESENTER_CHANNEL_INIT_HANDLER_H_

#include <google/protobuf/message.h>

#include "ascenddk/presenter/agent/channel.h"
#include "ascenddk/presenter/agent/errors.h"
#include "ascenddk/presenter/agent/presenter_types.h"

namespace ascend {
    namespace presenter {

        /**
         * Presenter Channel Init Handler
         */
        class PresentChannelInitHandler : public InitChannelHandler {
            public:
                /**
                * @brief Constructor
                * @param [in] param          Open channel parameter
                */
                PresentChannelInitHandler(const OpenChannelParam& param);

                /**
                * @brief Create OpenChannelRequest
                * @return OpenChannelRequest
                */
                google::protobuf::Message* CreateInitRequest() override;

                /**
                * @brief Check OpenChannelResponse
                * @param [in] response       response
                * @return check result
                */
                bool CheckInitResponse(const google::protobuf::Message& response) override;

                /**
                * @brief Get ErrorCode
                * @return PresenterErrorCode
                */
                PresenterErrorCode GetErrorCode() const;

            private:
                OpenChannelParam param_;
                PresenterErrorCode error_code_ = PresenterErrorCode::kOther;
        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_PRESENTER_PRESENTER_CHANNEL_INIT_HANDLER_H_ */
