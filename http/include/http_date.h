#pragma once

#include <string>
#include <vector>

class Date {
 public:
    Date() = delete;
    Date(const Date &) = delete;
    ~Date() = delete;
    static std::string get_date();

 private:
    static std::string get_week_day(int week_day);
    static std::string add_leading_zero(int num);
    static std::string get_month(int month);

 private:
    static const std::vector<std::string> months_;
    static const std::vector<std::string> week_days_;
};
