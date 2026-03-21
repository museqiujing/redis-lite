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

// iovec 序列化方法
std::vector<struct iovec> RespSerializer::serialize_simple_string_iovec(const SDS &value)
{
    std::vector<struct iovec> iovs;

    // "+" 前缀
    struct iovec iov1;
    iov1.iov_base = (void *)"+";
    iov1.iov_len = 1;
    iovs.push_back(iov1);

    // 字符串内容
    struct iovec iov2;
    iov2.iov_base = (void *)value.c_str();
    iov2.iov_len = value.size();
    iovs.push_back(iov2);

    // "\r\n" 后缀
    struct iovec iov3;
    iov3.iov_base = (void *)"\r\n";
    iov3.iov_len = 2;
    iovs.push_back(iov3);

    return iovs;
}

std::vector<struct iovec> RespSerializer::serialize_error_iovec(const SDS &message)
{
    std::vector<struct iovec> iovs;

    // "-" 前缀
    struct iovec iov1;
    iov1.iov_base = (void *)"-";
    iov1.iov_len = 1;
    iovs.push_back(iov1);

    // 错误信息
    struct iovec iov2;
    iov2.iov_base = (void *)message.c_str();
    iov2.iov_len = message.size();
    iovs.push_back(iov2);

    // "\r\n" 后缀
    struct iovec iov3;
    iov3.iov_base = (void *)"\r\n";
    iov3.iov_len = 2;
    iovs.push_back(iov3);

    return iovs;
}

std::vector<struct iovec> RespSerializer::serialize_integer_iovec(long long value)
{
    std::vector<struct iovec> iovs;

    // ":" 前缀
    struct iovec iov1;
    iov1.iov_base = (void *)":";
    iov1.iov_len = 1;
    iovs.push_back(iov1);

    // 整数字符串
    SDS value_str = std::to_string(value);
    struct iovec iov2;
    iov2.iov_base = (void *)value_str.c_str();
    iov2.iov_len = value_str.size();
    iovs.push_back(iov2);

    // "\r\n" 后缀
    struct iovec iov3;
    iov3.iov_base = (void *)"\r\n";
    iov3.iov_len = 2;
    iovs.push_back(iov3);

    return iovs;
}

std::vector<struct iovec> RespSerializer::serialize_bulk_string_iovec(const SDS &value)
{
    std::vector<struct iovec> iovs;

    if (value.empty())
    {
        // 空字符串情况
        struct iovec iov;
        iov.iov_base = (void *)"$-1\r\n";
        iov.iov_len = 5;
        iovs.push_back(iov);
        return iovs;
    }

    // "$" 前缀
    struct iovec iov1;
    iov1.iov_base = (void *)"$";
    iov1.iov_len = 1;
    iovs.push_back(iov1);

    // 长度字符串
    SDS len_str = std::to_string(value.size());
    struct iovec iov2;
    iov2.iov_base = (void *)len_str.c_str();
    iov2.iov_len = len_str.size();
    iovs.push_back(iov2);

    // "\r\n" 分隔符
    struct iovec iov3;
    iov3.iov_base = (void *)"\r\n";
    iov3.iov_len = 2;
    iovs.push_back(iov3);

    // 字符串内容
    struct iovec iov4;
    iov4.iov_base = (void *)value.c_str();
    iov4.iov_len = value.size();
    iovs.push_back(iov4);

    // "\r\n" 后缀
    struct iovec iov5;
    iov5.iov_base = (void *)"\r\n";
    iov5.iov_len = 2;
    iovs.push_back(iov5);

    return iovs;
}

std::vector<struct iovec> RespSerializer::serialize_null_bulk_string_iovec()
{
    std::vector<struct iovec> iovs;

    struct iovec iov;
    iov.iov_base = (void *)"$-1\r\n";
    iov.iov_len = 5;
    iovs.push_back(iov);

    return iovs;
}

std::vector<struct iovec> RespSerializer::serialize_array_iovec(const std::vector<SDS> &values)
{
    std::vector<struct iovec> iovs;

    if (values.empty())
    {
        // 空数组情况
        struct iovec iov;
        iov.iov_base = (void *)"*-1\r\n";
        iov.iov_len = 5;
        iovs.push_back(iov);
        return iovs;
    }

    // "*" 前缀
    struct iovec iov1;
    iov1.iov_base = (void *)"*";
    iov1.iov_len = 1;
    iovs.push_back(iov1);

    // 长度字符串
    SDS len_str = std::to_string(values.size());
    struct iovec iov2;
    iov2.iov_base = (void *)len_str.c_str();
    iov2.iov_len = len_str.size();
    iovs.push_back(iov2);

    // "\r\n" 分隔符
    struct iovec iov3;
    iov3.iov_base = (void *)"\r\n";
    iov3.iov_len = 2;
    iovs.push_back(iov3);

    // 每个元素
    for (const auto &value : values)
    {
        auto element_iovs = serialize_bulk_string_iovec(value);
        iovs.insert(iovs.end(), element_iovs.begin(), element_iovs.end());
    }

    return iovs;
}

std::vector<struct iovec> RespSerializer::serialize_null_array_iovec()
{
    std::vector<struct iovec> iovs;

    struct iovec iov;
    iov.iov_base = (void *)"*-1\r\n";
    iov.iov_len = 5;
    iovs.push_back(iov);

    return iovs;
}