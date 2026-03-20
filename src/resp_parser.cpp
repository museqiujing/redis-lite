// resp_parser.cpp
#include "resp_parser.h"
#include <sstream>
#include <stdexcept>
#include <iostream>
std::vector<SDS> RespParser::parse(const SDS &data)
{
    input_data_size = data.size();
    if (buffer.empty() && !data.empty())
    {
        buffer = data;
    }
    else
    {
        buffer += data;
    }
    std::vector<SDS> result;

    // 保存解析前的状态，用于错误恢复
    size_t original_pos = pos;
    size_t original_buffer_size = buffer.size();

    try
    {
        if (pos < buffer.size())
        {
            char type = buffer[pos];
            switch (type)
            {
            case '*':
                result = parse_array();
                break;
            case '+':
                result.push_back(parse_simple_string());
                break;
            case '-':
                result.push_back(parse_error());
                break;
            case ':':
                result.push_back(parse_integer());
                break;
            case '$':
                result.push_back(parse_bulk_string());
                break;
            default:
                throw std::runtime_error("Invalid RESP type: " + std::string(1, type));
            }
        }
    }
    catch (const std::exception &e)
    {
        // 解析失败，安全地保留未解析的数据
        if (original_pos < buffer.size())
        {
            buffer = buffer.substr(original_pos);
        }
        else
        {
            // 如果pos超出范围，保留整个缓冲区（可能是数据不完整）
            buffer.clear();
        }
        pos = 0;
        consumed = 0;
        throw;
    }

    // 解析成功，记录已解析字节数
    consumed = pos;

    // 优化缓冲区管理：避免不必要的substr操作
    if (pos == 0)
    {
        // 没有解析任何数据，保持缓冲区不变
    }
    else if (pos >= buffer.size())
    {
        // 解析了所有数据，清空缓冲区
        buffer.clear();
        pos = 0;
    }
    else
    {
        // 有未解析数据，保留剩余部分
        buffer = buffer.substr(pos);
        pos = 0;
    }

    return result;
}

bool RespParser::has_complete_request()
{
    if (buffer.empty())
    {
        return false;
    }

    size_t saved_pos = pos;

    try
    {
        const_cast<RespParser *>(this)->pos = 0;
        // 根据第一个字符的类型调用相应的解析函数
        char type = buffer[0];

        switch (type)
        {
        case '+':
            parse_simple_string();
            break;
        case '-':
            parse_error();
            break;
        case ':':
            parse_integer();
            break;
        case '$':
            parse_bulk_string();
            break;
        case '*':
            parse_array();
            break;
        default:
            const_cast<RespParser *>(this)->pos = saved_pos;

            return false; // 无效类型
        }

        const_cast<RespParser *>(this)->pos = saved_pos;

        return true;
    }
    catch (const std::runtime_error &e)
    {
        // 恢复解析位置
        const_cast<RespParser *>(this)->pos = saved_pos;
        // 如果是"Incomplete"错误，说明数据不完整
        std::string error_msg = e.what();
        std::cout << "[DEBUG] 运行时错误: " << error_msg << "\n";
        if (error_msg.find("Incomplete") != std::string::npos)
        {
            return false;
        }
        // 其他运行时错误说明数据格式错误
        return false;
    }
    catch (const std::exception &e)
    {
        // 恢复解析位置
        const_cast<RespParser *>(this)->pos = saved_pos;
        std::cout << "[DEBUG] 其他异常: " << e.what() << "\n";
        // 其他异常说明数据格式错误
        return false;
    }
}

SDS RespParser::get_remaining_data() const
{
    return buffer.substr(pos);
}

void RespParser::set_buffer(const SDS &data)
{
    buffer = data;
    pos = 0;
    consumed = 0;
}

void RespParser::reset_parser()
{
    buffer.clear();
    pos = 0;
    consumed = 0;
}

SDS RespParser::parse_simple_string()
{
    pos++;
    SDS line = read_line();
    return line;
}

SDS RespParser::parse_error()
{
    pos++;
    SDS line = read_line();
    return line;
}

SDS RespParser::parse_integer()
{
    pos++;
    SDS line = read_line();
    return line;
}

SDS RespParser::parse_bulk_string()
{
    pos++;
    SDS length_str = read_line();
    int length = std::stoi(length_str);

    if (length < 0)
    {
        return SDS();
    }

    size_t bulk_end = pos + length + 2;
    if (bulk_end > buffer.size())
    {
        throw std::runtime_error("Incomplete bulk string");
    }

    SDS bulk_str = buffer.substr(pos, length);
    skip(length + 2);
    return bulk_str;
}

std::vector<SDS> RespParser::parse_array()
{
    pos++;
    SDS length_str = read_line();
    int length = std::stoi(length_str);

    if (length < 0)
    {
        return {};
    }

    std::vector<SDS> array;
    for (int i = 0; i < length; i++)
    {
        char type = buffer[pos];
        switch (type)
        {
        case '+':
            array.push_back(parse_simple_string());
            break;
        case '-':
            array.push_back(parse_error());
            break;
        case ':':
            array.push_back(parse_integer());
            break;
        case '$':
            array.push_back(parse_bulk_string());
            break;
        case '*':
        {
            // 处理嵌套数组：递归解析
            std::vector<SDS> nested_array = parse_array();
            // 将嵌套数组的元素合并到当前数组中
            array.insert(array.end(), nested_array.begin(), nested_array.end());
            break;
        }
        default:
            throw std::runtime_error("Invalid RESP type in array");
        }
    }

    return array;
}

SDS RespParser::read_line()
{
    size_t end = buffer.find("\r\n", pos);
    if (end == std::string::npos)
    {
        throw std::runtime_error("Incomplete line");
    }

    SDS line = buffer.substr(pos, end - pos);
    pos = end + 2;
    return line;
}

void RespParser::skip(size_t length)
{
    pos += length;
    if (pos > buffer.size())
    {
        throw std::runtime_error("Skip beyond buffer");
    }
}