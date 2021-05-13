#include <ctime>

#include "http_date.h"

const std::vector<std::string>
        Date::months_ = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

const std::vector<std::string> Date::week_days_ = {"Unknown", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

Date::Date() {
    std::time_t t = std::time(nullptr);
    struct tm* tm = std::gmtime(&t);
    date_ = get_week_day(tm->tm_wday) + ", " + add_leading_zero(tm->tm_mday) + " " + get_month(tm->tm_mon) + " "
            + std::to_string(1900 + tm->tm_year) + " " + add_leading_zero(tm->tm_hour) + ":"
            + add_leading_zero(tm->tm_min) + ":" + add_leading_zero(tm->tm_sec) + " GMT";
}

std::string Date::get_week_day(int week_day) {
    return week_days_[week_day % 8];
}

std::string Date::add_leading_zero(int num) {
    if (num < 10) {
        return "0" + std::to_string(num);
    }
    return std::to_string(num);
}

std::string Date::get_month(int month) {
    return months_[month % 12];
}

std::string Date::get_date() const {
    return date_;
}
