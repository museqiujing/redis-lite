// resp_serializer.h
#ifndef RESP_SERIALIZER_H
#define RESP_SERIALIZER_H

#include <vector>
#include <string>
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
};

#endif // RESP_SERIALIZER_H
