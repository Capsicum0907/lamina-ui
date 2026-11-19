#include "calculator/engine.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <system_error>

namespace lamina::examples::calculator {

namespace {

constexpr NumberStyle raw_style{"-", ".", "", "e", 3};
constexpr std::size_t number_buffer = 64;
constexpr double percent_base = 100.0;

bool is_computed(std::string_view raw) {
    return raw.find('e') != std::string_view::npos;
}

int digit_count(std::string_view raw) {
    return static_cast<int>(std::count_if(raw.begin(), raw.end(), [](char c) { return c >= '0' && c <= '9'; }));
}

double parse(std::string_view raw) {
    double v = 0.0;
    std::from_chars(raw.data(), raw.data() + raw.size(), v);
    return v;
}

double round_to(double value, int digits) {
    if (!std::isfinite(value) || value == 0.0) {
        return value;
    }
    char buffer[number_buffer];
    const auto [end, ec] = std::to_chars(buffer, buffer + number_buffer, value, std::chars_format::scientific, digits - 1);
    if (ec != std::errc{}) {
        return value;
    }
    return parse(std::string_view(buffer, static_cast<std::size_t>(end - buffer)));
}

enum class ItemKind { number, op, open, close };

struct Item {
    ItemKind kind;
    Operator op;
    double value;
};

class Parser {
public:
    Parser(const std::vector<Item>& items, int digits) : items_(items), digits_(digits) {}

    Error error() const { return error_; }

    double expression() {
        double left = term();
        while (at(Operator::add) || at(Operator::subtract)) {
            const Operator op = items_[i_++].op;
            const double right = term();
            left = round_to(op == Operator::add ? left + right : left - right, digits_);
        }
        return left;
    }

private:
    bool at(Operator op) const { return i_ < items_.size() && items_[i_].kind == ItemKind::op && items_[i_].op == op; }

    double term() {
        double left = factor();
        while (at(Operator::multiply) || at(Operator::divide)) {
            const Operator op = items_[i_++].op;
            const double right = factor();
            if (op == Operator::divide && right == 0.0) {
                error_ = Error::divide_by_zero;
                return 0.0;
            }
            left = round_to(op == Operator::multiply ? left * right : left / right, digits_);
        }
        return left;
    }

    double factor() {
        if (i_ >= items_.size()) {
            return 0.0;
        }
        const Item& item = items_[i_++];
        if (item.kind != ItemKind::open) {
            return item.value;
        }
        const double v = expression();
        if (i_ < items_.size() && items_[i_].kind == ItemKind::close) {
            ++i_;
        }
        return v;
    }

