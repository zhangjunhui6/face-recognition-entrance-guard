#ifndef ASCENDDK_PRESENTER_AGENT_UTIL_LOGGING_H_
#define ASCENDDK_PRESENTER_AGENT_UTIL_LOGGING_H_

#include "acl/acl_base.h"
#include "cerrno"


// debug level logging
#define AGENT_LOG_DEBUG(fmt, ...) \
  aclAppLog(ACL_DEBUG, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// info level logging
#define AGENT_LOG_INFO(fmt, ...) \
  aclAppLog(ACL_INFO, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// warn level logging
#define AGENT_LOG_WARN(fmt, ...) \
  aclAppLog(ACL_WARNING, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// error level logging
#define AGENT_LOG_ERROR(fmt, ...) \
  aclAppLog(ACL_ERROR, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif /* ASCENDDK_PRESENTER_AGENT_UTIL_LOGGING_H_ */
