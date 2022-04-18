#include "ascenddk/presenter/agent/util/byte_buffer.h"

#include <netinet/in.h>

#include "ascenddk/presenter/agent/util/logging.h"
#include "ascenddk/presenter/agent/util/mem_utils.h"

using namespace google::protobuf;
using namespace google::protobuf::io;

using std::string;

namespace ascend {
namespace presenter {
    /* SharedByteBuffer */
    SharedByteBuffer SharedByteBuffer::Make(std::uint32_t size) {
      char *buffer = memutils::NewArray<char>(size);
      if (buffer == nullptr) {
        AGENT_LOG_ERROR("buffer new() failed");
        return SharedByteBuffer();
      }

      return SharedByteBuffer(buffer, size);
    }

    SharedByteBuffer::SharedByteBuffer()
        : buf_(nullptr),
          size_(0) {
    }

    SharedByteBuffer::SharedByteBuffer(char* buf, std::uint32_t size)
        : buf_(buf, std::default_delete<char[]>()),
          size_(size) {
    }

    const char* SharedByteBuffer::Get() const {
      return buf_.get();
    }

    char* SharedByteBuffer::GetMutable() const {
      return buf_.get();
    }

    uint32_t SharedByteBuffer::Size() const {
      return size_;
    }

    bool SharedByteBuffer::IsEmpty() const {
      return size_ == 0;
    }

    /* ByteBuffer */
    ByteBuffer::ByteBuffer(const char* buf, uint32_t size)
        : buf(buf),
          size(size) {
    }

    ByteBuffer::ByteBuffer()
        : buf(nullptr),
          size(0) {
    }

    const char* ByteBuffer::Get() const {
      return buf;
    }

    uint32_t ByteBuffer::Size() const {
      return size;
    }

    bool ByteBuffer::IsEmpty() const {
      return size == 0;
    }

    /* ByteBufferWriter */
    ByteBufferWriter::ByteBufferWriter(char* buf, int size)
        : buf_(buf),
          w_ptr_(buf),
          end_(buf + size) {
    }

    ByteBufferWriter::~ByteBufferWriter() {
    }

    void ByteBufferWriter::PutUInt8(uint8_t value) {
      PutBytes(&value, sizeof(value));
    }

    void ByteBufferWriter::PutUInt32(uint32_t value) {
      // host byte order to network byte order
      uint32_t converted = htonl(value);
      PutBytes(&converted, sizeof(converted));
    }

    void ByteBufferWriter::PutString(const string& value) {
      PutBytes(value.c_str(), value.size());
    }

    /* PutMessage */
    bool ByteBufferWriter::PutMessage(const ::google::protobuf::Message& msg) {
      bool result = msg.SerializePartialToArray(w_ptr_, end_ - w_ptr_); //Serialize the message and store it in the given byte array
      w_ptr_ += msg.ByteSize();
      return result;
    }

    void ByteBufferWriter::PutBytes(const void *data, size_t size) {
      ptrdiff_t remaining_bytes = end_ - w_ptr_;
      // validate buffer state
      if (remaining_bytes <= 0) {
        return;
      }

      if (size <= (size_t)remaining_bytes) {
          memcpy(w_ptr_, data, size);
          w_ptr_ += size;
          return;
      }

      AGENT_LOG_ERROR("memcpy error, buffer remains: %d, and requiring: %u", remaining_bytes, size);
      // memcpy failed, any following write will be meaningless,
      // So set wPtr after end, to set the buffer to a faulty state
      w_ptr_ += remaining_bytes + 1;
    }

    ByteBuffer ByteBufferWriter::GetBuffer() {
      if (buf_ == nullptr) {
        return ByteBuffer();
      }

      if (w_ptr_ > end_) {
        AGENT_LOG_ERROR("Buffer overflow");
        return ByteBuffer();
      }

      ptrdiff_t size = w_ptr_ - buf_;
      return ByteBuffer(buf_, size);
    }

    /* ByteBufferReader */
    ByteBufferReader::ByteBufferReader(const char* buf, int size)
        : r_ptr_(buf),
          end_(buf + size) {
    }

    uint8_t ByteBufferReader::ReadUInt8() {
      uint8_t value = *((uint8_t*) r_ptr_);
      r_ptr_ += sizeof(value);
      return value;
    }

    uint32_t ByteBufferReader::ReadUInt32() {
      // network byte order to host byte order
      uint32_t value = ntohl(*((uint32_t*) r_ptr_));
      r_ptr_ += sizeof(value);
      return value;
    }

    string ByteBufferReader::ReadString(int size) {
      string value(r_ptr_, size);
      r_ptr_ += size;
      return std::move(value);
    }

    /* Read Message */
    bool ByteBufferReader::ReadMessage(int size, Message &message) {
      // parse protobuf message
      if (!message.ParseFromArray(r_ptr_, size)) {
        return false;
      }

      r_ptr_ += size;
      return true;
    }

    int ByteBufferReader::RemainingBytes() {
      return end_ - r_ptr_;
    }

} /* namespace presenter */
} /* namespace ascend */