    const std::vector<Item>& items_;
    int digits_;
    std::size_t i_ = 0;
    Error error_ = Error::none;
};

std::string grouped(std::string_view digits, const NumberStyle& style) {
    if (style.group.empty() || style.group_size <= 0) {
        return std::string(digits);
    }
    std::string out;
    const auto n = static_cast<int>(digits.size());
    for (int i = 0; i < n; ++i) {
        if (i > 0 && (n - i) % style.group_size == 0) {
            out += style.group;
        }
        out += digits[static_cast<std::size_t>(i)];
    }
    return out;
}

std::string exponent_text(int exponent, const NumberStyle& style) {
    std::string out(style.exponent);
    if (exponent < 0) {
        out += style.minus;
    }
    out += std::to_string(std::abs(exponent));
    return out;
}

}

std::string format_number(double value, int significant_digits, int min_fixed_exponent, const NumberStyle& style) {
    if (value == 0.0) {
        return "0";
    }
    char buffer[number_buffer];
    const auto [end, ec] = std::to_chars(buffer, buffer + number_buffer, std::fabs(value), std::chars_format::scientific,
                                         significant_digits - 1);
    if (ec != std::errc{}) {
        return {};
    }
    const std::string_view text(buffer, static_cast<std::size_t>(end - buffer));
    const std::size_t e = text.find('e');
    std::string digits;
    for (char c : text.substr(0, e)) {
        if (c != '.') {
            digits += c;
        }
    }
    int exponent = 0;
    std::from_chars(text.data() + e + 1 + (text[e + 1] == '+' ? 1 : 0), text.data() + text.size(), exponent);
    while (digits.size() > 1 && digits.back() == '0') {
        digits.pop_back();
    }

    std::string out = value < 0.0 ? std::string(style.minus) : std::string();
    if (exponent >= significant_digits || exponent < min_fixed_exponent) {
        out += digits[0];
        if (digits.size() > 1) {
            out += style.point;
            out += digits.substr(1);
        }
        out += exponent_text(exponent, style);
    } else if (exponent >= 0) {
        const auto whole = static_cast<std::size_t>(exponent) + 1;
        std::string integer = digits.substr(0, std::min(whole, digits.size()));
        integer.resize(whole, '0');
        out += grouped(integer, style);
        if (digits.size() > whole) {
            out += style.point;
            out += digits.substr(whole);
        }
    } else {
        out += "0";
        out += style.point;
        out += std::string(static_cast<std::size_t>(-exponent - 1), '0');
        out += digits;
    }
    return out;
}

Engine::Engine(Settings settings) : settings_(settings) {}

void Engine::apply(Command c) {
    switch (c) {
    case Command::point: point(); return;
    case Command::add: apply_operator(Operator::add); return;
    case Command::subtract: apply_operator(Operator::subtract); return;
    case Command::multiply: apply_operator(Operator::multiply); return;
    case Command::divide: apply_operator(Operator::divide); return;
    case Command::equals: equals(); return;
    case Command::parentheses: parentheses(); return;
    case Command::open_parenthesis: open_parenthesis(); return;
    case Command::close_parenthesis: close_parenthesis(); return;
    case Command::percent: percent(); return;
    case Command::toggle_sign: toggle_sign(); return;
    case Command::backspace: backspace(); return;
    case Command::clear: clear(); return;
    case Command::all_clear: all_clear(); return;
    default: digit(static_cast<int>(c) - static_cast<int>(Command::digit_0)); return;
    }
}

void Engine::start_input() {
    if (phase_ != Phase::input) {
        all_clear();
    }
}

void Engine::continue_from_result() {
    if (phase_ == Phase::result) {
        const Token previous = number(result_);
        all_clear();
        tokens_.push_back(previous);
    } else if (phase_ == Phase::error) {
        all_clear();
    }
}

int Engine::depth() const {
    int d = 0;
    for (const Token& t : tokens_) {
        d += t.kind == TokenKind::open ? 1 : t.kind == TokenKind::close ? -1 : 0;
    }
    return d;
}

bool Engine::ends_complete() const {
    if (tokens_.empty()) {
        return false;
    }
    const Token& back = tokens_.back();
    return back.kind == TokenKind::close || (back.kind == TokenKind::number && back.text != "-");
}

bool Engine::ends_with_lone_minus() const {
    return !tokens_.empty() && tokens_.back().kind == TokenKind::number && tokens_.back().text == "-";
}

void Engine::push(TokenKind kind, Operator op, std::string text) {
    tokens_.push_back({kind, op, std::move(text)});
}

void Engine::begin_number(std::string text) {
    if (!tokens_.empty() && tokens_.back().kind == TokenKind::close) {
        push(TokenKind::op, Operator::multiply);
    }
    push(TokenKind::number, Operator::add, std::move(text));
}

void Engine::digit(int d) {
    start_input();
    const std::string typed(1, static_cast<char>('0' + d));
    if (tokens_.empty() || tokens_.back().kind != TokenKind::number) {
        begin_number(typed);
        return;
    }
    std::string& text = tokens_.back().text;
    if (is_computed(text) || text == "0") {
        text = typed;
    } else if (text == "-0") {
        text = "-" + typed;
    } else if (digit_count(text) < settings_.max_entry_digits) {
        text += typed;
    }
}

void Engine::point() {
    start_input();
    if (tokens_.empty() || tokens_.back().kind != TokenKind::number) {
        begin_number("0.");
        return;
    }
    std::string& text = tokens_.back().text;
    if (is_computed(text)) {
        text = "0.";
    } else if (text == "-") {
        text = "-0.";
    } else if (text.find('.') == std::string::npos) {
        text += '.';
    }
}

void Engine::apply_operator(Operator op) {
    if (phase_ == Phase::error) {
        return;
    }
    continue_from_result();
    if (tokens_.empty()) {
        if (op == Operator::subtract) {
            push(TokenKind::number, Operator::add, "-");
        } else {
            tokens_.push_back(number(0.0));
            push(TokenKind::op, op);
        }
        return;
    }
    Token& back = tokens_.back();
    switch (back.kind) {
    case TokenKind::number:
        if (back.text != "-") {
            push(TokenKind::op, op);
        }
        return;
    case TokenKind::close:
        push(TokenKind::op, op);
        return;
    case TokenKind::op:
        if (op == Operator::subtract && (back.op == Operator::multiply || back.op == Operator::divide)) {
            push(TokenKind::number, Operator::add, "-");
        } else {
            back.op = op;
        }
        return;
    case TokenKind::open:
        if (op == Operator::subtract) {
            push(TokenKind::number, Operator::add, "-");
        }
        return;
    }
}

void Engine::parentheses() {
    if (phase_ == Phase::input && depth() > 0 && ends_complete()) {
        close_parenthesis();
        return;
    }
    open_parenthesis();
}

void Engine::open_parenthesis() {
    continue_from_result();
    if (ends_with_lone_minus()) {
        return;
    }
    if (ends_complete()) {
        push(TokenKind::op, Operator::multiply);
    }
    push(TokenKind::open);
}

void Engine::close_parenthesis() {
    if (phase_ == Phase::input && ends_complete() && depth() > 0) {
        push(TokenKind::close);
    }
}

Engine::Tokens Engine::completed(const Tokens& tokens) const {
    Tokens out = tokens;
    while (!out.empty() && (out.back().kind == TokenKind::op || out.back().kind == TokenKind::open ||
                            (out.back().kind == TokenKind::number && out.back().text == "-"))) {
        out.pop_back();
    }
    int open = 0;
    for (Token& t : out) {
        open += t.kind == TokenKind::open ? 1 : t.kind == TokenKind::close ? -1 : 0;
        if (t.kind == TokenKind::number && !t.text.empty() && t.text.back() == '.') {
            t.text.pop_back();
        }
    }
    for (; open > 0; --open) {
        out.push_back({TokenKind::close, Operator::add, {}});
    }
    return out;
}

std::optional<std::pair<Operator, Engine::Token>> Engine::last_operation(const Tokens& tokens) const {
    std::optional<std::pair<Operator, Token>> out;
    int level = 0;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        level += t.kind == TokenKind::open ? 1 : t.kind == TokenKind::close ? -1 : 0;
        if (t.kind != TokenKind::op || level != 0) {
            continue;
        }
        const Tokens operand(tokens.begin() + static_cast<std::ptrdiff_t>(i) + 1, tokens.end());
        out = std::pair{t.op, operand.size() == 1 ? operand.front() : number(evaluate(operand).value)};
    }
    return out;
}

