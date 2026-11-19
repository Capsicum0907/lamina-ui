#include <doctest/doctest.h>

#include <initializer_list>
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
    case 'p': return Command::parentheses;
    case '(': return Command::open_parenthesis;
    case ')': return Command::close_parenthesis;
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

TEST_CASE("the parentheses key opens a group, and closes it after a complete operand") {
    Engine e = run("p2+3");
    CHECK(e.formula() == "(2 + 3");
    CHECK(e.result() == "5");
    press(e, "p");
    CHECK(e.formula() == "(2 + 3)");
    press(e, "*4=");
    CHECK(e.formula() == "(2 + 3) × 4 =");
    CHECK(e.result() == "20");
    CHECK(run("2p3").formula() == "2 × (3");
    CHECK(run("2p3p").formula() == "2 × (3)");
    CHECK(run("2*pp").formula() == "2 × ((");
}

TEST_CASE("open groups are closed by equals, and a digit after a group multiplies") {
    Engine e = run("p2+3=");
    CHECK(e.formula() == "(2 + 3) =");
    CHECK(e.result() == "5");
    CHECK(run("(2+3)4=").result() == "20");
    CHECK(run("(2+3)4").formula() == "(2 + 3) × 4");
    CHECK(run("2)").formula() == "2");
    CHECK(run("(2+)").formula() == "(2 +");
    CHECK(run("2*(3+4)-1=").result() == "13");
    CHECK(run("((2))=").result() == "2");
}

TEST_CASE("repeated equals after a group repeats the group's value") {
    Engine e = run("2*(3+4)=");
    CHECK(e.result() == "14");
    press(e, "=");
    CHECK(e.formula() == "14 × 7 =");
    CHECK(e.result() == "98");
}

TEST_CASE("minus starts a negative number at the start, after an open group and after times or divide") {
    Engine e = run("-5*-2");
    CHECK(e.formula() == "−5 × −2");
    CHECK(press(e, "=").result() == "10");
    CHECK(run("8/-2=").result() == "−4");
    CHECK(run("(-3)=").result() == "−3");
    CHECK(run("5+-").formula() == "5 −");
    CHECK(run("-").formula() == "−");
    CHECK(run("-+").formula() == "−");
    CHECK(run("-=").formula() == "0 =");
    CHECK(run("-<").formula() == "0");
}

TEST_CASE("percent after + or - inside a group takes its share of the group's left side") {
    Engine e = run("100+(50+10%");
    CHECK(e.formula() == "100 + (50 + 5");
    CHECK(press(e, "=").result() == "155");
}

TEST_CASE("backspace removes parentheses like any other token") {
    CHECK(run("p2<").formula() == "(");
    CHECK(run("p2<<").formula() == "0");
    CHECK(run("p2p<").formula() == "(2");
}

namespace {

Engine& apply(Engine& e, std::initializer_list<Command> commands) {
    for (Command c : commands) {
        e.apply(c);
    }
    return e;
}

}

TEST_CASE("trigonometric functions work in degrees by default and in radians after the toggle") {
    Engine e;
    CHECK(e.angle_unit() == AngleUnit::degrees);
    CHECK(std::string(e.angle_label()) == "DEG");
    apply(e, {Command::sine});
    press(e, "30");
    CHECK(e.formula() == "sin(30");
    CHECK(e.result() == "0.5");
    press(e, "=");
    CHECK(e.formula() == "sin(30) =");
    CHECK(e.result() == "0.5");
    CHECK(press(apply(e, {Command::cosine}), "60=").result() == "0.5");
    CHECK(press(apply(e, {Command::tangent}), "45=").result() == "1");
    CHECK(press(apply(e, {Command::sine}), "180=").result() == "0");
    apply(e, {Command::angle_unit});
    CHECK(e.angle_unit() == AngleUnit::radians);
    CHECK(std::string(e.angle_label()) == "RAD");
    CHECK(apply(e, {Command::sine, Command::pi, Command::equals}).result() == "0");
    CHECK(press(apply(e, {Command::cosine}), "0=").result() == "1");
}

TEST_CASE("Inv turns the function keys into their inverses until it is pressed again") {
    Engine e;
    apply(e, {Command::inverse});
    CHECK(e.inverse());
    apply(e, {Command::sine});
    CHECK(press(e, ".5").formula() == "asin(0.5");
    CHECK(press(e, "=").result() == "30");
    CHECK(press(apply(e, {Command::cosine}), "0=").result() == "90");
    CHECK(press(apply(e, {Command::tangent}), "1=").result() == "45");
    CHECK(press(apply(e, {Command::natural_log}), "1=").result() == "2.71828182846");
    CHECK(apply(e, {Command::all_clear, Command::natural_log}).formula() == "e^(");
    CHECK(press(apply(e, {Command::all_clear, Command::common_log}), "3=").result() == "1,000");
    Engine s;
    press(s, "7");
    apply(s, {Command::inverse, Command::square_root});
    CHECK(s.formula() == "7^2");
    CHECK(press(s, "=").result() == "49");
    apply(e, {Command::inverse, Command::sine});
    CHECK_FALSE(e.inverse());
    CHECK(e.formula() == "sin(");
}

