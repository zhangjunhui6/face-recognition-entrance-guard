#include "ascenddk/presenter/agent/net/socket.h"

#include "ascenddk/presenter/agent/util/logging.h"
#include "ascenddk/presenter/agent/util/socket_utils.h"

namespace ascend {
    namespace presenter {

        PresenterErrorCode Socket::Send(const char *data, int size) {
          int ret = DoSend(data, size);
          if (ret == socketutils::kSocketError) {
            return PresenterErrorCode::kConnection;
          }

          // check size of sent data
          if (ret < size) {
            AGENT_LOG_ERROR("Socket::Send() error, expect %d bytes, but sent %d", size,
                            ret);
            return PresenterErrorCode::kConnection;
          }

          AGENT_LOG_DEBUG("Socket::Send() succeeded, size = %d", size);
          return PresenterErrorCode::kNone;
        }

        PresenterErrorCode Socket::Recv(char *buffer, int size) {
          int ret = DoRecv(buffer, size);
          if (ret == socketutils::kSocketError) {
            return PresenterErrorCode::kConnection;
          } else if (ret == socketutils::kSocketTimeout) {
            return PresenterErrorCode::kSocketTimeout;
          }

          // check size of received data
          if (ret < size) {
            AGENT_LOG_ERROR("Socket::Recv() error, expect %d bytes, but received %d",
                            size, ret);
            return PresenterErrorCode::kConnection;
          }

          AGENT_LOG_DEBUG("Socket::Recv() succeeded, size = %d", size);
          return PresenterErrorCode::kNone;
        }

    } /* namespace presenter */
} /* namespace ascend */

