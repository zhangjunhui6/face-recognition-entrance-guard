#include "ascenddk/presenter/agent/net/socket_factory.h"

#include <cstring>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ascenddk/presenter/agent/errors.h"
#include "ascenddk/presenter/agent/util/logging.h"
#include "ascenddk/presenter/agent/util/socket_utils.h"

using namespace std;

namespace ascend {
    namespace presenter {

        // anonymous namespace for constants
        namespace {

            // Default Socket Timeout
            const int kDefaultTimeoutInSec = 3;

        } /* anonymous namespace */

        PresenterErrorCode SocketFactory::GetErrorCode() const {
          return error_code_;
        }

        void SocketFactory::SetErrorCode(PresenterErrorCode error_code) {
          this->error_code_ = error_code;
        }

        // common function for creating a socket with given hostIp and port
        int SocketFactory::CreateSocket(const string& host_ip, uint16_t port) {
          // parse address
          sockaddr_in addr;
          if (!socketutils::SetSockAddr(host_ip.c_str(), port, addr)) {
            AGENT_LOG_ERROR("Invalid address: %s:%d", host_ip.c_str(), port);
            SetErrorCode(PresenterErrorCode::kInvalidParam);
            return socketutils::kSocketError;
          }

          // create socket file descriptor
          int sock = socketutils::CreateSocket();
          if (sock == socketutils::kSocketError) {
            AGENT_LOG_ERROR("socket() error: %s", strerror(errno));
            SetErrorCode(PresenterErrorCode::kConnection);
            return socketutils::kSocketError;
          }

          // reuse address
          socketutils::SetSocketReuseAddr(sock);

          // set timeout
          socketutils::SetSocketTimeout(sock, kDefaultTimeoutInSec);

          // do connect
          if (socketutils::Connect(sock, addr) == socketutils::kSocketError) {
            if (errno == EINVAL) {
              SetErrorCode(PresenterErrorCode::kInvalidParam);
            } else {
              SetErrorCode(PresenterErrorCode::kConnection);
            }

            AGENT_LOG_ERROR("Failed to connect to server: %s:%u", host_ip.c_str(), port);

            // connect failed, close socket
            (void) close(sock);
            return socketutils::kSocketError;
          }

          // connect successfully
          SetErrorCode(PresenterErrorCode::kNone);
          AGENT_LOG_INFO("Connected to server %s:%d, socket file descriptor = %d",
                         host_ip.c_str(), port, sock);
          return sock;
        }

    } /* namespace presenter */
} /* namespace ascend */
