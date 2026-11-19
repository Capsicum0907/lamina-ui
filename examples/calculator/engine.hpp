#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lamina::examples::calculator {

enum class Operator { add, subtract, multiply, divide };

enum class Command {
    digit_0,
    digit_1,
    digit_2,
    digit_3,
    digit_4,
    digit_5,
    digit_6,
    digit_7,
    digit_8,
    digit_9,
    point,
    add,
    subtract,
    multiply,
    divide,
    equals,
    percent,
    toggle_sign,
    backspace,
    clear,
    all_clear,
};

enum class Error { none, divide_by_zero, out_of_range };

enum class Focus { formula, result };

struct NumberStyle {
    std::string_view minus = "−";
    std::string_view point = ".";
    std::string_view group = ",";
    std::string_view exponent = "e";
    int group_size = 3;
};

struct Settings {
    int max_entry_digits = 12;
    int display_digits = 12;
    int working_digits = 15;
    int min_fixed_exponent = -6;
    NumberStyle number;
    std::string_view add = "+";
    std::string_view subtract = "−";
    std::string_view multiply = "×";
    std::string_view divide = "÷";
    std::string_view equals = "=";
    std::string_view separator = " ";
    std::string_view divide_by_zero_message = "Can't divide by 0";
    std::string_view out_of_range_message = "Out of range";
};

std::string format_number(double value, int significant_digits, int min_fixed_exponent, const NumberStyle& style);

class Engine {
public:
    explicit Engine(Settings settings = {});

    void apply(Command command);

    std::string formula() const;
    std::string result() const;
    Focus focus() const;
    Error error() const { return error_; }

private:
    enum class Phase { input, result, error };

    struct Term {
        double value;
        std::string text;
    };

    struct Outcome {
        double value;
        Error error;
    };

    void digit(int d);
    void point();
    void apply_operator(Operator op);
    void equals();
    void percent();
    void toggle_sign();
    void backspace();
    void clear();
    void all_clear();

    void start_input();
    Term entry_term() const;
    Term computed_term(double value) const;
    double round(double value) const;
    Outcome evaluate(const std::vector<Term>& terms, const std::vector<Operator>& ops) const;
    std::string display(double value) const;
    std::string display_text(std::string_view raw) const;
    std::string_view symbol(Operator op) const;
    std::string join(const std::vector<Term>& terms, const std::vector<Operator>& ops) const;
    std::string_view message(Error e) const;

    Settings settings_;
    std::vector<Term> terms_;
    std::vector<Operator> ops_;
    std::string entry_;
    Phase phase_ = Phase::input;
    Error error_ = Error::none;
    double result_ = 0.0;
    std::string done_formula_;
    std::optional<std::pair<Operator, Term>> repeat_;
};

}
