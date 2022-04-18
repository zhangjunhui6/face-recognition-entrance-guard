#ifndef ASCENDDK_PRESENTER_AGENT_UTIL_MEM_UTILS_H_
#define ASCENDDK_PRESENTER_AGENT_UTIL_MEM_UTILS_H_

#include <cstddef>
#include <new>

#include "ascenddk/presenter/agent/util/logging.h"

namespace ascend {
    namespace presenter {
        namespace memutils {

            /**
             * @brief util for creating array, no throw
             * @param [in] T      type of the array
             * @param [in] size   size of the array
             * @return array of type T
             */
            template<typename T>
            T* NewArray(size_t size) {
              if (size == 0) {
                AGENT_LOG_ERROR("New array with size = 0");
                return nullptr;
              }

              T* arr = new (std::nothrow) T[size];
              return arr;
            }

        } /* namespace memutils */
    } /* namespace presenter */
} /* namespace ascend */

#endif /* ASCENDDK_PRESENTER_AGENT_UTIL_MEM_UTILS_H_ */
