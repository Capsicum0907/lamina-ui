#include "calculator/engine.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
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

void Engine::digit(int d) {
    start_input();
    if (is_computed(entry_)) {
        entry_.clear();
    }
    const char c = static_cast<char>('0' + d);
    if (entry_ == "0") {
        entry_ = std::string(1, c);
    } else if (entry_ == "-0") {
        entry_ = std::string("-") + c;
    } else if (digit_count(entry_) < settings_.max_entry_digits) {
        entry_ += c;
    }
}

void Engine::point() {
    start_input();
    if (is_computed(entry_)) {
        entry_.clear();
    }
    if (entry_.empty()) {
        entry_ = "0.";
    } else if (entry_.find('.') == std::string::npos) {
        entry_ += '.';
    }
}

void Engine::apply_operator(Operator op) {
    if (phase_ == Phase::error) {
        return;
    }
    if (phase_ == Phase::result) {
        const Term previous = computed_term(result_);
        all_clear();
        terms_.push_back(previous);
        ops_.push_back(op);
        return;
    }
    if (!entry_.empty()) {
        terms_.push_back(entry_term());
        entry_.clear();
        ops_.push_back(op);
    } else if (!ops_.empty()) {
        ops_.back() = op;
    } else {
        terms_.push_back(computed_term(0.0));
        ops_.push_back(op);
    }
}

void Engine::equals() {
    if (phase_ == Phase::error) {
        return;
    }
    std::vector<Term> terms;
    std::vector<Operator> ops;
    if (phase_ == Phase::result) {
        if (!repeat_) {
            return;
        }
        terms = {computed_term(result_), repeat_->second};
        ops = {repeat_->first};
    } else {
        terms = terms_;
        ops = ops_;
        if (!entry_.empty()) {
            terms.push_back(entry_term());
        } else if (!ops.empty()) {
            ops.pop_back();
        }
        if (terms.empty()) {
            terms.push_back(computed_term(0.0));
        }
    }
    const Outcome outcome = evaluate(terms, ops);
    const std::optional<std::pair<Operator, Term>> repeat =
        ops.empty() ? std::nullopt : std::optional<std::pair<Operator, Term>>{{ops.back(), terms.back()}};
    std::string formula = join(terms, ops);
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
        entry_ = computed_term(value).text;
        return;
    }
    if (entry_.empty()) {
        return;
    }
    double x = parse(entry_);
    if (!ops_.empty() && (ops_.back() == Operator::add || ops_.back() == Operator::subtract)) {
        const std::vector<Operator> left(ops_.begin(), ops_.end() - 1);
        const Outcome base = evaluate(terms_, left);
        if (base.error != Error::none) {
            return;
        }
        x = round(base.value * x / percent_base);
    } else {
        x = round(x / percent_base);
    }
    entry_ = computed_term(x).text;
}

void Engine::toggle_sign() {
    if (phase_ == Phase::error) {
        return;
    }
    if (phase_ == Phase::result) {
        const double value = -result_;
        all_clear();
        entry_ = computed_term(value).text;
        return;
    }
    if (entry_.empty()) {
        entry_ = "-0";
    } else if (entry_.front() == '-') {
        entry_.erase(0, 1);
    } else {
        entry_.insert(0, 1, '-');
    }
}

void Engine::backspace() {
    if (phase_ == Phase::error) {
        all_clear();
        return;
    }
    if (phase_ == Phase::result) {
        return;
    }
    if (!entry_.empty()) {
        if (is_computed(entry_)) {
            entry_.clear();
        } else {
            entry_.pop_back();
        }
        if (entry_ == "-") {
            entry_.clear();
        }
        return;
    }
    if (!ops_.empty()) {
        ops_.pop_back();
        entry_ = terms_.back().text;
        terms_.pop_back();
    }
}

void Engine::clear() {
    if (phase_ == Phase::input && !entry_.empty()) {
        entry_.clear();
        return;
    }
    all_clear();
}

void Engine::all_clear() {
    terms_.clear();
    ops_.clear();
    entry_.clear();
    phase_ = Phase::input;
    error_ = Error::none;
    result_ = 0.0;
    done_formula_.clear();
    repeat_.reset();
}

Engine::Term Engine::entry_term() const {
    std::string text = entry_;
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return {parse(text), text};
}

Engine::Term Engine::computed_term(double value) const {
    return {value, format_number(value, settings_.display_digits, settings_.min_fixed_exponent, raw_style)};
}

double Engine::round(double value) const {
    if (!std::isfinite(value) || value == 0.0) {
        return value;
    }
    char buffer[number_buffer];
    const auto [end, ec] = std::to_chars(buffer, buffer + number_buffer, value, std::chars_format::scientific, settings_.working_digits - 1);
    if (ec != std::errc{}) {
        return value;
    }
    return parse(std::string_view(buffer, static_cast<std::size_t>(end - buffer)));
}

Engine::Outcome Engine::evaluate(const std::vector<Term>& terms, const std::vector<Operator>& ops) const {
    double sum = 0.0;
    double product = terms.empty() ? 0.0 : terms.front().value;
    for (std::size_t i = 0; i < ops.size() && i + 1 < terms.size(); ++i) {
        const double x = terms[i + 1].value;
        switch (ops[i]) {
        case Operator::multiply:
            product = round(product * x);
            break;
        case Operator::divide:
            if (x == 0.0) {
                return {0.0, Error::divide_by_zero};
            }
            product = round(product / x);
            break;
        case Operator::add:
            sum = round(sum + product);
            product = x;
            break;
        case Operator::subtract:
            sum = round(sum + product);
            product = -x;
            break;
        }
    }
    const double value = round(sum + product);
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

std::string Engine::join(const std::vector<Term>& terms, const std::vector<Operator>& ops) const {
    std::string out;
    for (std::size_t i = 0; i < terms.size(); ++i) {
        out += display_text(terms[i].text);
        if (i < ops.size()) {
            out += settings_.separator;
            out += symbol(ops[i]);
            if (i + 1 < terms.size()) {
                out += settings_.separator;
            }
        }
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
    std::string out = join(terms_, ops_);
    if (!entry_.empty()) {
        if (!out.empty()) {
            out += settings_.separator;
        }
        out += display_text(entry_);
    } else if (out.empty()) {
        out = display(0.0);
    }
    return out;
}

std::string Engine::result() const {
    switch (phase_) {
    case Phase::result: return display(result_);
    case Phase::error: return std::string(message(error_));
    case Phase::input: break;
    }
    if (ops_.empty()) {
        return {};
    }
    std::vector<Term> terms = terms_;
    std::vector<Operator> ops = ops_;
    if (!entry_.empty()) {
        terms.push_back(entry_term());
    } else {
        ops.pop_back();
    }
    if (ops.empty()) {
        return {};
    }
    const Outcome outcome = evaluate(terms, ops);
    return outcome.error == Error::none ? display(outcome.value) : std::string();
}

Focus Engine::focus() const {
    return phase_ == Phase::input ? Focus::formula : Focus::result;
}

}
