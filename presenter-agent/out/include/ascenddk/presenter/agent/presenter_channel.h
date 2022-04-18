#ifndef ASCENDDK_PRESENTER_AGENT_PRESENTER_CHANNEL_H_
#define ASCENDDK_PRESENTER_AGENT_PRESENTER_CHANNEL_H_

#include "ascenddk/presenter/agent/channel.h"
#include "ascenddk/presenter/agent/errors.h"
#include "ascenddk/presenter/agent/presenter_types.h"

namespace ascend {
    namespace presenter {
        /**
         * @brief Open a channel to presenter server
         * @param [out] channel       channel must be a NULL pointer,
         *                            and it will point to an opened channel if
         *                            open successfully
         * @param [in]  param         parameters for opening a channel
         * @return PresenterErrorCode
         */
        PresenterErrorCode OpenChannel(Channel *&channel,
                                       const OpenChannelParam &param);
        /**
         * @brief Open a channel to presenter server by config file
         * @param [out] channel       channel must be a NULL pointer,
         *                            and it will point to an opened channel if
         *                            open successfully
         * @configFile [in]  param    config file of channel configuration
         * @return PresenterErrorCode
         */
        PresenterErrorCode OpenChannelByConfig(Channel*& channel,
                                               const char* configFile);
        /**
         * @brief Send the image to server for display through the given channel
         * @param [in] channel        the channel to send the image with
         * @param [in] image          the image to display
         * @return PresenterErrorCode
         */
        PresenterErrorCode PresentImage(Channel *channel, const ImageFrame &image);

        /**
         * @brief Send the image message to server for display through the given channel
         * @param [in] channel        the channel to send the image with
         * @param [in] message          a protobuf message
         * @return PresenterErrorCode
         */
        PresenterErrorCode SendMessage(Channel *channel,
                                       const google::protobuf::Message& message);

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_PRESENTER_CHANNEL_H_ */
