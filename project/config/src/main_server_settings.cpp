#include <string>
#include <utility>
#include <vector>
#include <fstream>

#include "main_server_settings.h"
#include "parse_functions.h"

const std::vector<std::string> MainServerSettings::valid_properties = {"http", "count_workflows", "access_log",
                                                                       "error_log", "server", "upstreams"};

MainServerSettings::MainServerSettings(std::string config_file_name) : config_file_name(std::move(config_file_name)) {
    parse_config(*this);
}

int MainServerSettings::get_number_of_properties(std::string property) {
    int begin = 0;
    while (isspace(property[begin])) {
        begin++;
    }
    int property_length;
    property_length = (property[property.length() - 1] == ':') ?
                      property.length() - begin - 1 : property.length() - begin;

    for (auto prop_iter = this->valid_properties.begin(); prop_iter != this->valid_properties.end(); prop_iter++) {
        if (property.substr(begin, property_length) == *prop_iter) {
            return prop_iter - this->valid_properties.begin();
        }
    }

    return -1;
}

void MainServerSettings::set_property(int number_of_property, std::string value) {
    int begin = 0;
    while (isspace(value[begin])) {
        begin++;
    }
    int value_length;
    value_length = (value[value.length() - 1] == ';') ?
                   value.length() - begin - 1 : value.length() - begin;
    switch (number_of_property) {
        case COUNT_WORKFLOWS_NUMBER:
            this->count_workflows = stoi(value.substr(begin, value_length));
            break;
        case ACCESS_LOG_NUMBER:
            this->access_log_file = value.substr(begin, value_length);
            this->is_access_log_file = true;
            break;
        case ERROR_LOG_NUMBER:
            this->error_log_file = value.substr(begin, value_length);
            this->is_error_log_file = true;
            break;
    }
}

void MainServerSettings::add_server() {
    this->is_any_server = true;
    if (this->is_access_log_file) {
        this->server.set_property(server.get_number_of_property("access_log"), this->access_log_file);
    }
    if (this->is_error_log_file) {
        this->server.set_property(server.get_number_of_property("error_log"), this->error_log_file);
    }
}

int MainServerSettings::get_count_workflows() {
    return this->count_workflows;
}

ServerSettings MainServerSettings::get_server() {
    return this->server;
}

std::string MainServerSettings::get_log_filename() {
    return this->log_filename;
}

std::string MainServerSettings::get_log_level() {
    return this->log_level;
}

