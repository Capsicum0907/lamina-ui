#include <doctest/doctest.h>

#include <string_view>

#include "calculator/engine.hpp"

using namespace lamina::examples::calculator;

namespace {

Command command_for(char c) {
    switch (c) {
    case '.': return Command::point;
    case '+': return Command::add;
    case '-': return Command::subtract;
    case '*': return Command::multiply;
    case '/': return Command::divide;
    case '=': return Command::equals;
    case '%': return Command::percent;
    case 'n': return Command::toggle_sign;
    case '<': return Command::backspace;
    case 'c': return Command::clear;
    case 'C': return Command::all_clear;
    default: return static_cast<Command>(static_cast<int>(Command::digit_0) + (c - '0'));
    }
}

Engine& press(Engine& e, std::string_view keys) {
    for (char c : keys) {
        e.apply(command_for(c));
    }
    return e;
}

Engine run(std::string_view keys) {
    Engine e;
    press(e, keys);
    return e;
}

std::string display(double v) {
    return format_number(v, Settings{}.display_digits, Settings{}.min_fixed_exponent, NumberStyle{});
}

}

TEST_CASE("a fresh calculator shows 0 and no result") {
    Engine e;
    CHECK(e.formula() == "0");
    CHECK(e.result() == "");
    CHECK(e.focus() == Focus::formula);
    CHECK(e.error() == Error::none);
}

TEST_CASE("digits build the entry, a leading zero is replaced and long entries stop at the digit limit") {
    CHECK(run("123").formula() == "123");
    CHECK(run("0007").formula() == "7");
    CHECK(run("1234567").formula() == "1,234,567");
    CHECK(run("1234567890123456").formula() == "123,456,789,012");
}

TEST_CASE("the decimal point starts with 0, appears once and keeps typed trailing zeros") {
    CHECK(run(".").formula() == "0.");
    CHECK(run(".5").formula() == "0.5");
    CHECK(run("1..5").formula() == "1.5");
    CHECK(run("1.50").formula() == "1.50");
    CHECK(run("1234.5").formula() == "1,234.5");
}

TEST_CASE("operators follow the usual precedence") {
    CHECK(run("12.5*8-3=").result() == "97");
    CHECK(run("2+3*4=").result() == "14");
    CHECK(run("2*3+4*5=").result() == "26");
    CHECK(run("8-6/3=").result() == "6");
    CHECK(run("2-3-4=").result() == "−5");
    CHECK(run("100/4/5=").result() == "5");
}

TEST_CASE("the formula shows the whole expression and the result line previews its value") {
    Engine e;
    press(e, "12.5*8");
    CHECK(e.formula() == "12.5 × 8");
    CHECK(e.result() == "100");
    press(e, "-");
    CHECK(e.formula() == "12.5 × 8 −");
    CHECK(e.result() == "100");
    press(e, "3");
    CHECK(e.result() == "97");
    CHECK(e.focus() == Focus::formula);
    press(e, "=");
    CHECK(e.formula() == "12.5 × 8 − 3 =");
    CHECK(e.result() == "97");
    CHECK(e.focus() == Focus::result);
}

TEST_CASE("a single number has no preview") {
    CHECK(run("5").result() == "");
    CHECK(run("5+").result() == "");
}

TEST_CASE("pressing another operator replaces the last one") {
    Engine e = run("2+*");
    CHECK(e.formula() == "2 ×");
    CHECK(press(e, "3=").result() == "6");
}

TEST_CASE("an operator first starts from 0") {
    CHECK(run("+5=").result() == "5");
    CHECK(run("-5=").result() == "−5");
}

TEST_CASE("equals with a trailing operator ignores it") {
    Engine e = run("2+=");
    CHECK(e.formula() == "2 =");
    CHECK(e.result() == "2");
}

TEST_CASE("equals on a fresh calculator gives 0") {
    Engine e = run("=");
    CHECK(e.formula() == "0 =");
    CHECK(e.result() == "0");
}

TEST_CASE("repeated equals repeats the last operator and operand") {
    Engine e = run("12.5*8-3=");
    press(e, "=");
    CHECK(e.formula() == "97 − 3 =");
    CHECK(e.result() == "94");
    press(e, "=");
    CHECK(e.result() == "91");
    CHECK(run("2+3*4==").result() == "56");
    CHECK(run("5==").result() == "5");
}

TEST_CASE("an operator after a result chains from it, a digit starts anew") {
    Engine e = run("2+3=");
    press(e, "*4");
    CHECK(e.formula() == "5 × 4");
    CHECK(press(e, "=").result() == "20");
    press(e, "7");
    CHECK(e.formula() == "7");
    CHECK(e.result() == "");
    CHECK(e.focus() == Focus::formula);
    CHECK(run("2+3=.").formula() == "0.");
}

