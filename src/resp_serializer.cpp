// resp_serializer.cpp
#include "resp_serializer.h"
#include <sstream>

std::string RespSerializer::serialize_simple_string(const std::string& value) {
    return "+" + value + "\r\n";
}

std::string RespSerializer::serialize_error(const std::string& message) {
    return "-ERR " + message + "\r\n";
}

std::string RespSerializer::serialize_integer(long long value) {
    std::ostringstream oss;
    oss << ":" << value << "\r\n";
    return oss.str();
}

std::string RespSerializer::serialize_bulk_string(const std::string& value) {
    std::ostringstream oss;
    oss << "$" << value.size() << "\r\n" << value << "\r\n";
    return oss.str();
}

std::string RespSerializer::serialize_null_bulk_string() {
    return "$-1\r\n";
}

std::string RespSerializer::serialize_array(const std::vector<std::string>& values) {
    std::ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto& value : values) {
        oss << serialize_bulk_string(value);
    }
    return oss.str();
}

std::string RespSerializer::serialize_null_array() {
    return "*-1\r\n";
}
