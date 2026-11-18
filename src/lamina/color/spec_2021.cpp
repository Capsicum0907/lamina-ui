#include "lamina/color/scheme.hpp"

#include <memory>

#include "cpp/cam/hct.h"
#include "cpp/dynamiccolor/dynamic_scheme.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"
#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_fidelity.h"
#include "cpp/scheme/scheme_fruit_salad.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_rainbow.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"

namespace lamina::color {

namespace {

namespace mcu = material_color_utilities;

using RoleSource = mcu::DynamicColor (*)();

constexpr std::array<RoleSource, role_count> role_sources{
    &mcu::MaterialDynamicColors::Background,
    &mcu::MaterialDynamicColors::OnBackground,
    &mcu::MaterialDynamicColors::Surface,
    &mcu::MaterialDynamicColors::SurfaceDim,
    &mcu::MaterialDynamicColors::SurfaceBright,
    &mcu::MaterialDynamicColors::SurfaceContainerLowest,
    &mcu::MaterialDynamicColors::SurfaceContainerLow,
    &mcu::MaterialDynamicColors::SurfaceContainer,
    &mcu::MaterialDynamicColors::SurfaceContainerHigh,
    &mcu::MaterialDynamicColors::SurfaceContainerHighest,
    &mcu::MaterialDynamicColors::OnSurface,
    &mcu::MaterialDynamicColors::SurfaceVariant,
    &mcu::MaterialDynamicColors::OnSurfaceVariant,
    &mcu::MaterialDynamicColors::InverseSurface,
    &mcu::MaterialDynamicColors::InverseOnSurface,
    &mcu::MaterialDynamicColors::Outline,
    &mcu::MaterialDynamicColors::OutlineVariant,
    &mcu::MaterialDynamicColors::Shadow,
    &mcu::MaterialDynamicColors::Scrim,
    &mcu::MaterialDynamicColors::SurfaceTint,
    &mcu::MaterialDynamicColors::Primary,
    &mcu::MaterialDynamicColors::OnPrimary,
    &mcu::MaterialDynamicColors::PrimaryContainer,
    &mcu::MaterialDynamicColors::OnPrimaryContainer,
    &mcu::MaterialDynamicColors::InversePrimary,
    &mcu::MaterialDynamicColors::Secondary,
    &mcu::MaterialDynamicColors::OnSecondary,
    &mcu::MaterialDynamicColors::SecondaryContainer,
    &mcu::MaterialDynamicColors::OnSecondaryContainer,
    &mcu::MaterialDynamicColors::Tertiary,
    &mcu::MaterialDynamicColors::OnTertiary,
    &mcu::MaterialDynamicColors::TertiaryContainer,
    &mcu::MaterialDynamicColors::OnTertiaryContainer,
    &mcu::MaterialDynamicColors::Error,
    &mcu::MaterialDynamicColors::OnError,
    &mcu::MaterialDynamicColors::ErrorContainer,
    &mcu::MaterialDynamicColors::OnErrorContainer,
    &mcu::MaterialDynamicColors::PrimaryFixed,
    &mcu::MaterialDynamicColors::PrimaryFixedDim,
    &mcu::MaterialDynamicColors::OnPrimaryFixed,
    &mcu::MaterialDynamicColors::OnPrimaryFixedVariant,
    &mcu::MaterialDynamicColors::SecondaryFixed,
    &mcu::MaterialDynamicColors::SecondaryFixedDim,
    &mcu::MaterialDynamicColors::OnSecondaryFixed,
    &mcu::MaterialDynamicColors::OnSecondaryFixedVariant,
    &mcu::MaterialDynamicColors::TertiaryFixed,
    &mcu::MaterialDynamicColors::TertiaryFixedDim,
    &mcu::MaterialDynamicColors::OnTertiaryFixed,
    &mcu::MaterialDynamicColors::OnTertiaryFixedVariant,
};

std::unique_ptr<mcu::DynamicScheme> make_scheme(const SchemeOptions& o) {
    const mcu::Hct seed(o.seed);
    switch (o.variant) {
    case Variant::monochrome: return std::make_unique<mcu::SchemeMonochrome>(seed, o.dark, o.contrast);
    case Variant::neutral: return std::make_unique<mcu::SchemeNeutral>(seed, o.dark, o.contrast);
    case Variant::tonal_spot: return std::make_unique<mcu::SchemeTonalSpot>(seed, o.dark, o.contrast);
    case Variant::vibrant: return std::make_unique<mcu::SchemeVibrant>(seed, o.dark, o.contrast);
    case Variant::expressive: return std::make_unique<mcu::SchemeExpressive>(seed, o.dark, o.contrast);
    case Variant::fidelity: return std::make_unique<mcu::SchemeFidelity>(seed, o.dark, o.contrast);
    case Variant::content: return std::make_unique<mcu::SchemeContent>(seed, o.dark, o.contrast);
    case Variant::rainbow: return std::make_unique<mcu::SchemeRainbow>(seed, o.dark, o.contrast);
    case Variant::fruit_salad: return std::make_unique<mcu::SchemeFruitSalad>(seed, o.dark, o.contrast);
    }
    return std::make_unique<mcu::SchemeTonalSpot>(seed, o.dark, o.contrast);
}

class Spec2021 final : public ColorSpec {
public:
    ColorScheme build(const SchemeOptions& options) const override {
        const auto scheme = make_scheme(options);
        ColorScheme result{};
        for (std::size_t i = 0; i < role_count; ++i) {
            result.colors[i] = role_sources[i]().GetArgb(*scheme);
        }
        return result;
    }
};

}

const ColorSpec& spec_2021() {
    static const Spec2021 instance;
    return instance;
}

}
