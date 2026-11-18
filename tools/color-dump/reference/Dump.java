import dynamiccolor.ColorSpec;
import dynamiccolor.ColorSpec.SpecVersion;
import dynamiccolor.ColorSpecs;
import dynamiccolor.DynamicColor;
import dynamiccolor.DynamicScheme;
import dynamiccolor.DynamicScheme.Platform;
import hct.Hct;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;
import java.util.function.Function;
import scheme.SchemeContent;
import scheme.SchemeExpressive;
import scheme.SchemeFidelity;
import scheme.SchemeFruitSalad;
import scheme.SchemeMonochrome;
import scheme.SchemeNeutral;
import scheme.SchemeRainbow;
import scheme.SchemeTonalSpot;
import scheme.SchemeVibrant;

public final class Dump {
  static final int[] SEEDS = {
    0xff6750a4, 0xffb33b15, 0xffffde3f, 0xffcddc39, 0xff9e9d24, 0xff009688, 0xff00bcd4, 0xff1e88e5,
    0xff7a7a80, 0xff0b6e4f, 0xffe91e63, 0xff8bc34a,
  };
  static final String[] VARIANTS = {
    "monochrome", "neutral", "tonal_spot", "vibrant", "expressive",
    "fidelity", "content", "rainbow", "fruit_salad",
  };
  static final double[] CONTRASTS = {-1.0, 0.0, 0.5, 1.0};

  static DynamicScheme scheme(String variant, Hct seed, boolean dark, double contrast, SpecVersion spec) {
    Platform p = Platform.PHONE;
    return switch (variant) {
      case "monochrome" -> new SchemeMonochrome(seed, dark, contrast, spec, p);
      case "neutral" -> new SchemeNeutral(seed, dark, contrast, spec, p);
      case "tonal_spot" -> new SchemeTonalSpot(seed, dark, contrast, spec, p);
      case "vibrant" -> new SchemeVibrant(seed, dark, contrast, spec, p);
      case "expressive" -> new SchemeExpressive(seed, dark, contrast, spec, p);
      case "fidelity" -> new SchemeFidelity(seed, dark, contrast, spec, p);
      case "content" -> new SchemeContent(seed, dark, contrast, spec, p);
      case "rainbow" -> new SchemeRainbow(seed, dark, contrast, spec, p);
      case "fruit_salad" -> new SchemeFruitSalad(seed, dark, contrast, spec, p);
      default -> throw new IllegalArgumentException(variant);
    };
  }

