// resp_parser.h
#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <vector>
#include <string>

class RespParser {
public:
    RespParser() : pos(0) {}
    
    std::vector<std::string> parse(const std::string& data);
    bool has_complete_request() const;
    std::string get_remaining_data() const;

private:
    size_t pos;
    std::string buffer;
    
    std::string parse_simple_string();
    std::string parse_error();
    std::string parse_integer();
    std::string parse_bulk_string();
    std::vector<std::string> parse_array();
    std::string read_line();
    void skip(size_t length);
};

#endif // RESP_PARSER_H
