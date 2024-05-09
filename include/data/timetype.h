#pragma once

#include <iomanip>
#include <sstream>

/**
 * Struct for recording timestamps
 */
struct TimeType {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int subsecond;  /*< in nanoseconds */

    /**
     * Constructor
     */
    TimeType(const std::string& timestampStr) {
        std::istringstream iss(timestampStr);
        char delimiter;
        iss >> year >> delimiter >> month >> delimiter >> day >> hour >> delimiter >>
            minute >> delimiter >> second >> delimiter >> subsecond;
    }

    /**
     * Return string format
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << std::setfill('0');
        oss << std::setw(4) << year << '-';
        oss << std::setw(2) << month << '-';
        oss << std::setw(2) << day << ' ';
        oss << std::setw(2) << hour << ':';
        oss << std::setw(2) << minute << ':';
        oss << std::setw(2) << second << '.';
        oss << std::setw(9) << std::setprecision(9) << subsecond; // Display nanoseconds
        return oss.str();
    }

    /**
     * Return nanoseconds time
     */
    long long toNanosecondsSinceEpoch() const {
        // Calculate total seconds since epoch
        const int epochYear = 1970;
        long long totalSeconds = 0;
        for (int y = epochYear; y < year; ++y) {
            totalSeconds += isLeapYear(y) ? 366 * 24 * 60 * 60 : 365 * 24 * 60 * 60;
        }
        totalSeconds += calculateSecondsInYear();

        // Convert to nanoseconds
        long long totalNanoseconds = totalSeconds * 1'000'000'000 + subsecond;
        return totalNanoseconds;
    }

    /**
     * Helper function to calculate leap year
     */
    bool isLeapYear(int y) const {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    /**
     * Helper function to calculate seconds in the current year
     */
    long long calculateSecondsInYear() const {
        // Assuming all months have 30 days for simplicity
        const int daysInMonth = 30;
        long long totalDays = (month - 1) * daysInMonth + day - 1;
        return totalDays * 24 * 60 * 60 + hour * 60 * 60 + minute * 60 + second;
    }
};