#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lamina::examples::calculator {

enum class Operator { add, subtract, multiply, divide, power };

enum class Function : std::size_t { sin, cos, tan, asin, acos, atan, ln, log, sqrt, count };

enum class Constant : std::size_t { pi, e, count };

enum class AngleUnit { degrees, radians };

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
    power,
    equals,
    parentheses,
    open_parenthesis,
    close_parenthesis,
    percent,
    toggle_sign,
    backspace,
    clear,
    all_clear,
    sine,
    cosine,
    tangent,
    natural_log,
    common_log,
    square_root,
    factorial,
    reciprocal,
    pi,
    euler,
    inverse,
    angle_unit,
};

enum class Error { none, divide_by_zero, out_of_range, domain };

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
    double trig_zero = 1e-12;
    int max_factorial = 170;
    AngleUnit angle_unit = AngleUnit::degrees;
    NumberStyle number;
    std::string_view add = "+";
    std::string_view subtract = "−";
    std::string_view multiply = "×";
    std::string_view divide = "÷";
    std::string_view power = "^";
    std::string_view factorial = "!";
    std::string_view equals = "=";
    std::string_view open = "(";
    std::string_view close = ")";
    std::string_view separator = " ";
    std::array<std::string_view, static_cast<std::size_t>(Function::count)> functions{
        "sin", "cos", "tan", "asin", "acos", "atan", "ln", "log", "√"};
    std::array<std::string_view, static_cast<std::size_t>(Constant::count)> constants{"π", "e"};
    std::string_view degrees_label = "DEG";
    std::string_view radians_label = "RAD";
    std::string_view divide_by_zero_message = "Can't divide by 0";
    std::string_view out_of_range_message = "Out of range";
    std::string_view domain_message = "Domain error";
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
    bool inverse() const { return inverse_; }
    AngleUnit angle_unit() const { return angle_unit_; }
    std::string_view angle_label() const;

private:
    enum class Phase { input, result, error };
    enum class TokenKind { number, constant, op, open, function, close, factorial };

    struct Token {
        TokenKind kind;
        Operator op = Operator::add;
        std::string text = {};
        Function function = Function::sin;
        Constant constant = Constant::pi;
    };

    using Tokens = std::vector<Token>;

    struct Outcome {
        double value;
        Error error;
    };

    void digit(int d);
    void point();
    void apply_operator(Operator op);
    void parentheses();
    void open_parenthesis();
    void close_parenthesis();
    void equals();
    void percent();
    void toggle_sign();
    void backspace();
    void clear();
    void all_clear();
    void apply_function(Function f);
    void apply_constant(Constant c);
    void apply_postfix(const Tokens& suffix);
    void apply_exponential(Token base);

    void start_input();
    void start_term();
    void continue_from_result();
    int depth() const;
    bool ends_complete() const;
    bool ends_with_lone_minus() const;
    void push(Token token);
    void push(TokenKind kind, Operator op = Operator::add, std::string text = {});
    void begin_number(std::string text);
    Token number(double value) const;
    Tokens completed(const Tokens& tokens) const;
    std::optional<std::pair<Operator, Token>> last_operation(const Tokens& tokens) const;
    double round(double value) const;
    Outcome evaluate(const Tokens& tokens) const;
    std::string display(double value) const;
    std::string display_text(std::string_view raw) const;
    std::string_view symbol(Operator op) const;
    std::string join(const Tokens& tokens) const;
    std::string_view message(Error e) const;

    Settings settings_;
    Tokens tokens_;
    Phase phase_ = Phase::input;
    Error error_ = Error::none;
    double result_ = 0.0;
    std::string done_formula_;
    std::optional<std::pair<Operator, Token>> repeat_;
    bool inverse_ = false;
    AngleUnit angle_unit_;
};

}