TEST_CASE("powers are right-associative and bind tighter than multiplication") {
    Engine e;
    press(e, "2");
    apply(e, {Command::power});
    press(e, "10");
    CHECK(e.formula() == "2^10");
    CHECK(press(e, "=").result() == "1,024");
    Engine r;
    press(r, "2");
    apply(r, {Command::power});
    press(r, "3");
    apply(r, {Command::power});
    press(r, "2=");
    CHECK(r.result() == "512");
    Engine m;
    press(m, "3*2");
    apply(m, {Command::power});
    press(m, "2=");
    CHECK(m.result() == "12");
    Engine n;
    press(n, "2");
    apply(n, {Command::power});
    press(n, "-1=");
    CHECK(n.formula() == "2^\u22121 =");
    CHECK(n.result() == "0.5");
}

TEST_CASE("factorial is postfix, and reciprocal and square follow a complete operand") {
    Engine e;
    press(e, "5");
    apply(e, {Command::factorial});
    CHECK(e.formula() == "5!");
    CHECK(e.result() == "120");
    CHECK(press(e, "=").result() == "120");
    Engine f;
    apply(f, {Command::factorial});
    CHECK(f.formula() == "0");
    Engine g;
    press(g, "3+2");
    apply(g, {Command::factorial});
    CHECK(press(g, "=").result() == "5");
    Engine r;
    press(r, "4");
    apply(r, {Command::reciprocal});
    CHECK(r.formula() == "4^\u22121");
    CHECK(press(r, "=").result() == "0.25");
}

TEST_CASE("square root, logarithms and the constants") {
    Engine e;
    apply(e, {Command::square_root});
    press(e, "2");
    CHECK(e.formula() == "\u221a(2");
    CHECK(press(e, "=").result() == "1.41421356237");
    CHECK(press(apply(e, {Command::natural_log, Command::euler}), "=").result() == "1");
    CHECK(press(apply(e, {Command::common_log}), "1000=").result() == "3");
    Engine p;
    press(p, "2");
    apply(p, {Command::pi});
    CHECK(p.formula() == "2 \u00d7 \u03c0");
    CHECK(p.result() == "6.28318530718");
    apply(p, {Command::sine});
    CHECK(p.formula() == "2 \u00d7 \u03c0 \u00d7 sin(");
    Engine q;
    apply(q, {Command::pi});
    press(q, "2");
    CHECK(q.formula() == "\u03c0 \u00d7 2");
    Engine n;
    press(n, "-");
    apply(n, {Command::pi});
    CHECK(n.formula() == "\u22121 \u00d7 \u03c0");
}

TEST_CASE("functions after a result start anew, postfix keys continue from it") {
    Engine e = run("2+2=");
    apply(e, {Command::square_root});
    CHECK(e.formula() == "\u221a(");
    Engine f = run("2+2=");
    apply(f, {Command::factorial});
    CHECK(f.formula() == "4!");
    CHECK(press(f, "=").result() == "24");
}

TEST_CASE("backspace removes a whole function name") {
    Engine e;
    apply(e, {Command::sine});
    press(e, "3<");
    CHECK(e.formula() == "sin(");
    press(e, "<");
    CHECK(e.formula() == "0");
}

TEST_CASE("values outside a function's domain are errors") {
    auto error_of = [](std::initializer_list<Command> before, std::string_view keys) {
        Engine e;
        apply(e, before);
        press(e, keys);
        return e.error();
    };
    CHECK(error_of({Command::square_root}, "-4=") == Error::domain);
    CHECK(error_of({Command::natural_log}, "0=") == Error::domain);
    CHECK(error_of({Command::common_log}, "-1=") == Error::domain);
    CHECK(error_of({Command::inverse, Command::sine}, "2=") == Error::domain);
    CHECK(error_of({Command::inverse, Command::cosine}, "-2=") == Error::domain);
    CHECK(error_of({Command::tangent}, "90=") == Error::domain);
    CHECK(error_of({Command::tangent}, "270=") == Error::domain);
    Engine e;
    apply(e, {Command::square_root});
    press(e, "-4=");
    CHECK(e.result() == "Domain error");
    Engine f;
    press(f, "2.5");
    apply(f, {Command::factorial, Command::equals});
    CHECK(f.error() == Error::domain);
    Engine g;
    press(g, "171");
    apply(g, {Command::factorial, Command::equals});
    CHECK(g.error() == Error::out_of_range);
    Engine h;
    press(h, "-8");
    apply(h, {Command::power});
    press(h, ".5=");
    CHECK(h.error() == Error::domain);
    Engine z;
    press(z, "0");
    apply(z, {Command::power});
    press(z, "-1=");
    CHECK(z.error() == Error::divide_by_zero);
}
