#include "ascenddk/presenter/agent/channel/default_channel.h"

namespace ascend {
    namespace presenter {
        Channel* ChannelFactory::NewChannel(const std::string& host_ip, uint16_t port) {
          return DefaultChannel::NewChannel(host_ip, port, nullptr);
        }

        Channel* ChannelFactory::NewChannel(
            const std::string& host_ip, uint16_t port,
            std::shared_ptr<InitChannelHandler> handler) {
          return DefaultChannel::NewChannel(host_ip, port, handler);
        }

    } /* namespace presenter */
} /* namespace ascend */