void Engine::equals() {
    if (phase_ == Phase::error) {
        return;
    }
    Tokens tokens;
    if (phase_ == Phase::result) {
        if (!repeat_) {
            return;
        }
        tokens = {number(result_), {TokenKind::op, repeat_->first, {}}, repeat_->second};
    } else {
        tokens = completed(tokens_);
        if (tokens.empty()) {
            tokens.push_back(number(0.0));
        }
    }
    const Outcome outcome = evaluate(tokens);
    const auto repeat = last_operation(tokens);
    std::string formula = join(tokens);
    formula += settings_.separator;
    formula += settings_.equals;
    all_clear();
    done_formula_ = std::move(formula);
    if (outcome.error != Error::none) {
        phase_ = Phase::error;
        error_ = outcome.error;
        return;
    }
    phase_ = Phase::result;
    result_ = outcome.value;
    repeat_ = repeat;
}

void Engine::percent() {
    if (phase_ == Phase::error) {
        return;
    }
    if (phase_ == Phase::result) {
        const double value = round(result_ / percent_base);
        all_clear();
        tokens_.push_back(number(value));
        return;
    }
    if (!ends_complete() || tokens_.back().kind != TokenKind::number) {
        return;
    }
    const std::size_t last = tokens_.size() - 1;
    double x = parse(tokens_[last].text);
    const bool additive = last > 0 && tokens_[last - 1].kind == TokenKind::op &&
                          (tokens_[last - 1].op == Operator::add || tokens_[last - 1].op == Operator::subtract);
    if (additive) {
        std::size_t start = last - 1;
        int level = 0;
        while (start > 0) {
            const Token& t = tokens_[start - 1];
            if (t.kind == TokenKind::open && level == 0) {
                break;
            }
            level += t.kind == TokenKind::close ? 1 : t.kind == TokenKind::open ? -1 : 0;
            --start;
        }
        const Outcome base = evaluate(Tokens(tokens_.begin() + static_cast<std::ptrdiff_t>(start),
                                             tokens_.begin() + static_cast<std::ptrdiff_t>(last - 1)));
        if (base.error != Error::none) {
            return;
        }
        x = round(base.value * x / percent_base);
    } else {
        x = round(x / percent_base);
    }
    tokens_[last] = number(x);
}

void Engine::toggle_sign() {
    if (phase_ == Phase::error) {
        return;
    }
    if (phase_ == Phase::result) {
        const double value = -result_;
        all_clear();
        tokens_.push_back(number(value));
        return;
    }
    if (tokens_.empty() || tokens_.back().kind == TokenKind::op || tokens_.back().kind == TokenKind::open) {
        push(TokenKind::number, Operator::add, "-0");
        return;
    }
    Token& back = tokens_.back();
    if (back.kind != TokenKind::number) {
        return;
    }
    if (back.text == "-") {
        tokens_.pop_back();
    } else if (back.text.front() == '-') {
        back.text.erase(0, 1);
    } else {
        back.text.insert(0, 1, '-');
    }
}

