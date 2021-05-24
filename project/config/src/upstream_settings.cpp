#include <vector>
#include <string>

#include "upstream_settings.h"

UpstreamSettings::UpstreamSettings(std::string upstream_address, int weight) {
    this->upstream_address = upstream_address;
    this->weight = weight;
}

std::string UpstreamSettings::get_upstream_address() {
    return this->upstream_address;
}

int UpstreamSettings::get_weight() {
    return this->weight;
}

void UpstreamSettings::set_upstream_address(std::string upstream_address) {
    this->upstream_address = upstream_address;
}

void UpstreamSettings::set_weight(int weight) {
    this->weight = weight;
}