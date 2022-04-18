#ifndef ASCENDDK_PRESENTER_AGENT_NET_SOCKET_H_
#define ASCENDDK_PRESENTER_AGENT_NET_SOCKET_H_

#include <string>
#include <cstdint>

#include "ascenddk/presenter/agent/errors.h"

namespace ascend {
    namespace presenter {

        /**
         * Abstract Socket Class
         * Subclasses can override protected method to implement socket with SSL
         */
        class Socket {
            public:
                Socket() = default;
                virtual ~Socket() = default;

                // Disable copy constructor and assignment operator
                Socket(const Socket& other) = delete;
                Socket& operator=(const Socket& other) = delete;

                /**
                * @brief Write bytes to socket
                * @param [in] data                 bytes to send
                * @param [in] size                 size of data
                * @return PresenterErrorCode
                */
                PresenterErrorCode Send(const char *data, int size);

                /**
                * @brief Read bytes from socket
                * @param [in] buffer               receive buffer
                * @param [in] size                 expected bytes
                * @return PresenterErrorCode
                */
                PresenterErrorCode Recv(char *buf, int size);

            protected:

                /**
                * @brief Read bytes from socket
                * @param [in] buffer               receive buffer
                * @param [in] size                 expected bytes
                * @return bytes received
                */
                virtual int DoRecv(char *buffer, int size) = 0;

                /**
                * @brief Write bytes to socket
                * @param [in] data                 bytes to send
                * @param [in] size                 size of data
                * @return bytes sent
                */
                virtual int DoSend(const char *data, int size) = 0;

        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_NET_SOCKET_H_ */
