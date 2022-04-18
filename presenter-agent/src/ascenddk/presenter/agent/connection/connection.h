#ifndef ASCENDDK_PRESENTER_AGENT_CONNECTION_CONNECTION_H_
#define ASCENDDK_PRESENTER_AGENT_CONNECTION_CONNECTION_H_

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <google/protobuf/message.h>

#include "ascenddk/presenter/agent/codec/message_codec.h"
#include "ascenddk/presenter/agent/errors.h"
#include "ascenddk/presenter/agent/net/socket_factory.h"

namespace ascend {
    namespace presenter {

        /**
         * Connection between agent and server
         * provide protobuf based interface
         */
        class Connection {
            public:
                static Connection* New(Socket* socket);
                ~Connection() = default;

                /**
                * @brief Send a protobuf Message to presenter server
                * @param [in] message        protobuf message
                * @return PresenterErrorCode
                */
                PresenterErrorCode SendMessage(const ::google::protobuf::Message& message);

                /**
                * @brief Send a Message to presenter server
                * @param [in] message        PartialMessageWithTlvs
                * @return PresenterErrorCode
                */
                PresenterErrorCode SendMessage(const PartialMessageWithTlvs& message);

                /**
                * @brief Receive a message from presenter server
                * @param [out] message       response message
                * @return PresenterErrorCode
                */
                PresenterErrorCode ReceiveMessage(
                  std::unique_ptr<::google::protobuf::Message>& message);

            private:
                PresenterErrorCode DoSendMessage(const ::google::protobuf::Message& message,
                                               const std::vector<Tlv>& tlv_list);

                Connection(Socket* socket);

                /**
                * @brief Send tlv in protobuf format to server
                * @param [out] message       response message
                * @return PresenterErrorCode
                */
                PresenterErrorCode SendTlvList(const std::vector<Tlv>& tlv_list);

            private:

                // max size of received message
                static const int kBufferSize = 1024;

                std::unique_ptr<Socket> socket_;

                char recv_buf_[kBufferSize] = { 0 };

                std::mutex mtx_;

                MessageCodec codec_;
        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_CONNECTION_CONNECTION_H_ */