void Engine::backspace() {
    if (phase_ == Phase::error) {
        all_clear();
        return;
    }
    if (phase_ == Phase::result || tokens_.empty()) {
        return;
    }
    Token& back = tokens_.back();
    if (back.kind == TokenKind::number && !is_computed(back.text)) {
        back.text.pop_back();
        if (!back.text.empty() && back.text != "-") {
            return;
        }
    }
    tokens_.pop_back();
}

void Engine::clear() {
    if (phase_ == Phase::input && !tokens_.empty() && tokens_.back().kind == TokenKind::number) {
        tokens_.pop_back();
        return;
    }
    all_clear();
}

void Engine::all_clear() {
    tokens_.clear();
    phase_ = Phase::input;
    error_ = Error::none;
    result_ = 0.0;
    done_formula_.clear();
    repeat_.reset();
}

Engine::Token Engine::number(double value) const {
    return {TokenKind::number, Operator::add, format_number(value, settings_.display_digits, settings_.min_fixed_exponent, raw_style)};
}

double Engine::round(double value) const {
    return round_to(value, settings_.working_digits);
}

Engine::Outcome Engine::evaluate(const Tokens& tokens) const {
    std::vector<Item> items;
    items.reserve(tokens.size());
    for (const Token& t : tokens) {
        switch (t.kind) {
        case TokenKind::number: items.push_back({ItemKind::number, t.op, parse(t.text)}); break;
        case TokenKind::op: items.push_back({ItemKind::op, t.op, 0.0}); break;
        case TokenKind::open: items.push_back({ItemKind::open, t.op, 0.0}); break;
        case TokenKind::close: items.push_back({ItemKind::close, t.op, 0.0}); break;
        }
    }
    Parser parser(items, settings_.working_digits);
    const double value = parser.expression();
    if (parser.error() != Error::none) {
        return {0.0, parser.error()};
    }
    if (!std::isfinite(value)) {
        return {0.0, Error::out_of_range};
    }
    return {value == 0.0 ? 0.0 : value, Error::none};
}

std::string Engine::display(double value) const {
    return format_number(value, settings_.display_digits, settings_.min_fixed_exponent, settings_.number);
}

std::string Engine::display_text(std::string_view raw) const {
    const NumberStyle& style = settings_.number;
    std::string out;
    if (!raw.empty() && raw.front() == '-') {
        out += style.minus;
        raw.remove_prefix(1);
    }
    const std::size_t e = raw.find('e');
    const std::string_view mantissa = raw.substr(0, e);
    const std::size_t dot = mantissa.find('.');
    out += grouped(mantissa.substr(0, dot), style);
    if (dot != std::string_view::npos) {
        out += style.point;
        out += mantissa.substr(dot + 1);
    }
    if (e != std::string_view::npos) {
        int exponent = 0;
        std::from_chars(raw.data() + e + 1, raw.data() + raw.size(), exponent);
        out += exponent_text(exponent, style);
    }
    return out;
}

std::string_view Engine::symbol(Operator op) const {
    switch (op) {
    case Operator::add: return settings_.add;
    case Operator::subtract: return settings_.subtract;
    case Operator::multiply: return settings_.multiply;
    case Operator::divide: return settings_.divide;
    }
    return {};
}

std::string Engine::join(const Tokens& tokens) const {
    std::string out;
    for (const Token& t : tokens) {
        switch (t.kind) {
        case TokenKind::number: out += display_text(t.text); break;
        case TokenKind::op:
            out += settings_.separator;
            out += symbol(t.op);
            out += settings_.separator;
            break;
        case TokenKind::open: out += settings_.open; break;
        case TokenKind::close: out += settings_.close; break;
        }
    }
    while (!settings_.separator.empty() && out.ends_with(settings_.separator)) {
        out.resize(out.size() - settings_.separator.size());
    }
    return out;
}

std::string_view Engine::message(Error e) const {
    switch (e) {
    case Error::divide_by_zero: return settings_.divide_by_zero_message;
    case Error::out_of_range: return settings_.out_of_range_message;
    case Error::none: break;
    }
    return {};
}

std::string Engine::formula() const {
    if (phase_ != Phase::input) {
        return done_formula_;
    }
    return tokens_.empty() ? display(0.0) : join(tokens_);
}

std::string Engine::result() const {
    switch (phase_) {
    case Phase::result: return display(result_);
    case Phase::error: return std::string(message(error_));
    case Phase::input: break;
    }
    const Tokens tokens = completed(tokens_);
    if (std::none_of(tokens.begin(), tokens.end(), [](const Token& t) { return t.kind == TokenKind::op; })) {
        return {};
    }
    const Outcome outcome = evaluate(tokens);
    return outcome.error == Error::none ? display(outcome.value) : std::string();
}

Focus Engine::focus() const {
    return phase_ == Phase::input ? Focus::formula : Focus::result;
}

}
