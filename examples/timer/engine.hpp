#pragma once

#include <string>

namespace lamina::examples::timer {

enum class State { entry, running, paused, finished };

struct Settings {
    int max_digits = 6;
    int digits_per_field = 2;
    double added_seconds = 60.0;
    int seconds_per_minute = 60;
    int minutes_per_hour = 60;
    const char* minus = "−";
};

struct EntryFields {
    int hours;
    int minutes;
    int seconds;
};

class Engine {
public:
    explicit Engine(Settings settings = {});

    bool type_digit(int digit);
    bool type_double_zero();
    bool backspace();
    void clear_entry();
    void set_entry_seconds(int seconds);
    const std::string& typed() const { return typed_; }
    std::string entry_digits() const;
    EntryFields entry_fields() const;
    int entry_seconds() const;

    bool start(double now);
    bool pause(double now);
    bool resume(double now);
    void toggle(double now);
    void reset();
    bool add_time(double now);

    State state(double now) const;
    double total_seconds() const { return total_; }
    double remaining_seconds(double now) const;
    double progress(double now) const;
    std::string readout(double now) const;

    const Settings& settings() const { return settings_; }

private:
    Settings settings_;
    std::string typed_;
    State state_ = State::entry;
    double total_ = 0.0;
    double end_ = 0.0;
    double paused_remaining_ = 0.0;
};

std::string format_seconds(long long seconds, const Settings& settings);
long long shown_seconds(double remaining);

}
