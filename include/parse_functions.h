#pragma once

#include <string>
#include <vector>

#include "main_server_settings.h"


typedef enum {
    S_START,
    S_BRACE_OPEN,
    S_KEY,
    S_VALUE,
    S_SERVER_START,
    S_BRACE_CLOSE,
    S_LOCATION,
    S_COUNT,
    S_ERR,
    S_END
} state_t;

typedef enum {
    L_PROTOCOL,
    L_BRACE_OPEN,
    L_BRACE_CLOSE,
    L_NEW_LINE,
    L_KEY,
    L_VALUE,
    L_SERVER_START,
    L_LOCATION,
    L_END_LOCATION,
    L_SERVER_END,
    L_ERR,
    L_COUNT,

} lexem_t;

typedef enum {
    EXACT_MATCH,
    PREFERENTIAL_PREFIX,
    REGEX_MATCH,
    PREFIX_MATCH
} location_type_t;

std::string get_string_from_file(const std::string &filename);

void parse_config(MainServerSettings &main_server);

int get_lexem(std::string &config, int &pos, const std::vector<std::string> &valid_properties);
location_type_t get_prefix_status(std::string& config, int &pos);
bool get_url(std::string &config, int &pos, location_t &location);