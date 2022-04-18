#ifndef ASCENDDK_PRESENTER_AGENT_NET_RAW_SOCKET_H_
#define ASCENDDK_PRESENTER_AGENT_NET_RAW_SOCKET_H_

#include "ascenddk/presenter/agent/errors.h"
#include "ascenddk/presenter/agent/net/socket.h"

namespace ascend {
    namespace presenter {

        /**
         * RawSocket, the data is not encrypted
         */
        class RawSocket : public Socket {
            public:
                /**
                * @brief Factory method
                * @param [in] socket               socket file descriptor
                */
                static RawSocket* New(int socket);

                /**
                * @brief Constructor 隐式类型转换 (构造函数的隐式调用)
                * @param [in] socket               socket file descriptor
                */
                explicit RawSocket(int socket);

                // Disable copy constructor and assignment operator
                RawSocket(const RawSocket& other) = delete;
                RawSocket& operator=(const RawSocket& other) = delete;

                /**
                * @brief Destructor
                */
                virtual ~RawSocket();

            protected:

                /**
                * @brief Read bytes from socket
                * @param [in] buffer               receive buffer
                * @param [in] size                 expected bytes
                * @return bytes received. -1 of read failed
                */
                virtual int DoRecv(char *buffer, int size) override;

                /**
                * @brief Write bytes to socket
                * @param [in] data                 bytes to send
                * @param [in] size                 size of data
                * @return bytes sent. -1 if send failed
                */
                virtual int DoSend(const char *data, int size) override;

            private:
                int socket_;
        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_NET_RAW_SOCKET_H_ */
