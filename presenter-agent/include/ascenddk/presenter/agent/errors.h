
#ifndef ASCENDDK_PRESENTER_AGENT_ERRORS_H_
#define ASCENDDK_PRESENTER_AGENT_ERRORS_H_

namespace ascend {
    namespace presenter {
        /**
         * PresenterErrorCode
         */
        enum class PresenterErrorCode {
            // Success, no error
            kNone = 0,

            // parameter check error
            kInvalidParam,

            // Connect to presenter server error
            kConnection,

            // SSL certification error
            kSsl,

            // Encode/Decode message error
            kCodec,

            // The given channel name is not created in server
            kNoSuchChannel,

            // The given channel is opened by another process
            kChannelAlreadyOpened,

            // Presenter server return unknown error
            kServerReturnedUnknownError,

            // Alloc object error
            kBadAlloc,

            // App returned error
            kAppDefinedError,

            // Timeout
            kSocketTimeout,

            // Uncategorized error
            kOther,
        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_ERRORS_H_ */
