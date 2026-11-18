#include "lamina/color/scheme.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>

#include "cpp/cam/cam.h"
#include "cpp/cam/hct.h"
#include "cpp/cam/viewing_conditions.h"
#include "cpp/contrast/contrast.h"
#include "cpp/dynamiccolor/dynamic_color.h"
#include "cpp/dynamiccolor/dynamic_scheme.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"
#include "cpp/utils/utils.h"
#include "lamina/color/mcu_scheme.hpp"

namespace lamina::color {

namespace {

namespace mcu = material_color_utilities;

struct HueRange {
    double from;
    double to;

    constexpr bool contains(double hue) const { return hue >= from && hue < to; }
};

constexpr HueRange blue_hues{250.0, 270.0};
constexpr HueRange yellow_hues{105.0, 125.0};
constexpr HueRange cyan_hues{170.0, 207.0};

constexpr double tone_black = 0.0;
constexpr double tone_white = 100.0;
constexpr double tone_search_step = 1.0;
constexpr double tone_without_background = 50.0;

struct YellowLightTone {
    double tone;
    double below;
    double above;
};

constexpr YellowLightTone yellow_light_tone{99.0, 98.0, 100.0};

struct BackgroundToneGap {
    double threshold;
    double light_min;
    double dark_max;
};

constexpr BackgroundToneGap background_tone_gap{57.0, 65.0, 49.0};

struct AvoidedToneBand {
    double from;
    double to;
    double dark_side;
    double light_side;
};

constexpr AvoidedToneBand avoided_tone_band_2021{50.0, 60.0, 49.0, 60.0};

struct Curve {
    double low;
    double normal;
    double medium;
    double high;
};

struct ContrastLevels {
    double reduced;
    double standard;
    double medium;
    double high;
};

constexpr ContrastLevels contrast_levels{-1.0, 0.0, 0.5, 1.0};

double curve_value(const Curve& c, double level) {
    if (level <= contrast_levels.reduced) {
        return c.low;
    }
    if (level < contrast_levels.standard) {
        return mcu::Lerp(c.low, c.normal, (level - contrast_levels.reduced) / (contrast_levels.standard - contrast_levels.reduced));
    }
    if (level < contrast_levels.medium) {
        return mcu::Lerp(c.normal, c.medium, (level - contrast_levels.standard) / (contrast_levels.medium - contrast_levels.standard));
    }
    if (level < contrast_levels.high) {
        return mcu::Lerp(c.medium, c.high, (level - contrast_levels.medium) / (contrast_levels.high - contrast_levels.medium));
    }
    return c.high;
}

constexpr Curve ratio_1_5{1.5, 1.5, 3.0, 5.5};
constexpr Curve ratio_3{3.0, 3.0, 4.5, 7.0};
constexpr Curve ratio_4_5{4.5, 4.5, 7.0, 11.0};
constexpr Curve ratio_6{6.0, 6.0, 7.0, 11.0};
constexpr Curve ratio_7{7.0, 7.0, 11.0, 21.0};
constexpr Curve ratio_9{9.0, 9.0, 11.0, 21.0};
constexpr Curve ratio_11{11.0, 11.0, 21.0, 21.0};

double clamp_double(double lower, double upper, double value) {
    if (value < lower) {
        return lower;
    }
    if (value > upper) {
        return upper;
    }
    return value;
}

struct HueTable {
    std::span<const double> breakpoints;
    std::span<const double> values;
};

double piecewise_hue(double hue, const HueTable& table) {
    const std::size_t size = std::min(table.breakpoints.size() - 1, table.values.size());
    for (std::size_t i = 0; i < size; ++i) {
        if (hue >= table.breakpoints[i] && hue < table.breakpoints[i + 1]) {
            return mcu::SanitizeDegreesDouble(table.values[i]);
        }
    }
    return hue;
}

double rotated_hue(double hue, const HueTable& table) {
    double rotation = piecewise_hue(hue, table);
    if (std::min(table.breakpoints.size() - 1, table.values.size()) == 0) {
        rotation = 0.0;
    }
    return mcu::SanitizeDegreesDouble(hue + rotation);
}

constexpr std::array<double, 8> neutral_tertiary_breakpoints{0, 38, 105, 161, 204, 278, 333, 360};
constexpr std::array<double, 7> neutral_tertiary_rotations{-32, 26, 10, -39, 24, -15, -32};
constexpr std::array<double, 6> tonal_spot_tertiary_breakpoints{0, 20, 71, 161, 333, 360};
constexpr std::array<double, 5> tonal_spot_tertiary_rotations{-40, 48, -32, 40, -32};
constexpr std::array<double, 9> expressive_breakpoints{0, 105, 140, 204, 253, 278, 300, 333, 360};
constexpr std::array<double, 8> expressive_secondary_rotations{-160, 155, -100, 96, -96, -156, -165, -160};
constexpr std::array<double, 8> expressive_tertiary_rotations{-165, 160, -105, 101, -101, -160, -170, -165};
constexpr std::array<double, 7> expressive_neutral_breakpoints{0, 71, 124, 253, 278, 300, 360};
constexpr std::array<double, 6> expressive_neutral_rotations{10, 0, 10, 0, 10, 0};
constexpr std::array<double, 6> vibrant_secondary_breakpoints{0, 38, 105, 140, 333, 360};
constexpr std::array<double, 5> vibrant_secondary_rotations{-14, 10, -14, 10, -14};
constexpr std::array<double, 9> vibrant_tertiary_breakpoints{0, 38, 71, 105, 140, 161, 253, 333, 360};
constexpr std::array<double, 8> vibrant_tertiary_rotations{-72, 35, 24, -24, 62, 50, 62, -72};
constexpr std::array<double, 9> error_breakpoints{0, 3, 13, 23, 33, 43, 153, 273, 360};
constexpr std::array<double, 8> error_hues{12, 22, 32, 12, 22, 32, 22, 12};

constexpr HueTable error_hue_table{error_breakpoints, error_hues};

struct NeutralPalettes {
    double primary_chroma;
    double primary_chroma_blue;
    double secondary_chroma;
    double secondary_chroma_blue;
    HueTable tertiary_hue;
    double tertiary_chroma;
    double neutral_chroma;
    double neutral_variant_scale;
    double error_chroma;
};

constexpr NeutralPalettes neutral_palettes{
    .primary_chroma = 8,
    .primary_chroma_blue = 12,
    .secondary_chroma = 4,
    .secondary_chroma_blue = 6,
    .tertiary_hue = {neutral_tertiary_breakpoints, neutral_tertiary_rotations},
    .tertiary_chroma = 20,
    .neutral_chroma = 1.4,
    .neutral_variant_scale = 2.2,
    .error_chroma = 50,
};

struct TonalSpotPalettes {
    double primary_chroma_light;
    double primary_chroma_dark;
    double secondary_chroma;
    HueTable tertiary_hue;
    double tertiary_chroma;
    double neutral_chroma;
    double neutral_variant_scale;
    double error_chroma;
};

constexpr TonalSpotPalettes tonal_spot_palettes{
    .primary_chroma_light = 32,
    .primary_chroma_dark = 26,
    .secondary_chroma = 16,
    .tertiary_hue = {tonal_spot_tertiary_breakpoints, tonal_spot_tertiary_rotations},
    .tertiary_chroma = 28,
    .neutral_chroma = 5,
    .neutral_variant_scale = 1.7,
    .error_chroma = 60,
};

struct ExpressivePalettes {
    double primary_chroma_light;
    double primary_chroma_dark;
    HueTable secondary_hue;
    double secondary_chroma_light;
    double secondary_chroma_dark;
    HueTable tertiary_hue;
    double tertiary_chroma;
    HueTable neutral_hue;
    double neutral_chroma_light;
    double neutral_chroma_dark;
    double neutral_chroma_dark_yellow;
    double neutral_variant_scale;
    double neutral_variant_scale_yellow;
    double error_chroma;
};

constexpr ExpressivePalettes expressive_palettes{
    .primary_chroma_light = 48,
    .primary_chroma_dark = 36,
    .secondary_hue = {expressive_breakpoints, expressive_secondary_rotations},
    .secondary_chroma_light = 24,
    .secondary_chroma_dark = 16,
    .tertiary_hue = {expressive_breakpoints, expressive_tertiary_rotations},
    .tertiary_chroma = 48,
    .neutral_hue = {expressive_neutral_breakpoints, expressive_neutral_rotations},
    .neutral_chroma_light = 18,
    .neutral_chroma_dark = 14,
    .neutral_chroma_dark_yellow = 6,
    .neutral_variant_scale = 2.3,
    .neutral_variant_scale_yellow = 1.6,
    .error_chroma = 64,
};

struct VibrantPalettes {
    double primary_chroma;
    HueTable secondary_hue;
    double secondary_chroma;
    HueTable tertiary_hue;
    double tertiary_chroma;
    HueTable neutral_hue;
    double neutral_chroma;
    double neutral_variant_scale;
    double error_chroma;
};

constexpr VibrantPalettes vibrant_palettes{
    .primary_chroma = 74,
    .secondary_hue = {vibrant_secondary_breakpoints, vibrant_secondary_rotations},
    .secondary_chroma = 56,
    .tertiary_hue = {vibrant_tertiary_breakpoints, vibrant_tertiary_rotations},
    .tertiary_chroma = 56,
    .neutral_hue = {vibrant_secondary_breakpoints, vibrant_secondary_rotations},
    .neutral_chroma = 28,
    .neutral_variant_scale = 1.29,
    .error_chroma = 80,
};

enum class Kind : std::size_t {
    neutral,
    tonal_spot,
    vibrant,
    expressive,
    count,
};

constexpr std::size_t kind_count = static_cast<std::size_t>(Kind::count);

std::optional<Kind> kind_of(Variant variant) {
    switch (variant) {
    case Variant::neutral: return Kind::neutral;
    case Variant::tonal_spot: return Kind::tonal_spot;
    case Variant::vibrant: return Kind::vibrant;
    case Variant::expressive: return Kind::expressive;
    default: return std::nullopt;
    }
}

enum class PaletteKind : std::size_t {
    primary,
    secondary,
    tertiary,
    neutral,
    neutral_variant,
    error,
    count,
};

struct Palette {
    double hue;
    double chroma;
};

using Palettes = std::array<Palette, static_cast<std::size_t>(PaletteKind::count)>;

Palettes palettes_for(Kind kind, double hue, bool dark) {
    const double error_hue = piecewise_hue(hue, error_hue_table);
    switch (kind) {
    case Kind::neutral: {
        const auto& p = neutral_palettes;
        const bool blue = blue_hues.contains(hue);
        return {{
            {hue, blue ? p.primary_chroma_blue : p.primary_chroma},
            {hue, blue ? p.secondary_chroma_blue : p.secondary_chroma},
            {rotated_hue(hue, p.tertiary_hue), p.tertiary_chroma},
            {hue, p.neutral_chroma},
            {hue, p.neutral_chroma * p.neutral_variant_scale},
            {error_hue, p.error_chroma},
        }};
    }
    case Kind::tonal_spot: {
        const auto& p = tonal_spot_palettes;
        return {{
            {hue, dark ? p.primary_chroma_dark : p.primary_chroma_light},
            {hue, p.secondary_chroma},
            {rotated_hue(hue, p.tertiary_hue), p.tertiary_chroma},
            {hue, p.neutral_chroma},
            {hue, p.neutral_chroma * p.neutral_variant_scale},
            {error_hue, p.error_chroma},
        }};
    }
    case Kind::expressive: {
        const auto& p = expressive_palettes;
        const double neutral_hue = rotated_hue(hue, p.neutral_hue);
        const bool yellow = yellow_hues.contains(neutral_hue);
        const double neutral_chroma = dark ? (yellow ? p.neutral_chroma_dark_yellow : p.neutral_chroma_dark) : p.neutral_chroma_light;
        return {{
            {hue, dark ? p.primary_chroma_dark : p.primary_chroma_light},
            {rotated_hue(hue, p.secondary_hue), dark ? p.secondary_chroma_dark : p.secondary_chroma_light},
            {rotated_hue(hue, p.tertiary_hue), p.tertiary_chroma},
            {neutral_hue, neutral_chroma},
            {neutral_hue, neutral_chroma * (yellow ? p.neutral_variant_scale_yellow : p.neutral_variant_scale)},
            {error_hue, p.error_chroma},
        }};
    }
    case Kind::vibrant:
    case Kind::count:
        break;
    }
    const auto& p = vibrant_palettes;
    const double neutral_hue = rotated_hue(hue, p.neutral_hue);
    return {{
        {hue, p.primary_chroma},
        {rotated_hue(hue, p.secondary_hue), p.secondary_chroma},
        {rotated_hue(hue, p.tertiary_hue), p.tertiary_chroma},
        {neutral_hue, p.neutral_chroma},
        {neutral_hue, p.neutral_chroma * p.neutral_variant_scale},
        {error_hue, p.error_chroma},
    }};
}

struct Scheme {
    Kind kind;
    bool dark;
    double contrast;
    Palettes palettes;

