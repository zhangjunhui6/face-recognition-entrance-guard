#ifndef ASCENDDK_PRESENTER_AGENT_CODEC_MESSAGE_CODEC_H_
#define ASCENDDK_PRESENTER_AGENT_CODEC_MESSAGE_CODEC_H_

#include <cstdint>
#include <google/protobuf/message.h>

#include "ascenddk/presenter/agent/channel.h"
#include "ascenddk/presenter/agent/util/byte_buffer.h"

namespace ascend {
    namespace presenter {

        /**
         * MessageCodec for encoding and decoding message
         *
         * A message has the following structure
         *    --------------------------------------------------------------------
         *    |Field Name          |  Size(bytes)   |    Type                     |
         *    --------------------------------------------------------------------
         *    |total message len   |       4        |    uint32                   |
         *    |-------------------------------------------------------------------
         *    |message name len    |       1        |    uint8                    |
         *    |-------------------------------------------------------------------
         *    |message name        |  Var. max 255  |  String, NO terminated '\0' |
         *    |-------------------------------------------------------------------
         *    |message body        |      Var.      |  Bytes. Encoded by protobuf |
         *    --------------------------------------------------------------------
         */
        class MessageCodec {
            public:
                // size of channel message total length
                static const int kPacketLengthSize = sizeof(uint32_t);

                /**
                * @brief Encode the message to a ByteBuffer
                * @param [in] message              message
                * @return ByteBuffer. Empty if encode failed
                */
                SharedByteBuffer EncodeMessage(const google::protobuf::Message& message);

                /**
                * @brief Encode the message to a ByteBuffer
                * @param [in] message              message
                * @return ByteBuffer. Empty if encode failed
                */
                SharedByteBuffer EncodeMessage(const PartialMessageWithTlvs& message);

                /**
                * @brief Encode the tag and length to a ByteBuffer
                * @param [in] Tlv                  Tlv
                * @return ByteBuffer. Empty if encode failed
                */
                SharedByteBuffer EncodeTagAndLength(const Tlv& tlv);

                /**
                * @brief Decode the message from buffer
                * @param [in] data                 data buffer
                * @param [in] size                 data size
                * @return Message. NULL if decode failed
                */
                google::protobuf::Message* DecodeMessage(const char* data, int size);

        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_CODEC_MESSAGE_CODEC_H_ */
