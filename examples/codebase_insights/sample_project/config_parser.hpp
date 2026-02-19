#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

// Circular dependency will be created with config_parser.cpp including utils.hpp
// and utils.hpp including config_parser.hpp

class ConfigParser {
public:
    void loadConfig(const char* filename);
    void saveConfig(const char* filename);
};

#endif // CONFIG_PARSER_HPP
