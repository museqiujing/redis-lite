// resp_serializer.h
#ifndef RESP_SERIALIZER_H
#define RESP_SERIALIZER_H

#include <vector>
#include <string>

class RespSerializer {
public:
    static std::string serialize_simple_string(const std::string& value);
    static std::string serialize_error(const std::string& message);
    static std::string serialize_integer(long long value);
    static std::string serialize_bulk_string(const std::string& value);
    static std::string serialize_null_bulk_string();
    static std::string serialize_array(const std::vector<std::string>& values);
    static std::string serialize_null_array();
};

#endif // RESP_SERIALIZER_H
