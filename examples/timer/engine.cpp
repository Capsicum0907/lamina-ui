#include "timer/engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace lamina::examples::timer {

Engine::Engine(Settings settings) : settings_(settings) {}

bool Engine::type_digit(int digit) {
    if (state_ != State::entry || digit < 0 || digit > 9 || static_cast<int>(typed_.size()) >= settings_.max_digits) {
        return false;
    }
    if (typed_.empty() && digit == 0) {
        return false;
    }
    typed_.push_back(static_cast<char>('0' + digit));
    return true;
}

bool Engine::type_double_zero() {
    const bool first = type_digit(0);
    const bool second = type_digit(0);
    return first || second;
}

bool Engine::backspace() {
    if (state_ != State::entry || typed_.empty()) {
        return false;
    }
    typed_.pop_back();
    return true;
}

void Engine::clear_entry() {
    typed_.clear();
}

void Engine::set_entry_seconds(int seconds) {
    if (state_ != State::entry) {
        return;
    }
    const int per_hour = settings_.seconds_per_minute * settings_.minutes_per_hour;
    const int hours = seconds / per_hour;
    const int minutes = seconds % per_hour / settings_.seconds_per_minute;
    const int rest = seconds % settings_.seconds_per_minute;
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%d%02d%02d", hours, minutes, rest);
    std::string digits = buffer;
    const auto first = digits.find_first_not_of('0');
    typed_ = first == std::string::npos ? std::string() : digits.substr(first);
    if (static_cast<int>(typed_.size()) > settings_.max_digits) {
        typed_ = typed_.substr(typed_.size() - static_cast<std::size_t>(settings_.max_digits));
    }
}

std::string Engine::entry_digits() const {
    return std::string(static_cast<std::size_t>(settings_.max_digits) - typed_.size(), '0') + typed_;
}

EntryFields Engine::entry_fields() const {
    const std::string d = entry_digits();
    const auto n = static_cast<std::size_t>(settings_.digits_per_field);
    const std::size_t end = d.size();
    auto field = [&](std::size_t from_end) { return std::stoi(d.substr(end - from_end - n, n)); };
    return {field(2 * n), field(n), field(0)};
}

int Engine::entry_seconds() const {
    const EntryFields f = entry_fields();
    return (f.hours * settings_.minutes_per_hour + f.minutes) * settings_.seconds_per_minute + f.seconds;
}

bool Engine::start(double now) {
    if (state_ != State::entry || entry_seconds() <= 0) {
        return false;
    }
    total_ = entry_seconds();
    end_ = now + total_;
    state_ = State::running;
    return true;
}

bool Engine::pause(double now) {
    if (state(now) != State::running) {
        return false;
    }
    paused_remaining_ = end_ - now;
    state_ = State::paused;
    return true;
}

bool Engine::resume(double now) {
    if (state_ != State::paused) {
        return false;
    }
    end_ = now + paused_remaining_;
    state_ = State::running;
    return true;
}

void Engine::toggle(double now) {
    switch (state(now)) {
    case State::entry: start(now); break;
    case State::running: pause(now); break;
    case State::paused: resume(now); break;
    case State::finished: reset(); break;
    }
}

void Engine::reset() {
    state_ = State::entry;
    typed_.clear();
    total_ = 0.0;
    end_ = 0.0;
    paused_remaining_ = 0.0;
}

bool Engine::add_time(double now) {
    switch (state(now)) {
    case State::entry: return false;
    case State::running:
        end_ += settings_.added_seconds;
        total_ += settings_.added_seconds;
        return true;
    case State::paused:
        paused_remaining_ += settings_.added_seconds;
        total_ += settings_.added_seconds;
        return true;
    case State::finished:
        end_ = now + settings_.added_seconds;
        total_ = settings_.added_seconds;
        return true;
    }
    return false;
}

State Engine::state(double now) const {
    if (state_ == State::running && now >= end_) {
        return State::finished;
    }
    return state_;
}

double Engine::remaining_seconds(double now) const {
    switch (state_) {
    case State::entry: return entry_seconds();
    case State::paused: return paused_remaining_;
    default: return end_ - now;
    }
}

double Engine::progress(double now) const {
    if (state_ == State::entry || total_ <= 0.0) {
        return 1.0;
    }
    return std::clamp(remaining_seconds(now) / total_, 0.0, 1.0);
}

std::string Engine::readout(double now) const {
    const long long shown = shown_seconds(remaining_seconds(now));
    if (shown < 0) {
        return settings_.minus + format_seconds(-shown, settings_);
    }
    return format_seconds(shown, settings_);
}

long long shown_seconds(double remaining) {
    if (remaining >= 0.0) {
        return static_cast<long long>(std::ceil(remaining));
    }
    return -static_cast<long long>(std::floor(-remaining));
}

std::string format_seconds(long long seconds, const Settings& settings) {
    const long long per_hour = static_cast<long long>(settings.seconds_per_minute) * settings.minutes_per_hour;
    const long long hours = seconds / per_hour;
    const long long minutes = seconds % per_hour / settings.seconds_per_minute;
    const long long rest = seconds % settings.seconds_per_minute;
    char buffer[32];
    if (hours > 0) {
        std::snprintf(buffer, sizeof buffer, "%lld:%02lld:%02lld", hours, minutes, rest);
    } else {
        std::snprintf(buffer, sizeof buffer, "%02lld:%02lld", minutes, rest);
    }
    return buffer;
}

}
