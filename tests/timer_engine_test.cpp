#include <doctest/doctest.h>

#include <string_view>

#include "timer/engine.hpp"

using namespace lamina::examples::timer;

namespace {

Engine typed(std::string_view digits) {
    Engine e;
    for (char c : digits) {
        e.type_digit(c - '0');
    }
    return e;
}

}

TEST_CASE("digits fill hh:mm:ss from the right and the entry shows which were typed") {
    Engine e = typed("130");
    CHECK(e.entry_digits() == "000130");
    CHECK(e.typed() == "130");
    const EntryFields f = e.entry_fields();
    CHECK(f.hours == 0);
    CHECK(f.minutes == 1);
    CHECK(f.seconds == 30);
    CHECK(e.entry_seconds() == 90);
}

TEST_CASE("leading zeros are ignored and the entry stops at six digits") {
    Engine e;
    CHECK_FALSE(e.type_digit(0));
    CHECK_FALSE(e.type_double_zero());
    CHECK(e.entry_digits() == "000000");
    e = typed("123456");
    CHECK_FALSE(e.type_digit(7));
    CHECK(e.entry_digits() == "123456");
    e = typed("12345");
    CHECK(e.type_double_zero());
    CHECK(e.entry_digits() == "123450");
    e = typed("1");
    CHECK(e.type_double_zero());
    CHECK(e.entry_digits() == "000100");
}

TEST_CASE("backspace removes the last digit typed") {
    Engine e = typed("125");
    CHECK(e.backspace());
    CHECK(e.entry_digits() == "000012");
    CHECK(e.backspace());
    CHECK(e.backspace());
    CHECK_FALSE(e.backspace());
    CHECK(e.entry_digits() == "000000");
}

TEST_CASE("presets replace the entry") {
    Engine e = typed("42");
    e.set_entry_seconds(60);
    CHECK(e.entry_digits() == "000100");
    CHECK(e.typed() == "100");
    e.set_entry_seconds(900);
    CHECK(e.typed() == "1500");
    e.set_entry_seconds(3600 + 2 * 60 + 3);
    CHECK(e.entry_digits() == "010203");
    e.set_entry_seconds(0);
    CHECK(e.typed().empty());
}

TEST_CASE("starting normalises the fields: 90 seconds shows 01:30 and 99:99:99 counts every field") {
    Engine e = typed("90");
    CHECK(e.start(0.0));
    CHECK(e.total_seconds() == 90.0);
    CHECK(e.readout(0.0) == "01:30");
    Engine big = typed("999999");
    CHECK(big.start(0.0));
    CHECK(big.total_seconds() == 99 * 3600 + 99 * 60 + 99);
    CHECK(big.readout(0.0) == "100:40:39");
    Engine none;
    CHECK_FALSE(none.start(0.0));
    CHECK(none.state(0.0) == State::entry);
}

TEST_CASE("the readout rounds remaining time up and overtime down, without a minus zero") {
    Engine e = typed("10");
    e.start(100.0);
    CHECK(e.readout(100.0) == "00:10");
    CHECK(e.readout(100.5) == "00:10");
    CHECK(e.readout(101.0) == "00:09");
    CHECK(e.readout(109.5) == "00:01");
    CHECK(e.state(109.99) == State::running);
    CHECK(e.readout(110.0) == "00:00");
    CHECK(e.state(110.0) == State::finished);
    CHECK(e.readout(110.5) == "00:00");
    CHECK(e.readout(111.0) == "−00:01");
    CHECK(e.readout(110.0 + 3725.0) == "−1:02:05");
}

TEST_CASE("progress falls continuously from 1 to 0") {
    Engine e = typed("10");
    CHECK(e.progress(0.0) == 1.0);
    e.start(0.0);
    CHECK(e.progress(0.0) == doctest::Approx(1.0));
    CHECK(e.progress(2.5) == doctest::Approx(0.75));
    CHECK(e.progress(10.0) == doctest::Approx(0.0));
    CHECK(e.progress(20.0) == doctest::Approx(0.0));
}

TEST_CASE("pause holds the remaining time and resume continues from it") {
    Engine e = typed("10");
    e.start(0.0);
    CHECK(e.pause(4.0));
    CHECK(e.state(50.0) == State::paused);
    CHECK(e.remaining_seconds(50.0) == doctest::Approx(6.0));
    CHECK_FALSE(e.pause(50.0));
    CHECK(e.resume(50.0));
    CHECK(e.remaining_seconds(51.0) == doctest::Approx(5.0));
    CHECK(e.state(56.0) == State::finished);
    CHECK_FALSE(e.resume(56.0));
}

TEST_CASE("toggle starts, pauses, resumes and stops a finished timer") {
    Engine e = typed("5");
    e.toggle(0.0);
    CHECK(e.state(0.0) == State::running);
    e.toggle(1.0);
    CHECK(e.state(1.0) == State::paused);
    e.toggle(2.0);
    CHECK(e.state(2.0) == State::running);
    CHECK(e.state(6.0) == State::finished);
    e.toggle(6.0);
    CHECK(e.state(6.0) == State::entry);
    CHECK(e.typed().empty());
}

TEST_CASE("+1:00 adds a minute to the remaining time and the total, and restarts a finished timer") {
    Engine e = typed("10");
    CHECK_FALSE(e.add_time(0.0));
    e.start(0.0);
    CHECK(e.add_time(5.0));
    CHECK(e.remaining_seconds(5.0) == doctest::Approx(65.0));
    CHECK(e.total_seconds() == 70.0);
    e.pause(10.0);
    CHECK(e.add_time(10.0));
    CHECK(e.remaining_seconds(10.0) == doctest::Approx(120.0));
    CHECK(e.total_seconds() == 130.0);

    Engine f = typed("3");
    f.start(0.0);
    CHECK(f.state(7.0) == State::finished);
    CHECK(f.add_time(7.0));
    CHECK(f.state(7.0) == State::running);
    CHECK(f.readout(7.0) == "01:00");
    CHECK(f.progress(7.0) == doctest::Approx(1.0));
}

TEST_CASE("reset returns to an empty entry from any state") {
    Engine e = typed("10");
    e.start(0.0);
    e.reset();
    CHECK(e.state(0.0) == State::entry);
    CHECK(e.entry_digits() == "000000");
    CHECK(e.type_digit(3));
    CHECK(e.entry_seconds() == 3);
}

TEST_CASE("digits are ignored while the timer runs") {
    Engine e = typed("10");
    e.start(0.0);
    CHECK_FALSE(e.type_digit(5));
    CHECK_FALSE(e.backspace());
    e.set_entry_seconds(60);
    CHECK(e.typed() == "10");
}
