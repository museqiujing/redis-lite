// resp_serializer.h
#ifndef RESP_SERIALIZER_H
#define RESP_SERIALIZER_H

#include <vector>
#include <string>
#include <sys/uio.h>
#include "sds.h"
class RespSerializer
{
public:
    static SDS serialize_simple_string(const SDS &value);
    static SDS serialize_error(const SDS &message);
    static SDS serialize_integer(long long value);
    static SDS serialize_bulk_string(const SDS &value);
    static SDS serialize_null_bulk_string();
    static SDS serialize_array(const std::vector<SDS> &values);
    static SDS serialize_null_array();

    // iovec 序列化支持
    static std::vector<struct iovec> serialize_simple_string_iovec(const SDS &value);
    static std::vector<struct iovec> serialize_error_iovec(const SDS &message);
    static std::vector<struct iovec> serialize_integer_iovec(long long value);
    static std::vector<struct iovec> serialize_bulk_string_iovec(const SDS &value);
    static std::vector<struct iovec> serialize_null_bulk_string_iovec();
    static std::vector<struct iovec> serialize_array_iovec(const std::vector<SDS> &values);
    static std::vector<struct iovec> serialize_null_array_iovec();
};

#endif // RESP_SERIALIZER_H
