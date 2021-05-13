#include <iostream>
#include <algorithm>
#include <regex>

#include "server_settings.h"
#include "exceptions_config_file.h"
#include "server_settings_exceptions.h"

#define STATICS_FOLDER_PATH "../statics"

const std::vector<std::string> ServerSettings::valid_properties = {"listen", "access_log", "error_log", "root",
                                                                   "location", "servername"};

const std::vector<std::string> ServerSettings::valid_location_properties = {"root", "add_root", "access_log", "error_log"};

int ServerSettings::get_number_of_property(std::string property) {
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

void ServerSettings::set_property(int number_of_property, std::string value) {
    int begin = 0;
    while (isspace(value[begin])) {
        begin++;
    }
    int value_length;
    value_length = (value[value.length() - 1] == ';') ?
                   value.length() - begin - 1 : value.length() - begin;
    switch (number_of_property) {
        case LISTEN_NUMBER:
            try {
                this->port = stoi(value.substr(begin, value_length));
            } catch (std::exception &exception) {
                throw InvalidConfigException("listen port can be only integer");
            }
            break;
        case ACCESS_LOG_NUMBER:
            this->access_log_file = value.substr(begin, value_length);
            break;
        case ERROR_LOG_NUMBER:
            this->error_log_file = value.substr(begin, value_length);
            break;
        case ROOT_NUMBER:
            this->root = STATICS_FOLDER_PATH + value.substr(begin, value_length);
            this->is_root = true;
            break;
        case SERVERNAME_NUMBER:
            this->servername = value.substr(begin, value_length);
            break;
    }
}

void ServerSettings::print_properties() {
    std::cout << this->access_log_file << std::endl;
    std::cout << this->error_log_file << std::endl;
    std::cout << this->root << std::endl;
    std::cout << this->port << std::endl;
    std::cout << this->servername << std::endl;
    std::cout << "urls" << std::endl;
    for (auto &i : this->exact_match_urls) {
        std::cout << "url: " << i.url << "; root: " << i.root << std::endl;
    }
    for (auto &i : this->preferential_prefix_urls) {
        std::cout << "url: " << i.url << "; root: " << i.root << std::endl;
    }
    for (auto &i : this->regex_match_urls) {
        std::cout << "url: " << i.url << "; root: " << i.root << std::endl;
    }
    for (auto &i : this->prefix_match_urls) {
        std::cout << "url: " << i.url << "; root: " << i.root << std::endl;
    }

}

int ServerSettings::get_number_of_location_property(std::string property) {
    int begin = 0;
    while (isspace(property[begin])) {
        begin++;
    }
    int property_length;
    property_length = (property[property.length() - 1] == ':') ?
                      property.length() - begin - 1 : property.length() - begin;

    for (auto prop_iter = this->valid_location_properties.begin();
         prop_iter != this->valid_location_properties.end(); prop_iter++) {
        if (property.substr(begin, property_length) == *prop_iter) {
            return prop_iter - this->valid_location_properties.begin();
        }
    }

    return -1;
}

void ServerSettings::set_location_property(int number_of_property, std::string value, location_t &location) {
    int begin = 0;
    while (isspace(value[begin])) {
        begin++;
    }
    int value_length;
    value_length = (value[value.length() - 1] == ';') ?
                   value.length() - begin - 1 : value.length() - begin;
    switch (number_of_property) {
        case ROOT_LOCATION_NUMBER:
            location.root = STATICS_FOLDER_PATH + value.substr(begin, value_length);
            break;
        case ADD_ROOT_NUMBER:
            location.root = this->root + value.substr(begin, value_length);
            break;
        case ACCESS_LOG_LOCATION_NUMBER:
            if (value.substr(begin, value_length) == "on") {
                location.is_access_log = true;
            } else if (value.substr(begin, value_length) == "off") {
                location.is_access_log = false;
            } else {
                throw InvalidConfigException("access_log field in 'location' must be only 'on' or 'off'");
            }
            break;
        case ERROR_LOG_LOCATION_NUMBER:
            if (value.substr(begin, value_length) == "on") {
                location.is_error_log = true;
            } else if (value.substr(begin, value_length) == "off") {
                location.is_error_log = false;
            } else {
                throw InvalidConfigException("error_log field in 'location' must be only 'on' or 'off'");
            }
            break;
    }
}


std::string ServerSettings::get_root(std::string url) {
    for (auto exact_match_url : exact_match_urls) {
        if (!exact_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(),
                           url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        if (url == exact_match_url.url) {
            return exact_match_url.root;
        }
    }

    for (auto preferential_match_url : preferential_prefix_urls) {
        if (!preferential_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(),
                           url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        if (url.find(preferential_match_url.url) == 0) {
            return preferential_match_url.root;
        }
    }

    for (auto regex_match_url : regex_match_urls) {
        if (!regex_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(),
                           url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        std::regex regex(regex_match_url.url);
        if (std::regex_match(url.cbegin(), url.cend(), regex)) {
            return regex_match_url.root;
        }
    }

    for (auto prefix_match_url : prefix_match_urls) {
        if (!prefix_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(),
                           url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        if (url.find(prefix_match_url.url) == 0) {
            return prefix_match_url.root;
        }
    }

    throw RootNotFoundException("404");
}

void ServerSettings::add_exact_match_url(location_t &location) {
    this->exact_match_urls.push_back(location);
}

void ServerSettings::add_preferential_prefix_urls(location_t &location) {
    this->preferential_prefix_urls.push_back(location);
}

void ServerSettings::add_regex_match_urls(location_t &location) {
    this->regex_match_urls.push_back(location);
}

void ServerSettings::add_prefix_match_urls(location_t &location) {
    this->prefix_match_urls.push_back(location);
}

int ServerSettings::get_port() {
    return this->port;
}

std::string ServerSettings::get_servername() {
    return this->servername;
}

std::string ServerSettings::get_access_log_filename() {
    return this->access_log_file;
}

std::string ServerSettings::get_error_log_filename() {
    return this->error_log_file;
}
