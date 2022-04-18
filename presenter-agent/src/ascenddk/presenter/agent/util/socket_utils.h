#ifndef ASCENDDK_PRESENTER_AGENT_UTIL_SOCKET_UTILS_H_
#define ASCENDDK_PRESENTER_AGENT_UTIL_SOCKET_UTILS_H_

#include <string>
#include <cstdint>
#include <netinet/in.h>

namespace ascend {
    namespace presenter {
        namespace socketutils {

            // indicating socket error
            const int kSocketError = -1;

            // indicating socket timeout
            const int kSocketTimeout = -11;

            /**
             * @brief SetNonBlocking
             * @param [in] socket               file descriptor of the socket
             * @param [in] nonblocking
             */
            void SetNonBlocking(int socket, bool nonblocking);

            /**
             * @brief SetSockAddr
             * @param [in] host_ip              host IP
             * @param [in] port                 port
             * @param [out] addr                address
             * @return true: success, false: failure
             */
            bool SetSockAddr(const char *host_ip, uint16_t port, sockaddr_in &addr);

            /**
             * @brief set reuse address option
             * @param [in]  socket              file descriptor of the socket
             */
            void SetSocketReuseAddr(int socket);

            /**
             * @brief set read timeout and write timeout to a socket
             * @param [in]  socket              file descriptor of the socket
             * @param [in]  timeout_in_sec      timeout in second
             */
            void SetSocketTimeout(int socket, int timeout_in_sec);

            /**
             * @brief Create a new socket
             * @return a file descriptor for the new socket, or SOCKET_ERROR(-1) for errors
             */
            int CreateSocket();

            /**
             * @brief Open a connection on socket FD to peer at ADDR
             * @param [in] socket               file descriptor of the socket
             * @param [in] addr                 peer address
             * @return 0 on success, -1 for errors.
             */
            int Connect(int socket, const sockaddr_in &addr);

            /**
             * @brief  Read N bytes into BUF from socket FD.
             * @param [in] socket               file descriptor of the socket
             * @param [out] buffer              buffer to write data to
             * @param [in] size                 size of data to read
             * @return the number read or -1 for errors.
             */
            int ReadN(int socket, char *buffer, int size);

            /**
             * @brief  Write N bytes into BUF to socket FD.
             * @param [in] socket               file descriptor of the socket
             * @param [in] data                 buffer of data to write to socket
             * @param [in] size                 size of data to write
             * @return the number wrote or -1 for errors.
             */
            int WriteN(int socket, const char *data, int size);

            /**
             * @brief close the socket
             * @param [in|out]  socket          file descriptor of the socket
             */
            void CloseSocket(int &socket);

        } /* namespace socketutils */
    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_UTIL_SOCKET_UTILS_H_ */
