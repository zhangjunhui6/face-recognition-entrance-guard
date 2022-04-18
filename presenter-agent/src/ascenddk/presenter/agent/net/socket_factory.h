#ifndef ASCENDDK_PRESENTER_AGENT_NET_SOCKET_FACTORY_H_
#define ASCENDDK_PRESENTER_AGENT_NET_SOCKET_FACTORY_H_

#include "ascenddk/presenter/agent/net/socket.h"

#include <string>
#include <memory>


namespace ascend {
    namespace presenter {

        /**
         * Abstract SocketFactory for creating Socket
         * Subclasses implement Create() to return concrete instance
         */
        class SocketFactory {
            public:

                /**
                * Destructor
                */
                virtual ~SocketFactory() = default;

                /**
                * @brief Create instance of Socket, If NULL is returned,
                *        invoke GetErrorCode() for error code
                * @return pointer of Socket
                */
                virtual Socket* Create() = 0;

                /**
                * @brief Get error code
                */
                PresenterErrorCode GetErrorCode() const;

            protected:

                /**
                * @brief create a socket and connect to server
                * @param [in] host_ip              host IP
                * @param [in] port                 port
                * @return socket file descriptor, if SOCKET_ERROR(-1) is returned,
                *         invoke GetErrorCode() for error code
                */
                int CreateSocket(const std::string& host_ip, std::uint16_t port);

                /**
                * @brief Set error code
                * @param[in] error_code             error code
                */
                void SetErrorCode(PresenterErrorCode error_code);

            private:
                PresenterErrorCode error_code_ = PresenterErrorCode::kNone;
        };

    }
}

#endif /* ASCENDDK_PRESENTER_AGENT_NET_SOCKET_FACTORY_H_ */
