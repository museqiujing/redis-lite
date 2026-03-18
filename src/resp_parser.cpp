// resp_parser.cpp
#include "resp_parser.h"
#include <sstream>
#include <stdexcept>

std::vector<std::string> RespParser::parse(const std::string& data) {
    buffer += data;
    std::vector<std::string> result;
    
    try {
        if (pos < buffer.size()) {
            char type = buffer[pos];
            switch (type) {
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
                    throw std::runtime_error("Invalid RESP type");
            }
        }
    } catch (const std::exception& e) {
        // 解析失败，保留未解析的数据
        buffer = buffer.substr(pos);
        pos = 0;
        throw;
    }
    
    // 解析成功，清空缓冲区
    buffer.clear();
    pos = 0;
    return result;
}

bool RespParser::has_complete_request() const {
    if (buffer.empty()) return false;
    
    char type = buffer[0];
    switch (type) {
        case '*': {
            size_t newline_pos = buffer.find("\r\n");
            if (newline_pos == std::string::npos) return false;
            
            std::string length_str = buffer.substr(1, newline_pos - 1);
            int length = std::stoi(length_str);
            if (length < 0) return true;
            
            size_t current_pos = newline_pos + 2;
            for (int i = 0; i < length; i++) {
                if (current_pos >= buffer.size()) return false;
                
                char elem_type = buffer[current_pos];
                size_t elem_end = buffer.find("\r\n", current_pos);
                if (elem_end == std::string::npos) return false;
                
                if (elem_type == '$') {
                    std::string elem_length_str = buffer.substr(current_pos + 1, elem_end - current_pos - 1);
                    int elem_length = std::stoi(elem_length_str);
                    if (elem_length < 0) {
                        current_pos = elem_end + 2;
                    } else {
                        size_t elem_content_end = elem_end + 2 + elem_length + 2;
                        if (elem_content_end > buffer.size()) return false;
                        current_pos = elem_content_end;
                    }
                } else {
                    current_pos = elem_end + 2;
                }
            }
            return true;
        }
        case '+':
        case '-':
        case ':': {
            return buffer.find("\r\n") != std::string::npos;
        }
        case '$': {
            size_t newline_pos = buffer.find("\r\n");
            if (newline_pos == std::string::npos) return false;
            
            std::string length_str = buffer.substr(1, newline_pos - 1);
            int length = std::stoi(length_str);
            if (length < 0) return true;
            
            size_t content_end = newline_pos + 2 + length + 2;
            return content_end <= buffer.size();
        }
        default:
            return false;
    }
}

std::string RespParser::get_remaining_data() const {
    return buffer.substr(pos);
}

std::string RespParser::parse_simple_string() {
    pos++;
    std::string line = read_line();
    return line;
}

std::string RespParser::parse_error() {
    pos++;
    std::string line = read_line();
    return line;
}

std::string RespParser::parse_integer() {
    pos++;
    std::string line = read_line();
    return line;
}

std::string RespParser::parse_bulk_string() {
    pos++;
    std::string length_str = read_line();
    int length = std::stoi(length_str);
    
    if (length < 0) {
        return "";
    }
    
    size_t bulk_end = pos + length + 2;
    if (bulk_end > buffer.size()) {
        throw std::runtime_error("Incomplete bulk string");
    }
    
    std::string bulk_str = buffer.substr(pos, length);
    skip(length + 2);
    return bulk_str;
}

std::vector<std::string> RespParser::parse_array() {
    pos++;
    std::string length_str = read_line();
    int length = std::stoi(length_str);
    
    if (length < 0) {
        return {};
    }
    
    std::vector<std::string> array;
    for (int i = 0; i < length; i++) {
        char type = buffer[pos];
        switch (type) {
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
            default:
                throw std::runtime_error("Invalid RESP type in array");
        }
    }
    
    return array;
}

std::string RespParser::read_line() {
    size_t end = buffer.find("\r\n", pos);
    if (end == std::string::npos) {
        throw std::runtime_error("Incomplete line");
    }
    
    std::string line = buffer.substr(pos, end - pos);
    pos = end + 2;
    return line;
}

void RespParser::skip(size_t length) {
    pos += length;
    if (pos > buffer.size()) {
        throw std::runtime_error("Skip beyond buffer");
    }
}
