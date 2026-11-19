#include "lamina/color/mcu_scheme.hpp"

#include <memory>

#include "cpp/cam/hct.h"
#include "cpp/dynamiccolor/dynamic_scheme.h"
#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_fidelity.h"
#include "cpp/scheme/scheme_fruit_salad.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_rainbow.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"

namespace lamina::color::detail {

std::unique_ptr<material_color_utilities::DynamicScheme> make_mcu_scheme(const SchemeOptions& o) {
    namespace mcu = material_color_utilities;
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

}
