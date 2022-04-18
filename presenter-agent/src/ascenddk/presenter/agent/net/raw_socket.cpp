#include "ascenddk/presenter/agent/net/raw_socket.h"

#include "ascenddk/presenter/agent/util/logging.h"
#include "ascenddk/presenter/agent/util/socket_utils.h"

namespace ascend {
    namespace presenter {

        RawSocket* RawSocket::New(int socket) {
          return new (std::nothrow) RawSocket(socket);
        }

        RawSocket::RawSocket(int socket)
            : socket_(socket) {
        }

        RawSocket::~RawSocket() {
          socketutils::CloseSocket(socket_);
        }

        int RawSocket::DoSend(const char* data, int size) {
          return socketutils::WriteN(socket_, data, size);
        }

        int RawSocket::DoRecv(char* buf, int size) {
          return socketutils::ReadN(socket_, buf, size);
        }

    } /* namespace presenter */
} /* namespace ascend */
