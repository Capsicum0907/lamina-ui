#include "lamina/color/scheme.hpp"

#include <algorithm>

#include "cpp/dynamiccolor/dynamic_scheme.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"
#include "lamina/color/mcu_scheme.hpp"

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
    &mcu::MaterialDynamicColors::Primary,
    &mcu::MaterialDynamicColors::Secondary,
    &mcu::MaterialDynamicColors::Tertiary,
    &mcu::MaterialDynamicColors::Error,
};

static_assert(std::ranges::none_of(role_sources, [](RoleSource source) { return source == nullptr; }));

class Spec2021 final : public ColorSpec {
public:
    ColorScheme build(const SchemeOptions& options) const override {
        const auto scheme = detail::make_mcu_scheme(options);
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