  static Map<String, Function<ColorSpec, DynamicColor>> roles() {
    Map<String, Function<ColorSpec, DynamicColor>> m = new LinkedHashMap<>();
    m.put("background", ColorSpec::background);
    m.put("on_background", ColorSpec::onBackground);
    m.put("surface", ColorSpec::surface);
    m.put("surface_dim", ColorSpec::surfaceDim);
    m.put("surface_bright", ColorSpec::surfaceBright);
    m.put("surface_container_lowest", ColorSpec::surfaceContainerLowest);
    m.put("surface_container_low", ColorSpec::surfaceContainerLow);
    m.put("surface_container", ColorSpec::surfaceContainer);
    m.put("surface_container_high", ColorSpec::surfaceContainerHigh);
    m.put("surface_container_highest", ColorSpec::surfaceContainerHighest);
    m.put("on_surface", ColorSpec::onSurface);
    m.put("surface_variant", ColorSpec::surfaceVariant);
    m.put("on_surface_variant", ColorSpec::onSurfaceVariant);
    m.put("inverse_surface", ColorSpec::inverseSurface);
    m.put("inverse_on_surface", ColorSpec::inverseOnSurface);
    m.put("outline", ColorSpec::outline);
    m.put("outline_variant", ColorSpec::outlineVariant);
    m.put("shadow", ColorSpec::shadow);
    m.put("scrim", ColorSpec::scrim);
    m.put("surface_tint", ColorSpec::surfaceTint);
    m.put("primary", ColorSpec::primary);
    m.put("on_primary", ColorSpec::onPrimary);
    m.put("primary_container", ColorSpec::primaryContainer);
    m.put("on_primary_container", ColorSpec::onPrimaryContainer);
    m.put("inverse_primary", ColorSpec::inversePrimary);
    m.put("secondary", ColorSpec::secondary);
    m.put("on_secondary", ColorSpec::onSecondary);
    m.put("secondary_container", ColorSpec::secondaryContainer);
    m.put("on_secondary_container", ColorSpec::onSecondaryContainer);
    m.put("tertiary", ColorSpec::tertiary);
    m.put("on_tertiary", ColorSpec::onTertiary);
    m.put("tertiary_container", ColorSpec::tertiaryContainer);
    m.put("on_tertiary_container", ColorSpec::onTertiaryContainer);
    m.put("error", ColorSpec::error);
    m.put("on_error", ColorSpec::onError);
    m.put("error_container", ColorSpec::errorContainer);
    m.put("on_error_container", ColorSpec::onErrorContainer);
    m.put("primary_fixed", ColorSpec::primaryFixed);
    m.put("primary_fixed_dim", ColorSpec::primaryFixedDim);
    m.put("on_primary_fixed", ColorSpec::onPrimaryFixed);
    m.put("on_primary_fixed_variant", ColorSpec::onPrimaryFixedVariant);
    m.put("secondary_fixed", ColorSpec::secondaryFixed);
    m.put("secondary_fixed_dim", ColorSpec::secondaryFixedDim);
    m.put("on_secondary_fixed", ColorSpec::onSecondaryFixed);
    m.put("on_secondary_fixed_variant", ColorSpec::onSecondaryFixedVariant);
    m.put("tertiary_fixed", ColorSpec::tertiaryFixed);
    m.put("tertiary_fixed_dim", ColorSpec::tertiaryFixedDim);
    m.put("on_tertiary_fixed", ColorSpec::onTertiaryFixed);
    m.put("on_tertiary_fixed_variant", ColorSpec::onTertiaryFixedVariant);
    m.put("primary_dim", ColorSpec::primaryDim);
    m.put("secondary_dim", ColorSpec::secondaryDim);
    m.put("tertiary_dim", ColorSpec::tertiaryDim);
    m.put("error_dim", ColorSpec::errorDim);
    return m;
  }

  public static void main(String[] args) {
    int[] seeds = args.length == 0 ? SEEDS : new int[args.length];
    for (int i = 0; i < args.length; i++) {
      seeds[i] = (int) Long.parseLong(args[i], 16);
    }
    StringBuilder out = new StringBuilder();
    for (int seed : seeds) {
      out.append(String.format(Locale.ROOT, "# seed %08x hue %.4f chroma %.4f tone %.4f%n",
          seed, Hct.fromInt(seed).getHue(), Hct.fromInt(seed).getChroma(), Hct.fromInt(seed).getTone()));
    }
    Map<String, Function<ColorSpec, DynamicColor>> roles = roles();
    for (SpecVersion spec : new SpecVersion[] {SpecVersion.SPEC_2021, SpecVersion.SPEC_2025}) {
      String specName = spec == SpecVersion.SPEC_2021 ? "2021" : "2025";
      ColorSpec colors = ColorSpecs.get(spec);
      for (int seed : seeds) {
        Hct seedHct = Hct.fromInt(seed);
        for (String variant : VARIANTS) {
          for (boolean dark : new boolean[] {false, true}) {
            for (double contrast : CONTRASTS) {
              DynamicScheme s = scheme(variant, seedHct, dark, contrast, spec);
              for (Map.Entry<String, Function<ColorSpec, DynamicColor>> e : roles.entrySet()) {
                DynamicColor color = e.getValue().apply(colors);
                String value = color == null ? "null" : String.format("%08x", color.getArgb(s));
                out.append(String.format(Locale.ROOT, "%s %08x %s %s %.1f %s %s%n",
                    specName, seed, variant, dark ? "dark" : "light", contrast, e.getKey(), value));
              }
            }
          }
        }
      }
    }
    System.out.print(out);
  }
}