TEST_CASE("percent divides by 100, and after + or - takes that share of the left side") {
    CHECK(run("50%").formula() == "0.5");
    Engine e = run("200+10%");
    CHECK(e.formula() == "200 + 20");
    CHECK(e.result() == "220");
    CHECK(run("200-10%=").result() == "180");
    CHECK(run("2*3+50%=").result() == "9");
    CHECK(run("50*10%=").result() == "5");
    CHECK(run("200+%").formula() == "200 +");
    Engine r = run("50+50=%");
    CHECK(r.formula() == "1");
    CHECK(r.focus() == Focus::formula);
}

TEST_CASE("the sign toggles on the entry, and on a result it makes a new entry") {
    CHECK(run("5n").formula() == "−5");
    CHECK(run("5nn").formula() == "5");
    CHECK(run("n").formula() == "−0");
    CHECK(run("n3").formula() == "−3");
    CHECK(run("4*n2=").result() == "−8");
    Engine e = run("2+3=n");
    CHECK(e.formula() == "−5");
    CHECK(e.focus() == Focus::formula);
    CHECK(run("2-2=n").formula() == "0");
}

TEST_CASE("backspace removes the last character and then steps back through the expression") {
    CHECK(run("123<").formula() == "12");
    CHECK(run("1.5<").formula() == "1.");
    CHECK(run("5n<").formula() == "0");
    Engine e = run("12+3<");
    CHECK(e.formula() == "12 +");
    press(e, "<");
    CHECK(e.formula() == "12");
    press(e, "<");
    CHECK(e.formula() == "1");
    press(e, "<<");
    CHECK(e.formula() == "0");
    Engine r = run("2+3=<");
    CHECK(r.result() == "5");
    CHECK(r.focus() == Focus::result);
}

TEST_CASE("clear removes the entry first and then everything; all clear removes everything") {
    Engine e = run("12+3c");
    CHECK(e.formula() == "12 +");
    press(e, "c");
    CHECK(e.formula() == "0");
    CHECK(run("12+3C").formula() == "0");
    Engine r = run("2+3=c");
    CHECK(r.formula() == "0");
    CHECK(r.result() == "");
}

TEST_CASE("dividing by zero is an error until a digit or a clear") {
    Engine e = run("8/0=");
    CHECK(e.error() == Error::divide_by_zero);
    CHECK(e.formula() == "8 ÷ 0 =");
    CHECK(e.result() == "Can't divide by 0");
    CHECK(e.focus() == Focus::result);
    press(e, "+=%n");
    CHECK(e.error() == Error::divide_by_zero);
    press(e, "5");
    CHECK(e.error() == Error::none);
    CHECK(e.formula() == "5");
    CHECK(run("0/0=").error() == Error::divide_by_zero);
    CHECK(run("8/0=c").error() == Error::none);
    CHECK(run("8/0=<").formula() == "0");
}

TEST_CASE("a division by zero only in the preview shows no preview") {
    Engine e = run("8/0");
    CHECK(e.result() == "");
    CHECK(e.error() == Error::none);
}

TEST_CASE("a result beyond the range of double is an error") {
    Engine e = run("999999999999*999999999999=");
    for (int i = 0; i < 40 && e.error() == Error::none; ++i) {
        press(e, "=");
    }
    CHECK(e.error() == Error::out_of_range);
    CHECK(e.result() == "Out of range");
}

TEST_CASE("results are rounded to the display digits and decimal noise does not show") {
    CHECK(run("1/3=").result() == "0.333333333333");
    CHECK(run("2/3=").result() == "0.666666666667");
    CHECK(run(".1+.2=").result() == "0.3");
    CHECK(run(".1+.2-.3=").result() == "0");
    CHECK(run("1.1*1.1=").result() == "1.21");
    CHECK(run("5n*0=").result() == "0");
}

TEST_CASE("long results switch to an exponent") {
    CHECK(run("123456789012*10=").result() == "1.23456789012e12");
    CHECK(run("999999999999+1=").result() == "1e12");
    CHECK(run("999999999999=").result() == "999,999,999,999");
    CHECK(run(".0000001*1=").result() == "1e−7");
    CHECK(run(".000001*1=").result() == "0.000001");
    CHECK(run("1n/3000000000=").result() == "−3.33333333333e−10");
}

TEST_CASE("a result in exponent form chains and can be replaced by typing") {
    Engine e = run("123456789012*10=*10");
    CHECK(e.formula() == "1.23456789012e12 × 10");
    CHECK(press(e, "=").result() == "1.23456789012e13");
    Engine p = run("123456789012*10=n");
    CHECK(p.formula() == "−1.23456789012e12");
    press(p, "4");
    CHECK(p.formula() == "4");
}

TEST_CASE("format_number groups the integer part and trims trailing zeros") {
    CHECK(display(0.0) == "0");
    CHECK(display(-0.0) == "0");
    CHECK(display(1234.5) == "1,234.5");
    CHECK(display(-1234567.25) == "−1,234,567.25");
    CHECK(display(100.0) == "100");
    CHECK(display(0.05) == "0.05");
    CHECK(display(1e15) == "1e15");
}
