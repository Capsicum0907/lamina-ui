#pragma once

#include <array>
#include <string_view>

#include "lamina/input/events.hpp"

namespace lamina::tokens {

enum class ShortcutModifier { ctrl, alt, shift };

struct ShortcutKeyName {
    input::Key key;
    std::string_view name;
};

struct ShortcutFormat {
    std::string_view ctrl;
    std::string_view alt;
    std::string_view shift;
    std::string_view separator;
    std::array<ShortcutModifier, 3> order;
    std::array<ShortcutKeyName, 32> names;
};

inline constexpr ShortcutFormat pc_shortcut_format{
    .ctrl = "Ctrl",
    .alt = "Alt",
    .shift = "Shift",
    .separator = "+",
    .order = {ShortcutModifier::ctrl, ShortcutModifier::alt, ShortcutModifier::shift},
    .names = {{
        {input::Key::backspace, "Backspace"}, {input::Key::delete_forward, "Del"}, {input::Key::left, "Left"},
        {input::Key::right, "Right"},         {input::Key::up, "Up"},              {input::Key::down, "Down"},
        {input::Key::home, "Home"},           {input::Key::end, "End"},            {input::Key::enter, "Enter"},
        {input::Key::escape, "Esc"},          {input::Key::tab, "Tab"},            {input::Key::space, "Space"},
        {input::Key::page_up, "PgUp"},        {input::Key::page_down, "PgDn"},     {input::Key::insert, "Ins"},
        {input::Key::plus, "Plus"},           {input::Key::minus, "Minus"},        {input::Key::f1, "F1"},
        {input::Key::f2, "F2"},               {input::Key::f3, "F3"},              {input::Key::f4, "F4"},
        {input::Key::f5, "F5"},               {input::Key::f6, "F6"},              {input::Key::f7, "F7"},
        {input::Key::f8, "F8"},               {input::Key::f9, "F9"},              {input::Key::f10, "F10"},
        {input::Key::f11, "F11"},             {input::Key::f12, "F12"},            {input::Key::menu, "Menu"},
        {input::Key::asterisk, "*"},          {input::Key::slash, "/"},
    }},
};

inline constexpr ShortcutFormat mac_shortcut_format{
    .ctrl = "⌃",
    .alt = "⌥",
    .shift = "⇧",
    .separator = "",
    .order = {ShortcutModifier::ctrl, ShortcutModifier::alt, ShortcutModifier::shift},
    .names = {{
        {input::Key::backspace, "⌫"}, {input::Key::delete_forward, "⌦"}, {input::Key::left, "←"},
        {input::Key::right, "→"},     {input::Key::up, "↑"},             {input::Key::down, "↓"},
        {input::Key::home, "↖"},      {input::Key::end, "↘"},            {input::Key::enter, "↩"},
        {input::Key::escape, "⎋"},    {input::Key::tab, "⇥"},            {input::Key::space, "Space"},
        {input::Key::page_up, "⇞"},   {input::Key::page_down, "⇟"},      {input::Key::insert, "Ins"},
        {input::Key::plus, "+"},           {input::Key::minus, "-"},               {input::Key::f1, "F1"},
        {input::Key::f2, "F2"},            {input::Key::f3, "F3"},                 {input::Key::f4, "F4"},
        {input::Key::f5, "F5"},            {input::Key::f6, "F6"},                 {input::Key::f7, "F7"},
        {input::Key::f8, "F8"},            {input::Key::f9, "F9"},                 {input::Key::f10, "F10"},
        {input::Key::f11, "F11"},          {input::Key::f12, "F12"},               {input::Key::menu, "Menu"},
        {input::Key::asterisk, "*"},       {input::Key::slash, "/"},
    }},
};

constexpr const ShortcutFormat& default_shortcut_format() {
#ifdef __APPLE__
    return mac_shortcut_format;
#else
    return pc_shortcut_format;
#endif
}

}
