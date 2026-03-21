// resp_parser.h
#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <vector>
#include <string>
#include "sds.h"
class RespParser
{
public:
    RespParser() : pos(0), consumed(0), input_data_size(0) {}

    std::vector<SDS> parse(const SDS &data);                    // 解析数据
    std::vector<std::vector<SDS>> parse_batch(const SDS &data); // 批量解析
    bool has_complete_request();                                // 检查是否有完整请求
    SDS get_remaining_data() const;                             // 获取剩余数据
    size_t get_consumed_bytes() const { return consumed; };     // 获取已消耗的字节数
    void set_buffer(const SDS &data);                           // 设置解析缓冲区
    void reset_parser();                                        // 重置解析器

private:
    size_t pos;             // 当前解析位置
    size_t consumed;        // 已消费的字节数
    size_t input_data_size; // 输入数据的总大小
    SDS buffer;

    SDS parse_simple_string();            // 解析简单字符串
    SDS parse_error();                    // 解析错误
    SDS parse_integer();                  // 解析整数
    SDS parse_bulk_string();              // 解析批量字符串
    std::vector<SDS> parse_array();       // 解析数组
    SDS read_line();                      // 读取一行数据
    void skip(size_t length);             // 跳过指定字节数
    bool has_complete_request_internal(); // 内部检查完整请求
};

#endif // RESP_PARSER_H
