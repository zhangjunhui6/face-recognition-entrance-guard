#ifndef ASCENDDK_PRESENTER_AGENT_DATA_TYPES_H_
#define ASCENDDK_PRESENTER_AGENT_DATA_TYPES_H_

#include <string>
#include <cstdint>
#include <vector>

namespace ascend {
    namespace presenter {
        /**
         * ContentType
         */
        enum class ContentType {
            // Image
            kImage = 0,

            // Video
            kVideo = 1,

            // Reserved content type, do not use this
            kReserved = 127,
        };

        /**
         * ImageFormat
         */
        enum class ImageFormat {
            // JPEG
            kJpeg = 0,

            // Reserved format, do not use this
            kReserved = 127,
        };

        /**
         * OpenChannelParam
         */
        struct OpenChannelParam {
            std::string host_ip;
            std::uint16_t port;
            std::string channel_name;
            ContentType content_type;
        };

        struct Point {
            std::uint32_t x;
            std::uint32_t y;
        };

        struct DetectionResult {
            Point lt;   //The coordinate of left top point
            Point rb;   //The coordinate of the right bottom point
            std::string result_text;  // Face:xx%
        };

        /**
         * ImageFrame
         */
        struct ImageFrame {
            ImageFormat format;
            std::uint32_t width;
            std::uint32_t height;
            std::uint32_t size;
            unsigned char *data;
            std::vector<DetectionResult> detection_results;
        };
    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_DATA_TYPES_H_ */
