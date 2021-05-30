#include <vector>
#include <string>

#include "upstream_settings.h"
#include <iostream>

UpstreamSettings::UpstreamSettings(std::string upstream_address, int weight, int port) {
    set_upstream_address(upstream_address);
    this->weight = weight;
    this->port = port;
}

std::string UpstreamSettings::get_upstream_address() {
    return this->upstream_address;
}

int UpstreamSettings::get_weight() {
    return this->weight;
}

void UpstreamSettings::set_upstream_address(std::string upstream_address) {
    this->upstream_address = upstream_address;
    int i = 0;
    this->is_ip = true;
    int res;

    for (int _ = 0; _ < 3; _++) {
        res = this->is_sub_ip_num(upstream_address.substr(i, upstream_address.length() - i));
        i += res;
        if (res == -1) {
            this->is_ip = false;
            return;
        }
    }
    int j = i;
    for (; i < upstream_address.length(); i++) {
        if (!isdigit(upstream_address[i])) {
            this->is_ip = false;
            return;
        }
    }

    int num = std::stoi(upstream_address.substr(j, i));
    if (num < 0 || num > 255) {
        this->is_ip = false;
    }
}

int UpstreamSettings::is_sub_ip_num(std::string sub_ip_num) {
    int i = 0;

    while (i < sub_ip_num.length() && isdigit(sub_ip_num[i])) {
        i++;
    }

    if (i == 0) {
        return -1;
    }

    int num = std::stoi(sub_ip_num.substr(0, i));
    if (num < 0 || num > 255) {
        return -1;
    }

    std::string ip_num = sub_ip_num.substr(0, i);
    if (i == sub_ip_num.length() || sub_ip_num[i] != '.') {
        return -1;
    }

    return i + 1;
}

void UpstreamSettings::set_weight(int weight) {
    this->weight = weight;
}

bool UpstreamSettings::is_ip_address() {
    return this->is_ip;
}

int UpstreamSettings::get_port() {
    return this->port;
}
