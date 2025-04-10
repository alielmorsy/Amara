//
// Created by Ali Elmorsy on 4/7/2025.
//

#ifndef SCOPEDTIMER_H
#define SCOPEDTIMER_H

#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>



class ScopedTimer {
public:
    ScopedTimer(const std::string &label = __builtin_FUNCTION())
        : label_(label),
          start_time_(std::chrono::high_resolution_clock::now()) {
    }
     std::string GREEN_LIGHT = "\033[38;5;46m";
    const std::string GREEN_MEDIUM = "\033[38;5;76m";
    const std::string GREEN_DARK = "\033[38;5;34m";
    const std::string RED_LIGHT = "\033[38;5;196m";
    const std::string RED_MEDIUM = "\033[38;5;160m";
    const std::string RED_DARK = "\033[38;5;124m";
    const std::string RESET_COLOR = "\033[0m"; // Reset color

    std::string getColorForTime(double value, const std::string& unit) {
        if (unit == "µs") {
            if (value < 100) {
                return GREEN_LIGHT;
            } else if (value < 500) {
                return GREEN_MEDIUM;
            } else {
                return GREEN_DARK;
            }
        } else if (unit == "ms") {
            if (value < 10) {
                return RED_LIGHT;
            } else if (value < 50) {
                return RED_MEDIUM;
            } else {
                return RED_DARK;
            }
        } else { // seconds
            return RED_DARK;
        }
    }
    ~ScopedTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = end_time - start_time_;

        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();

        std::string color;
        std::string unit;
        double value;

        if (micros < 1000) {
            value = micros;
            unit = "µs";
        } else if (millis < 1000) {
            value = millis;
            unit = "ms";
        } else {
            value = seconds;
            unit = "s";
        }

        color = getColorForTime(value, unit);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << color << value << " " << unit << RESET_COLOR << " - " << label_ << std::endl;
    }

private:
    std::string label_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

#endif //SCOPEDTIMER_H