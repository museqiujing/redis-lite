// resp_serializer.cpp
#include "resp_serializer.h"
#include <sstream>

SDS RespSerializer::serialize_simple_string(const SDS &value)
{
    return "+" + value + "\r\n";
}

SDS RespSerializer::serialize_error(const SDS &message)
{
    return "-" + message + "\r\n";
}

SDS RespSerializer::serialize_integer(long long value)
{
    std::ostringstream oss;
    oss << ":" << value << "\r\n";
    return oss.str();
}

SDS RespSerializer::serialize_bulk_string(const SDS &value)
{
    // 处理空字符串情况
    if (value.empty())
    {
        return "$-1\r\n";
    }

    SDS result;
    result += "$" + std::to_string(value.size()) + "\r\n";
    result += value;
    result += "\r\n";
    return result;
}

SDS RespSerializer::serialize_null_bulk_string()
{
    return "$-1\r\n";
}

SDS RespSerializer::serialize_array(const std::vector<SDS> &values)
{
    // 处理空数组情况
    if (values.empty())
    {
        return "*-1\r\n";
    }

    std::ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto &value : values)
    {
        oss << serialize_bulk_string(value);
    }
    return oss.str();
}

SDS RespSerializer::serialize_null_array()
{
    return "*-1\r\n";
}