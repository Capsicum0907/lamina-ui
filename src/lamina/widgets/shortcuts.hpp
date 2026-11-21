#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/input/events.hpp"
#include "lamina/tokens/shortcut.hpp"

namespace lamina::widgets {

struct KeyChord {
    input::Key key;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    constexpr bool matches(const input::KeyEvent& e) const { return e.key == key && e.ctrl == ctrl && e.shift == shift && e.alt == alt; }
    constexpr bool operator==(const KeyChord&) const = default;
};

std::string format_chord(const KeyChord& chord, const tokens::ShortcutFormat& format = tokens::default_shortcut_format());

class ShortcutRegistry {
public:
    void add(std::string id, std::optional<KeyChord> chord, std::function<void()> action);
    bool contains(std::string_view id) const { return find(id) != nullptr; }
    std::optional<KeyChord> chord(std::string_view id) const;
    std::string label(std::string_view id, const tokens::ShortcutFormat& format = tokens::default_shortcut_format()) const;
    bool enabled(std::string_view id) const;
    void set_enabled(std::string_view id, bool enabled);
    bool run(std::string_view id);
    std::optional<std::string> match(const input::KeyEvent& event) const;
    bool handle(const input::KeyEvent& event);
    std::size_t size() const { return commands_.size(); }

private:
    struct Command {
        std::string id;
        std::optional<KeyChord> chord;
        std::function<void()> action;
        bool enabled = true;
    };

    const Command* find(std::string_view id) const;
    Command& at(std::string_view id);

    std::vector<Command> commands_;
};

}
