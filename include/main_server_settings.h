#pragma once

#include <string>
#include <vector>

#include "server_settings.h"

class MainServerSettings {
private:
    std::string config_file_name;
    int count_workflows;
    std::string access_log_file;
    std::string error_log_file;
    ServerSettings server;

    bool is_access_log_file = false;
    bool is_error_log_file = false;
    bool is_any_server = false;

    enum numbers_of_properties {
        COUNT_WORKFLOWS_NUMBER = 1,
        ACCESS_LOG_NUMBER = 2,
        ERROR_LOG_NUMBER = 3
    };

public:
    static const std::vector<std::string> valid_properties;

    MainServerSettings() = default;

    explicit MainServerSettings(std::string config_file_name);


    MainServerSettings &operator=(const MainServerSettings &other) = default;

    int get_number_of_properties(std::string property);

    void set_property(int number_of_property, std::string value);

    void add_server();

    friend void parse_config(MainServerSettings &server);

    int get_count_workflows();

    ServerSettings get_server();
};