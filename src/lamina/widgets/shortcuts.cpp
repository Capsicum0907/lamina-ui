#include "lamina/widgets/shortcuts.hpp"

#include <algorithm>
#include <stdexcept>

namespace lamina::widgets {

namespace {

std::string key_name(input::Key key, const tokens::ShortcutFormat& format) {
    for (const tokens::ShortcutKeyName& n : format.names) {
        if (n.key == key) {
            return std::string(n.name);
        }
    }
    const std::optional<char32_t> c = input::character_for_key(key);
    if (!c) {
        throw std::invalid_argument("the shortcut format has no name for this key");
    }
    const char32_t upper = *c >= U'a' && *c <= U'z' ? *c - U'a' + U'A' : *c;
    return std::string(1, static_cast<char>(upper));
}

std::string_view modifier_name(tokens::ShortcutModifier m, const KeyChord& chord, const tokens::ShortcutFormat& format) {
    switch (m) {
    case tokens::ShortcutModifier::ctrl: return chord.ctrl ? format.ctrl : std::string_view{};
    case tokens::ShortcutModifier::alt: return chord.alt ? format.alt : std::string_view{};
    case tokens::ShortcutModifier::shift: return chord.shift ? format.shift : std::string_view{};
    }
    return {};
}

}

std::string format_chord(const KeyChord& chord, const tokens::ShortcutFormat& format) {
    std::string out;
    for (tokens::ShortcutModifier m : format.order) {
        const std::string_view name = modifier_name(m, chord, format);
        if (!name.empty()) {
            out += name;
            out += format.separator;
        }
    }
    return out + key_name(chord.key, format);
}

const ShortcutRegistry::Command* ShortcutRegistry::find(std::string_view id) const {
    const auto it = std::find_if(commands_.begin(), commands_.end(), [id](const Command& c) { return c.id == id; });
    return it == commands_.end() ? nullptr : &*it;
}

ShortcutRegistry::Command& ShortcutRegistry::at(std::string_view id) {
    const auto it = std::find_if(commands_.begin(), commands_.end(), [id](const Command& c) { return c.id == id; });
    if (it == commands_.end()) {
        throw std::out_of_range("no command named " + std::string(id));
    }
    return *it;
}

void ShortcutRegistry::add(std::string id, std::optional<KeyChord> chord, std::function<void()> action) {
    if (id.empty()) {
        throw std::invalid_argument("a command needs an id");
    }
    if (contains(id)) {
        throw std::invalid_argument("the command " + id + " is already registered");
    }
    if (chord && std::any_of(commands_.begin(), commands_.end(), [&](const Command& c) { return c.chord == chord; })) {
        throw std::invalid_argument("the shortcut " + format_chord(*chord) + " is already bound");
    }
    commands_.push_back({std::move(id), chord, std::move(action)});
}

std::optional<KeyChord> ShortcutRegistry::chord(std::string_view id) const {
    const Command* c = find(id);
    return c ? c->chord : std::nullopt;
}

std::string ShortcutRegistry::label(std::string_view id, const tokens::ShortcutFormat& format) const {
    const std::optional<KeyChord> c = chord(id);
    return c ? format_chord(*c, format) : std::string{};
}

bool ShortcutRegistry::enabled(std::string_view id) const {
    const Command* c = find(id);
    return c && c->enabled;
}

void ShortcutRegistry::set_enabled(std::string_view id, bool enabled) {
    at(id).enabled = enabled;
}

bool ShortcutRegistry::run(std::string_view id) {
    Command& c = at(id);
    if (!c.enabled) {
        return false;
    }
    if (c.action) {
        c.action();
    }
    return true;
}

std::optional<std::string> ShortcutRegistry::match(const input::KeyEvent& e) const {
    for (const Command& c : commands_) {
        if (c.chord && c.chord->matches(e)) {
            return c.id;
        }
    }
    return std::nullopt;
}

bool ShortcutRegistry::handle(const input::KeyEvent& e) {
    const std::optional<std::string> id = match(e);
    return id && run(*id);
}

}
