#ifndef ASCENDDK_PRESENTER_AGENT_NET_RAW_SOCKET_FACTORY_H_
#define ASCENDDK_PRESENTER_AGENT_NET_RAW_SOCKET_FACTORY_H_

#include <memory>
#include <string>

#include "ascenddk/presenter/agent/net/raw_socket.h"
#include "ascenddk/presenter/agent/net/socket_factory.h"

namespace ascend {
    namespace presenter {

        /**
         * Factory of RawSocket
         */
        class RawSocketFactory : public SocketFactory {
            public:
                /**
                * @brief Constructor
                * @param hostIp                    host IP
                * @param port                      port
                */
                RawSocketFactory(const std::string& host_ip, uint16_t port);

                /**
                * @brief Create instance of RawSocket, If NULL is returned,
                *        Invoke GetErrorCode() for error code
                * @return pointer of RawSocket
                */
                virtual RawSocket* Create() override;

            private:
                std::string host_ip_;
                uint16_t port_;
        };

    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_NET_RAW_SOCKET_FACTORY_H_ */
