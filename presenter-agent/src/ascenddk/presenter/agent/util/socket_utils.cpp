#include "ascenddk/presenter/agent/util/socket_utils.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>

#include "ascenddk/presenter/agent/util/logging.h"

namespace {

    // no flag is need for now
    const int kSocketFlagNone = 0;

    // socket closed
    const int kSocketClosed = 0;

    // connect timeout
    const int kDefaultTimeoutInSec = 3;

    const int kSocketSuccess = 0;

    // indicating invalid socket file descriptor
    const int kSocketFdNull = -1;

    const int kReuseAddress = 1;

}

namespace ascend {
    namespace presenter {
        namespace socketutils {

            // set blocking mode 设置阻塞模型
            void SetNonBlocking(int socket, bool nonblocking) {
              // get original mask 获取原来的socket模式
              long mask = fcntl(socket, F_GETFL, NULL);
              if (nonblocking) {
                mask |= O_NONBLOCK;  // set nonblocking 非阻塞模式
              } else {
                mask &= ~O_NONBLOCK;  // unset nonblocking 阻塞模式
              }
              (void) fcntl(socket, F_SETFL, mask); //设置socket模式
            }

            // set socket addr
            bool SetSockAddr(const char *host_ip, uint16_t port, sockaddr_in &addr) {
              // valid port is 1~65535
              if (port == 0) {
                AGENT_LOG_ERROR("Invalid port: %d", port);
                return false;
              }

              // convert host and port to sockaddr
              memset(&addr, 0, sizeof(addr));

              addr.sin_family = AF_INET; //IPV4
              addr.sin_port = htons(port);
              if (inet_pton(AF_INET, host_ip, &addr.sin_addr) <= 0) {
                AGENT_LOG_ERROR("Invalid host IP: %s", host_ip);
                return false;
              }

              return true;
            }

            void SetSocketReuseAddr(int socket) {
              // set reuse address
              int so_reuse = kReuseAddress;  // 1
              int ret = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &so_reuse,
                                   sizeof(so_reuse));
              if (ret != kSocketSuccess) {
                AGENT_LOG_WARN("set socket opt SO_REUSEADDR failed");
              }
            }

            void SetSocketTimeout(int socket, int timeout_in_sec) {
              // initialize timeout
              timeval timeout = { timeout_in_sec, 0 };

              // set write timeout
              int ret = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                                   sizeof(timeout));
              if (ret != kSocketSuccess) {
                AGENT_LOG_WARN("set socket opt SO_SNDTIMEO failed");
              }

              // set read timeout
              ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
              if (ret != kSocketSuccess) {
                AGENT_LOG_WARN("set socket opt SO_RCVTIMEO failed");
              }
            }

            // 第一步，IPV4,面向连接的数据传输方式，使用TCP协议；
            int CreateSocket() {
              return ::socket(AF_INET, SOCK_STREAM,  IPPROTO_TCP );
            }

            // 第二步，建立TCP连接
            int Connect(int socket, const sockaddr_in& addr) {
              // Ignore SIGPIPE signals,服务器关闭，此时不要退出程序
              signal(SIGPIPE, SIG_IGN);

              // set nonblocking and connect with timeout 设置非阻塞模型
              SetNonBlocking(socket, true);

              // do connect
              int ret = ::connect(socket, (sockaddr*) &addr, sizeof(addr));
              if (ret < 0) {
                if (errno != EINPROGRESS) {
                  AGENT_LOG_ERROR("connect() error: %s", strerror(errno));
                  return kSocketError;
                }

                // select 函数用于在非阻塞中，当一个套接字有信号时通知你
                fd_set fdset; // 可写文件描述符
                FD_ZERO(&fdset); // 将指定的文件描述符清空
                FD_SET(socket, &fdset); // 用于在文件描述符中增加一个新的文件描述符
                // connect timeout = 3 seconds
                timeval tv = { kDefaultTimeoutInSec, 0 };
                int select_ret = select(socket + 1, NULL, &fdset, NULL, &tv);
                if (select_ret < 0) {  // error
                  AGENT_LOG_ERROR("select() error: %s", strerror(errno));
                  return kSocketError;
                }

                if (select_ret == 0) {  // no FD is ready
                  AGENT_LOG_ERROR("select() timeout");
                  return kSocketError;
                }

                int so_error = kSocketError;
                socklen_t len = sizeof(so_error);
                getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error != kSocketSuccess) {
                  AGENT_LOG_ERROR("getsockopt() error: %d", so_error);
                  return kSocketError;
                }
              }

              // reset to blocking mode 设置阻塞模式
              SetNonBlocking(socket, false);
              return kSocketSuccess;
            }

            // 第三步，接收数据
            int ReadN(int socket, char *buffer, int size) {
              int received_cnt = 0;
              // keep reading until nReceived == size
              while (received_cnt < size) {
                char *read_ptr = buffer + received_cnt;
                int ret = ::recv(socket, read_ptr, size - received_cnt, kSocketFlagNone);  // [false alarm]: will never write over size
                if (ret == kSocketError) {
                  if (errno == EAGAIN || errno == EINTR) {
                    AGENT_LOG_INFO("recv() timeout. error = %s", strerror(errno));
                    return kSocketTimeout;
                  }

                  AGENT_LOG_ERROR("recv() error. error = %s", strerror(errno));
                  return kSocketError;
                }

                if (ret == kSocketClosed) {
                  AGENT_LOG_ERROR("socket closed");
                  return kSocketError;
                }

                received_cnt += ret;
              }

              return received_cnt;
            }

            // 第三步，发送数据
            int WriteN(int socket, const char *data, int size) {
              int sent_cnt = 0;
              // keep reading until nReceived == size
              while (sent_cnt < size) {
                const char *write_ptr = data + sent_cnt;
                int ret = ::send(socket, write_ptr, size - sent_cnt, kSocketFlagNone);
                if (ret == kSocketError) {
                  AGENT_LOG_ERROR("send() error. errno = %s", strerror(errno));
                  return kSocketError;
                }

                if (ret == kSocketClosed) {
                  AGENT_LOG_ERROR("socket closed");
                  return kSocketError;
                }

                sent_cnt += ret;
              }

              return sent_cnt;
            }

            // 第四步，关闭Socket套接字
            void CloseSocket(int &socket) {
              if (socket >= 0) {
                (void) close(socket);
                socket = kSocketFdNull;
              }
            }

        } /* namespace sockutil */
    } /* namespace presenter */
} /* namespace ascend */