    const Palette& palette(PaletteKind k) const { return palettes[static_cast<std::size_t>(k)]; }
};

constexpr double default_background_lstar = 50.0;

const mcu::ViewingConditions& exact_viewing_conditions() {
    static const mcu::ViewingConditions conditions = mcu::DefaultWithBackgroundLstar(default_background_lstar);
    return conditions;
}

double hue_of(Argb argb) { return mcu::CamFromIntAndViewingConditions(argb, exact_viewing_conditions()).hue; }

double chroma_of(double hue, double chroma, double tone) {
    return mcu::CamFromIntAndViewingConditions(mcu::Hct(hue, chroma, tone).ToInt(), exact_viewing_conditions()).chroma;
}

double best_tone_for_chroma(double hue, double chroma, double tone, bool by_decreasing_tone) {
    double answer = tone;
    double best = chroma_of(hue, chroma, answer);
    while (best < chroma) {
        if (tone < tone_black || tone > tone_white) {
            break;
        }
        tone += by_decreasing_tone ? -tone_search_step : tone_search_step;
        const double candidate = chroma_of(hue, chroma, tone);
        if (best < candidate) {
            best = candidate;
            answer = tone;
        }
    }
    return answer;
}

enum class Pick {
    fixed,
    max_chroma,
    min_chroma,
    background,
};

struct ToneRule {
    Pick pick;
    double lower;
    double upper;
    double cyan_upper;
    double yellow_upper;
    double chroma_scale;
};

constexpr ToneRule fixed(double tone) { return {Pick::fixed, tone, tone, tone, tone, 1.0}; }

constexpr ToneRule max_chroma(double lower = tone_black, double upper = tone_white) {
    return {Pick::max_chroma, lower, upper, upper, upper, 1.0};
}

constexpr ToneRule min_chroma(double lower = tone_black, double upper = tone_white) {
    return {Pick::min_chroma, lower, upper, upper, upper, 1.0};
}

constexpr ToneRule from_background() { return {Pick::background, 0.0, 0.0, 0.0, 0.0, 1.0}; }

constexpr ToneRule if_cyan(ToneRule rule, double upper) {
    rule.cyan_upper = upper;
    return rule;
}

constexpr ToneRule if_yellow(ToneRule rule, double upper) {
    rule.yellow_upper = upper;
    return rule;
}

constexpr ToneRule scaled(ToneRule rule, double chroma_scale) {
    rule.chroma_scale = chroma_scale;
    return rule;
}

double picked_tone(const ToneRule& rule, const Palette& palette) {
    const double upper = cyan_hues.contains(palette.hue) ? rule.cyan_upper
                         : yellow_hues.contains(palette.hue) ? rule.yellow_upper
                                                              : rule.upper;
    switch (rule.pick) {
    case Pick::max_chroma:
        return clamp_double(rule.lower, upper, best_tone_for_chroma(palette.hue, palette.chroma * rule.chroma_scale, tone_white, true));
    case Pick::min_chroma:
        return clamp_double(rule.lower, upper, best_tone_for_chroma(palette.hue, palette.chroma, tone_black, false));
    case Pick::fixed:
    case Pick::background:
        break;
    }
    return rule.lower;
}

struct ModeTones {
    ToneRule light;
    ToneRule dark;
};

constexpr ModeTones both(ToneRule rule) { return {rule, rule}; }

using VariantTones = std::array<ModeTones, kind_count>;

constexpr VariantTones per_kind(ModeTones neutral, ModeTones tonal_spot, ModeTones vibrant, ModeTones expressive) {
    return {neutral, tonal_spot, vibrant, expressive};
}

constexpr ModeTones primary_dim_other = both(max_chroma());
constexpr ModeTones secondary_dim_other = both(max_chroma(0, 90));
constexpr ModeTones tertiary_dim_other = both(max_chroma());
constexpr ModeTones error_dim_all = both(min_chroma());

constexpr VariantTones on_surface_tones = per_kind(
    both(from_background()),
    both(from_background()),
    both(scaled(max_chroma(), 1.1)),
    both(from_background()));

constexpr VariantTones primary_tones = per_kind(
    {fixed(40), fixed(80)},
    {max_chroma(), fixed(80)},
    both(if_cyan(max_chroma(0, 98), 88)),
    {if_yellow(max_chroma(0, 98), 25), if_cyan(max_chroma(0, 98), 88)});

constexpr VariantTones primary_dim_tones = per_kind(
    both(fixed(85)),
    both(max_chroma(0, 90)),
    primary_dim_other,
    primary_dim_other);

constexpr VariantTones primary_container_tones = per_kind(
    {fixed(90), fixed(30)},
    {max_chroma(0, 90), min_chroma(35, 93)},
    {if_cyan(max_chroma(66, 93), 88), min_chroma(66, 93)},
    {if_cyan(max_chroma(78, 90), 88), min_chroma(30, 93)});

constexpr VariantTones inverse_primary_tones = per_kind(
    both(max_chroma()),
    both(max_chroma()),
    both(max_chroma()),
    both(max_chroma()));

constexpr VariantTones secondary_tones = per_kind(
    {max_chroma(), min_chroma(0, 98)},
    {max_chroma(), fixed(80)},
    {max_chroma(0, 98), max_chroma(0, 90)},
    {max_chroma(), fixed(80)});

constexpr VariantTones secondary_dim_tones = per_kind(
    both(fixed(85)),
    secondary_dim_other,
    secondary_dim_other,
    secondary_dim_other);

constexpr VariantTones secondary_container_tones = per_kind(
    {fixed(90), fixed(25)},
    {fixed(90), fixed(25)},
    {max_chroma(84, 90), min_chroma(30, 40)},
    {max_chroma(90, 95), fixed(15)});

constexpr VariantTones tertiary_tones = per_kind(
    {max_chroma(), max_chroma(0, 98)},
    {max_chroma(), max_chroma(0, 98)},
    {if_cyan(max_chroma(0, 100), 88), if_cyan(max_chroma(0, 98), 88)},
    {if_cyan(max_chroma(0, 100), 88), if_cyan(max_chroma(0, 98), 88)});

constexpr VariantTones tertiary_dim_tones = per_kind(
    tertiary_dim_other,
    both(max_chroma(0, 90)),
    tertiary_dim_other,
    tertiary_dim_other);

constexpr VariantTones tertiary_container_tones = per_kind(
    {max_chroma(0, 96), max_chroma(0, 93)},
    {max_chroma(0, 100), max_chroma(0, 93)},
    {max_chroma(72, 100), max_chroma(0, 93)},
    {if_cyan(max_chroma(75, 100), 88), if_cyan(max_chroma(75, 93), 88)});

constexpr ModeTones error_mode_tones{max_chroma(), min_chroma(0, 98)};
constexpr VariantTones error_tones = per_kind(error_mode_tones, error_mode_tones, error_mode_tones, error_mode_tones);
constexpr VariantTones error_dim_tones = per_kind(error_dim_all, error_dim_all, error_dim_all, error_dim_all);
constexpr ModeTones error_container_mode_tones{max_chroma(0, 90), min_chroma(30, 93)};
constexpr VariantTones error_container_tones =
    per_kind(error_container_mode_tones, error_container_mode_tones, error_container_mode_tones, error_container_mode_tones);

struct SurfaceTones {
    double dark;
    double light_yellow;
    double light_vibrant;
    double light;
};

enum class ScaleWhen {
    always,
    light,
    dark,
};

struct ChromaScale {
    double neutral;
    double tonal_spot;
    double vibrant;
    double expressive;
    double expressive_yellow_light;
    double expressive_yellow_dark;
    ScaleWhen when;
};

constexpr ChromaScale surface_dim_scale{2.5, 1.7, 1.36, 1.75, 2.7, 2.7, ScaleWhen::light};
constexpr ChromaScale surface_bright_scale{2.5, 1.7, 1.36, 1.75, 2.7, 2.7, ScaleWhen::dark};
constexpr ChromaScale surface_container_low_scale{1.3, 1.25, 1.08, 1.15, 1.3, 1.3, ScaleWhen::always};
constexpr ChromaScale surface_container_scale{1.6, 1.4, 1.15, 1.3, 1.6, 1.6, ScaleWhen::always};
constexpr ChromaScale surface_container_high_scale{1.9, 1.5, 1.22, 1.45, 1.95, 1.95, ScaleWhen::always};
constexpr ChromaScale surface_container_highest_scale{2.2, 1.7, 1.29, 1.6, 2.3, 2.3, ScaleWhen::always};
constexpr ChromaScale on_surface_scale{2.2, 1.7, 1.0, 1.6, 2.3, 3.0, ScaleWhen::always};

double chroma_scale_for(const ChromaScale& scale, const Scheme& s) {
    if ((scale.when == ScaleWhen::light && s.dark) || (scale.when == ScaleWhen::dark && !s.dark)) {
        return 1.0;
    }
    switch (s.kind) {
    case Kind::neutral: return scale.neutral;
    case Kind::tonal_spot: return scale.tonal_spot;
    case Kind::vibrant: return scale.vibrant;
    case Kind::expressive:
        if (yellow_hues.contains(s.palette(PaletteKind::neutral).hue)) {
            return s.dark ? scale.expressive_yellow_dark : scale.expressive_yellow_light;
        }
        return scale.expressive;
    case Kind::count: break;
    }
    return 1.0;
}

enum class ToneSource {
    table,
    surface,
    background,
    role,
    role_on_light_standard,
};

enum class BackgroundKind {
    none,
    surface_by_mode,
    role,
};

enum class Polarity {
    darker,
    lighter,
    relative_darker,
    relative_lighter,
};

enum class Constraint {
    exact,
    nearer,
    farther,
};

struct Pair {
    Role a;
    Role b;
    double delta;
    Polarity polarity;
    Constraint constraint;
};

constexpr double pair_delta = 5.0;

struct RoleDef {
    bool defined = false;
    std::optional<Role> alias;
    PaletteKind palette = PaletteKind::neutral;
    ToneSource tone = ToneSource::background;
    const VariantTones* tones = nullptr;
    SurfaceTones surface{};
    Role source = Role::count;
    bool is_background = false;
    bool keeps_tone_gap = false;
    const ChromaScale* chroma = nullptr;
    BackgroundKind background = BackgroundKind::none;
    Role background_role = Role::count;
    std::optional<Curve> light_curve;
    std::optional<Curve> dark_curve;
    bool curve_needs_raised_contrast = false;
    std::optional<Pair> pair;
};

constexpr std::size_t index_of(Role role) { return static_cast<std::size_t>(role); }

constexpr RoleDef surface_role(SurfaceTones tones, const ChromaScale* chroma) {
    RoleDef d;
    d.defined = true;
    d.palette = PaletteKind::neutral;
    d.tone = ToneSource::surface;
    d.surface = tones;
    d.is_background = true;
    d.chroma = chroma;
    return d;
}

constexpr RoleDef text_on_surface(ToneSource tone, const VariantTones* tones, Curve light, Curve dark) {
    RoleDef d;
    d.defined = true;
    d.palette = PaletteKind::neutral;
    d.tone = tone;
    d.tones = tones;
    d.chroma = &on_surface_scale;
    d.background = BackgroundKind::surface_by_mode;
    d.light_curve = light;
    d.dark_curve = dark;
    return d;
}

constexpr RoleDef on_role(PaletteKind palette, Role background, Curve curve) {
    RoleDef d;
    d.defined = true;
    d.palette = palette;
    d.background = BackgroundKind::role;
    d.background_role = background;
    d.light_curve = curve;
    d.dark_curve = curve;
    return d;
}

constexpr RoleDef accent(PaletteKind palette, const VariantTones* tones, Curve curve, std::optional<Pair> pair) {
    RoleDef d;
    d.defined = true;
    d.palette = palette;
    d.tone = ToneSource::table;
    d.tones = tones;
    d.is_background = true;
    d.background = BackgroundKind::surface_by_mode;
    d.light_curve = curve;
    d.dark_curve = curve;
    d.pair = pair;
    return d;
}

constexpr RoleDef container(PaletteKind palette, const VariantTones* tones) {
    RoleDef d = accent(palette, tones, ratio_1_5, std::nullopt);
    d.curve_needs_raised_contrast = true;
    return d;
}

constexpr RoleDef dim(PaletteKind palette, const VariantTones* tones, Role self, Role base) {
    RoleDef d = accent(palette, tones, ratio_4_5, Pair{self, base, pair_delta, Polarity::darker, Constraint::farther});
    d.background = BackgroundKind::role;
    d.background_role = Role::surface_container_high;
    return d;
}

constexpr RoleDef fixed_role(PaletteKind palette, Role container_role) {
    RoleDef d = container(palette, nullptr);
    d.tone = ToneSource::role_on_light_standard;
    d.source = container_role;
    return d;
}

constexpr RoleDef fixed_dim_role(PaletteKind palette, Role self, Role base) {
    RoleDef d;
    d.defined = true;
    d.palette = palette;
    d.tone = ToneSource::role;
    d.source = base;
    d.is_background = true;
    d.keeps_tone_gap = true;
    d.pair = Pair{self, base, pair_delta, Polarity::darker, Constraint::exact};
    return d;
}

constexpr RoleDef alias_of(Role role) {
    RoleDef d;
    d.defined = true;
    d.alias = role;
    return d;
}

constexpr SurfaceTones surface_tones{4, 99, 97, 98};
constexpr SurfaceTones surface_dim_tones{4, 90, 85, 87};
constexpr SurfaceTones surface_bright_tones{18, 99, 97, 98};
constexpr SurfaceTones surface_container_lowest_tones{0, 100, 100, 100};
constexpr SurfaceTones surface_container_low_tones{6, 98, 95, 96};
constexpr SurfaceTones surface_container_tones{9, 96, 92, 94};
constexpr SurfaceTones surface_container_high_tones{12, 94, 90, 92};
constexpr SurfaceTones surface_container_highest_tones{15, 92, 88, 90};
constexpr SurfaceTones inverse_surface_tones{98, 4, 4, 4};
constexpr SurfaceTones shadow_tones{0, 0, 0, 0};

constexpr Pair accent_pair(Role container_role, Role self) {
    return {container_role, self, pair_delta, Polarity::relative_lighter, Constraint::farther};
}

constexpr std::array<RoleDef, role_count> make_defs() {
    std::array<RoleDef, role_count> d{};
    using P = PaletteKind;
    using R = Role;
    d[index_of(R::background)] = alias_of(R::surface);
    d[index_of(R::surface)] = surface_role(surface_tones, nullptr);
    d[index_of(R::surface_dim)] = surface_role(surface_dim_tones, &surface_dim_scale);
    d[index_of(R::surface_bright)] = surface_role(surface_bright_tones, &surface_bright_scale);
    d[index_of(R::surface_container_lowest)] = surface_role(surface_container_lowest_tones, nullptr);
    d[index_of(R::surface_container_low)] = surface_role(surface_container_low_tones, &surface_container_low_scale);
    d[index_of(R::surface_container)] = surface_role(surface_container_tones, &surface_container_scale);
    d[index_of(R::surface_container_high)] = surface_role(surface_container_high_tones, &surface_container_high_scale);
    d[index_of(R::surface_container_highest)] = surface_role(surface_container_highest_tones, &surface_container_highest_scale);
    d[index_of(R::surface_variant)] = alias_of(R::surface_container_highest);
    d[index_of(R::inverse_surface)] = surface_role(inverse_surface_tones, nullptr);
    d[index_of(R::shadow)] = surface_role(shadow_tones, nullptr);
    d[index_of(R::shadow)].is_background = false;
    d[index_of(R::scrim)] = d[index_of(R::shadow)];

    d[index_of(R::on_surface)] = text_on_surface(ToneSource::table, &on_surface_tones, ratio_9, ratio_11);
    d[index_of(R::on_background)] = text_on_surface(ToneSource::role, nullptr, ratio_9, ratio_11);
    d[index_of(R::on_background)].source = R::on_surface;
    d[index_of(R::on_surface_variant)] = text_on_surface(ToneSource::background, nullptr, ratio_4_5, ratio_6);
    d[index_of(R::outline)] = text_on_surface(ToneSource::background, nullptr, ratio_3, ratio_3);
    d[index_of(R::outline_variant)] = text_on_surface(ToneSource::background, nullptr, ratio_1_5, ratio_1_5);
    d[index_of(R::inverse_on_surface)] = on_role(P::neutral, R::inverse_surface, ratio_7);

    d[index_of(R::surface_tint)] = alias_of(R::primary);
    d[index_of(R::primary)] = accent(P::primary, &primary_tones, ratio_4_5, accent_pair(R::primary_container, R::primary));
    d[index_of(R::primary_dim)] = dim(P::primary, &primary_dim_tones, R::primary_dim, R::primary);
    d[index_of(R::on_primary)] = on_role(P::primary, R::primary, ratio_6);
    d[index_of(R::primary_container)] = container(P::primary, &primary_container_tones);
    d[index_of(R::on_primary_container)] = on_role(P::primary, R::primary_container, ratio_6);
    d[index_of(R::inverse_primary)] = on_role(P::primary, R::inverse_surface, ratio_6);
    d[index_of(R::inverse_primary)].tone = ToneSource::table;
    d[index_of(R::inverse_primary)].tones = &inverse_primary_tones;

    d[index_of(R::secondary)] = accent(P::secondary, &secondary_tones, ratio_4_5, accent_pair(R::secondary_container, R::secondary));
    d[index_of(R::secondary_dim)] = dim(P::secondary, &secondary_dim_tones, R::secondary_dim, R::secondary);
    d[index_of(R::on_secondary)] = on_role(P::secondary, R::secondary, ratio_6);
    d[index_of(R::secondary_container)] = container(P::secondary, &secondary_container_tones);
    d[index_of(R::on_secondary_container)] = on_role(P::secondary, R::secondary_container, ratio_6);

    d[index_of(R::tertiary)] = accent(P::tertiary, &tertiary_tones, ratio_4_5, accent_pair(R::tertiary_container, R::tertiary));
    d[index_of(R::tertiary_dim)] = dim(P::tertiary, &tertiary_dim_tones, R::tertiary_dim, R::tertiary);
    d[index_of(R::on_tertiary)] = on_role(P::tertiary, R::tertiary, ratio_6);
    d[index_of(R::tertiary_container)] = container(P::tertiary, &tertiary_container_tones);
    d[index_of(R::on_tertiary_container)] = on_role(P::tertiary, R::tertiary_container, ratio_6);

    d[index_of(R::error)] = accent(P::error, &error_tones, ratio_4_5, accent_pair(R::error_container, R::error));
    d[index_of(R::error_dim)] = dim(P::error, &error_dim_tones, R::error_dim, R::error);
    d[index_of(R::on_error)] = on_role(P::error, R::error, ratio_6);
    d[index_of(R::error_container)] = container(P::error, &error_container_tones);
    d[index_of(R::on_error_container)] = on_role(P::error, R::error_container, ratio_4_5);

    d[index_of(R::primary_fixed)] = fixed_role(P::primary, R::primary_container);
    d[index_of(R::primary_fixed_dim)] = fixed_dim_role(P::primary, R::primary_fixed_dim, R::primary_fixed);
    d[index_of(R::on_primary_fixed)] = on_role(P::primary, R::primary_fixed_dim, ratio_7);
    d[index_of(R::on_primary_fixed_variant)] = on_role(P::primary, R::primary_fixed_dim, ratio_4_5);
    d[index_of(R::secondary_fixed)] = fixed_role(P::secondary, R::secondary_container);
    d[index_of(R::secondary_fixed_dim)] = fixed_dim_role(P::secondary, R::secondary_fixed_dim, R::secondary_fixed);
    d[index_of(R::on_secondary_fixed)] = on_role(P::secondary, R::secondary_fixed_dim, ratio_7);
    d[index_of(R::on_secondary_fixed_variant)] = on_role(P::secondary, R::secondary_fixed_dim, ratio_4_5);
    d[index_of(R::tertiary_fixed)] = fixed_role(P::tertiary, R::tertiary_container);
    d[index_of(R::tertiary_fixed_dim)] = fixed_dim_role(P::tertiary, R::tertiary_fixed_dim, R::tertiary_fixed);
    d[index_of(R::on_tertiary_fixed)] = on_role(P::tertiary, R::tertiary_fixed_dim, ratio_7);
    d[index_of(R::on_tertiary_fixed_variant)] = on_role(P::tertiary, R::tertiary_fixed_dim, ratio_4_5);
    return d;
}

constexpr std::array<RoleDef, role_count> defs = make_defs();

static_assert(std::ranges::all_of(defs, [](const RoleDef& d) { return d.defined; }));

Role resolve(Role role) {
    const RoleDef& d = defs[index_of(role)];
    return d.alias ? *d.alias : role;
}

std::optional<Role> background_of(const Scheme& s, const RoleDef& d) {
    switch (d.background) {
    case BackgroundKind::surface_by_mode: return s.dark ? Role::surface_bright : Role::surface_dim;
    case BackgroundKind::role: return d.background_role;
    case BackgroundKind::none: break;
    }
    return std::nullopt;
}

std::optional<Curve> curve_of(const Scheme& s, const RoleDef& d) {
    if (d.curve_needs_raised_contrast && !(s.contrast > contrast_levels.standard)) {
        return std::nullopt;
    }
    return s.dark ? d.dark_curve : d.light_curve;
}

double tone_of(const Scheme& s, Role role);

double requested_tone(const Scheme& s, const RoleDef& d) {
    switch (d.tone) {
    case ToneSource::table: {
        const ModeTones& modes = (*d.tones)[static_cast<std::size_t>(s.kind)];
        const ToneRule& rule = s.dark ? modes.dark : modes.light;
        if (rule.pick != Pick::background) {
            return picked_tone(rule, s.palette(d.palette));
        }
        break;
    }
    case ToneSource::surface:
        if (s.dark) {
            return d.surface.dark;
        }
        if (yellow_hues.contains(s.palette(PaletteKind::neutral).hue)) {
            return d.surface.light_yellow;
        }
        return s.kind == Kind::vibrant ? d.surface.light_vibrant : d.surface.light;
    case ToneSource::role: return tone_of(s, d.source);
    case ToneSource::role_on_light_standard: {
        Scheme light = s;
        light.dark = false;
        light.contrast = contrast_levels.standard;
        return tone_of(light, d.source);
    }
    case ToneSource::background: break;
    }
    const auto bg = background_of(s, d);
    return bg ? tone_of(s, *bg) : tone_without_background;
}

double keep_tone_gap(const RoleDef& d, double tone) {
    if (!d.is_background || d.keeps_tone_gap) {
        return tone;
    }
    if (tone >= background_tone_gap.threshold) {
        return clamp_double(background_tone_gap.light_min, tone_white, tone);
    }
    return clamp_double(tone_black, background_tone_gap.dark_max, tone);
}

double meet_contrast(const Scheme& s, double tone, double bg_tone, double ratio) {
    return mcu::RatioOfTones(bg_tone, tone) >= ratio && s.contrast >= contrast_levels.standard ? tone : mcu::ForegroundTone(bg_tone, ratio);
}

double tone_of(const Scheme& s, Role role) {
    const Role self = resolve(role);
    const RoleDef& d = defs[index_of(self)];
    const auto bg = background_of(s, d);
    const auto curve = curve_of(s, d);
    if (d.pair) {
        const Pair& p = *d.pair;
        const bool negative = p.polarity == Polarity::darker || (p.polarity == Polarity::relative_lighter && s.dark) ||
                              (p.polarity == Polarity::relative_darker && !s.dark);
        const double absolute_delta = negative ? -p.delta : p.delta;
        const bool am_a = self == p.a;
        const Role self_role = am_a ? p.a : p.b;
        const Role reference_role = am_a ? p.b : p.a;
        double self_tone = requested_tone(s, defs[index_of(self_role)]);
        const double reference_tone = tone_of(s, reference_role);
        const double relative_delta = am_a ? absolute_delta : -absolute_delta;
        switch (p.constraint) {
        case Constraint::exact:
            self_tone = clamp_double(tone_black, tone_white, reference_tone + relative_delta);
            break;
        case Constraint::nearer:
            if (relative_delta > 0) {
                self_tone = clamp_double(tone_black, tone_white, clamp_double(reference_tone, reference_tone + relative_delta, self_tone));
            } else {
                self_tone = clamp_double(tone_black, tone_white, clamp_double(reference_tone + relative_delta, reference_tone, self_tone));
            }
            break;
        case Constraint::farther:
            if (relative_delta > 0) {
                self_tone = clamp_double(reference_tone + relative_delta, tone_white, self_tone);
            } else {
                self_tone = clamp_double(tone_black, reference_tone + relative_delta, self_tone);
            }
            break;
        }
        if (bg && curve) {
            self_tone = meet_contrast(s, self_tone, tone_of(s, *bg), curve_value(*curve, s.contrast));
        }
        return keep_tone_gap(d, self_tone);
    }
    const double answer = requested_tone(s, d);
    if (!bg || !curve) {
        return answer;
    }
    return keep_tone_gap(d, meet_contrast(s, answer, tone_of(s, *bg), curve_value(*curve, s.contrast)));
}

Argb average_argb(Argb a, Argb b) {
    const int red = (mcu::RedFromInt(a) + mcu::RedFromInt(b) + 1) / 2;
    const int green = (mcu::GreenFromInt(a) + mcu::GreenFromInt(b) + 1) / 2;
    const int blue = (mcu::BlueFromInt(a) + mcu::BlueFromInt(b) + 1) / 2;
    return mcu::ArgbFromRgb(red, green, blue);
}

Argb palette_argb(double hue, double chroma, double tone) {
    if (tone == yellow_light_tone.tone && yellow_hues.contains(hue)) {
        return average_argb(mcu::Hct(hue, chroma, yellow_light_tone.below).ToInt(), mcu::Hct(hue, chroma, yellow_light_tone.above).ToInt());
    }
    return mcu::Hct(hue, chroma, tone).ToInt();
}

Argb argb_of(const Scheme& s, Role role) {
    const RoleDef& d = defs[index_of(resolve(role))];
    const Palette& palette = s.palette(d.palette);
    const double tone = tone_of(s, role);
    const double scale = d.chroma ? chroma_scale_for(*d.chroma, s) : 1.0;
    return palette_argb(palette.hue, palette.chroma * scale, tone);
}

struct FallbackDim {
    Role role;
    const mcu::TonalPalette mcu::DynamicScheme::*palette;
    ModeTones tones;
    mcu::DynamicColor (*base)();
};

constexpr Curve dim_curve = ratio_4_5;

const std::array<FallbackDim, 4> fallback_dims{{
    {Role::primary_dim, &mcu::DynamicScheme::primary_palette, primary_dim_other, &mcu::MaterialDynamicColors::Primary},
    {Role::secondary_dim, &mcu::DynamicScheme::secondary_palette, secondary_dim_other, &mcu::MaterialDynamicColors::Secondary},
    {Role::tertiary_dim, &mcu::DynamicScheme::tertiary_palette, tertiary_dim_other, &mcu::MaterialDynamicColors::Tertiary},
    {Role::error_dim, &mcu::DynamicScheme::error_palette, error_dim_all, &mcu::MaterialDynamicColors::Error},
}};

double fallback_dim_tone(const mcu::DynamicScheme& m, const FallbackDim& f) {
    const mcu::TonalPalette& tonal = m.*f.palette;
    const Palette palette{tonal.get_hue(), tonal.get_chroma()};
    mcu::DynamicColor base = f.base();
    const double dim_tone = picked_tone(m.is_dark ? f.tones.dark : f.tones.light, palette);
    const double base_tone = base.tone_(m);
    const bool dim_is_nearer = !m.is_dark;
    const double expansion = m.is_dark ? 1.0 : -1.0;
    double nearer = dim_is_nearer ? dim_tone : base_tone;
    double farther = dim_is_nearer ? base_tone : dim_tone;
    if (base.contrast_curve_) {
        const Curve base_curve{base.contrast_curve_->low, base.contrast_curve_->normal, base.contrast_curve_->medium, base.contrast_curve_->high};
        const double near_ratio = curve_value(dim_is_nearer ? dim_curve : base_curve, m.contrast_level);
        const double far_ratio = curve_value(dim_is_nearer ? base_curve : dim_curve, m.contrast_level);
        const double bg_tone = mcu::MaterialDynamicColors::SurfaceContainerHigh().GetTone(m);
        if (mcu::RatioOfTones(bg_tone, nearer) < near_ratio) {
            nearer = mcu::ForegroundTone(bg_tone, near_ratio);
        }
        if (mcu::RatioOfTones(bg_tone, farther) < far_ratio) {
            farther = mcu::ForegroundTone(bg_tone, far_ratio);
        }
        if (m.contrast_level < contrast_levels.standard) {
            nearer = mcu::ForegroundTone(bg_tone, near_ratio);
            farther = mcu::ForegroundTone(bg_tone, far_ratio);
        }
    }
    if ((farther - nearer) * expansion < pair_delta) {
        farther = clamp_double(tone_black, tone_white, nearer + pair_delta * expansion);
        if ((farther - nearer) * expansion < pair_delta) {
            nearer = clamp_double(tone_black, tone_white, farther - pair_delta * expansion);
        }
    }
    const auto& band = avoided_tone_band_2021;
    if ((band.from <= nearer && nearer < band.to) || (band.from <= farther && farther < band.to)) {
        if (expansion > 0) {
            nearer = band.light_side;
            farther = std::max(farther, nearer + pair_delta * expansion);
        } else {
            nearer = band.dark_side;
            farther = std::min(farther, nearer + pair_delta * expansion);
        }
    }
    return dim_is_nearer ? nearer : farther;
}

class Spec2025 final : public ColorSpec {
public:
    ColorScheme build(const SchemeOptions& options) const override {
        const auto kind = kind_of(options.variant);
        if (!kind) {
            return build_fallback(options);
        }
        const double hue = hue_of(options.seed);
        const Scheme s{*kind, options.dark, options.contrast, palettes_for(*kind, hue, options.dark)};
        ColorScheme result{};
        for (std::size_t i = 0; i < role_count; ++i) {
            result.colors[i] = argb_of(s, static_cast<Role>(i));
        }
        return result;
    }

private:
    static ColorScheme build_fallback(const SchemeOptions& options) {
        ColorScheme result = spec_2021().build(options);
        const auto m = detail::make_mcu_scheme(options);
        for (const FallbackDim& f : fallback_dims) {
            const mcu::TonalPalette& tonal = (*m).*f.palette;
            result.colors[index_of(f.role)] = palette_argb(tonal.get_hue(), tonal.get_chroma(), fallback_dim_tone(*m, f));
        }
        return result;
    }
};

}

const ColorSpec& spec_2025() {
    static const Spec2025 instance;
    return instance;
}

}
