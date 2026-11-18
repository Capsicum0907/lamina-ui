#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lamina/color/scheme.hpp"

namespace {

using namespace lamina::color;

constexpr std::array<Argb, 12> seeds{
    0xff6750a4, 0xffb33b15, 0xffffde3f, 0xffcddc39, 0xff9e9d24, 0xff009688,
    0xff00bcd4, 0xff1e88e5, 0xff7a7a80, 0xff0b6e4f, 0xffe91e63, 0xff8bc34a,
};

constexpr std::array<std::pair<Variant, std::string_view>, 9> variants{{
    {Variant::monochrome, "monochrome"},
    {Variant::neutral, "neutral"},
    {Variant::tonal_spot, "tonal_spot"},
    {Variant::vibrant, "vibrant"},
    {Variant::expressive, "expressive"},
    {Variant::fidelity, "fidelity"},
    {Variant::content, "content"},
    {Variant::rainbow, "rainbow"},
    {Variant::fruit_salad, "fruit_salad"},
}};

constexpr std::array<double, 4> contrasts{-1.0, 0.0, 0.5, 1.0};

constexpr std::array<std::string_view, role_count> role_names{
    "background",
    "on_background",
    "surface",
    "surface_dim",
    "surface_bright",
    "surface_container_lowest",
    "surface_container_low",
    "surface_container",
    "surface_container_high",
    "surface_container_highest",
    "on_surface",
    "surface_variant",
    "on_surface_variant",
    "inverse_surface",
    "inverse_on_surface",
    "outline",
    "outline_variant",
    "shadow",
    "scrim",
    "surface_tint",
    "primary",
    "on_primary",
    "primary_container",
    "on_primary_container",
    "inverse_primary",
    "secondary",
    "on_secondary",
    "secondary_container",
    "on_secondary_container",
    "tertiary",
    "on_tertiary",
    "tertiary_container",
    "on_tertiary_container",
    "error",
    "on_error",
    "error_container",
    "on_error_container",
    "primary_fixed",
    "primary_fixed_dim",
    "on_primary_fixed",
    "on_primary_fixed_variant",
    "secondary_fixed",
    "secondary_fixed_dim",
    "on_secondary_fixed",
    "on_secondary_fixed_variant",
    "tertiary_fixed",
    "tertiary_fixed_dim",
    "on_tertiary_fixed",
    "on_tertiary_fixed_variant",
    "primary_dim",
    "secondary_dim",
    "tertiary_dim",
    "error_dim",
};

void dump(std::string_view spec_name, const ColorSpec& spec, const std::vector<Argb>& seed_list) {
    for (Argb seed : seed_list) {
        for (const auto& [variant, variant_name] : variants) {
            for (bool dark : {false, true}) {
                for (double contrast : contrasts) {
                    const ColorScheme scheme = spec.build({.seed = seed, .dark = dark, .variant = variant, .contrast = contrast});
                    for (std::size_t i = 0; i < role_count; ++i) {
                        std::printf("%.*s %08x %.*s %s %.1f %.*s %08x\n",
                                    static_cast<int>(spec_name.size()), spec_name.data(),
                                    seed,
                                    static_cast<int>(variant_name.size()), variant_name.data(),
                                    dark ? "dark" : "light",
                                    contrast,
                                    static_cast<int>(role_names[i].size()), role_names[i].data(),
                                    scheme.colors[i]);
                    }
                }
            }
        }
    }
}

}

int main(int argc, char** argv) {
    std::vector<Argb> seed_list(seeds.begin(), seeds.end());
    if (argc > 1) {
        seed_list.clear();
        for (int i = 1; i < argc; ++i) {
            seed_list.push_back(static_cast<Argb>(std::stoul(argv[i], nullptr, 16)));
        }
    }
    dump("2021", spec_2021(), seed_list);
    dump("2025", spec_2025(), seed_list);
}
