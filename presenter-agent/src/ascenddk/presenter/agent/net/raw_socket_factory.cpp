#include "ascenddk/presenter/agent/net/raw_socket_factory.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ascenddk/presenter/agent/util/logging.h"
#include "ascenddk/presenter/agent/util/socket_utils.h"

using std::string;

namespace ascend {
    namespace presenter {

        RawSocketFactory::RawSocketFactory(const string& host_ip, uint16_t port)
            : host_ip_(host_ip),
              port_(port) {
        }

        // overrided method of Create()
        RawSocket* RawSocketFactory::Create() {
          // create a socket and connect to server
          int sock = CreateSocket(host_ip_, port_);
          if (sock == socketutils::kSocketError) {
            return nullptr;
          }

          // No error, create RawSocket and return
          RawSocket *ret = RawSocket::New(sock);
          if (ret == nullptr) {
            (void) close(sock);
            SetErrorCode(PresenterErrorCode::kBadAlloc);
          }

          return ret;
        }

    } /* namespace presenter */
} /* namespace ascend */
