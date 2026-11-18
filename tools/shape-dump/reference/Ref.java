import androidx.graphics.shapes.*;
import java.lang.reflect.Proxy;
import java.util.*;

public class Ref {
    record PNR(float x, float y, CornerRounding r) {}
    static CornerRounding cr(float r) { return new CornerRounding(r, 0f); }
    static CornerRounding cr(float r, float s) { return new CornerRounding(r, s); }
    static PNR p(float x, float y) { return new PNR(x, y, CornerRounding.Unrounded); }
    static PNR p(float x, float y, CornerRounding r) { return new PNR(x, y, r); }

    static long pack(float a, float b) {
        return (((long) Float.floatToRawIntBits(a)) << 32) | (((long) Float.floatToRawIntBits(b)) & 0xFFFFFFFFL);
    }

    interface F2 { float[] apply(float x, float y); }

    static RoundedPolygon tr(RoundedPolygon poly, F2 f) {
        PointTransformer t = (PointTransformer) Proxy.newProxyInstance(Ref.class.getClassLoader(),
            new Class<?>[] {PointTransformer.class}, (proxy, m, args) -> {
                if (m.getName().startsWith("transform")) {
                    float[] r = f.apply((Float) args[0], (Float) args[1]);
                    return pack(r[0], r[1]);
                }
                throw new UnsupportedOperationException(m.getName());
            });
        return poly.transformed(t);
    }

    static float[] rot(float degrees) {
        double r = degrees * (Math.PI / 180.0);
        float s = (float) Math.sin(r);
        float c = (float) Math.cos(r);
        return new float[] {c, s, -s, c};
    }
    static float[] scale(float x, float y) { return new float[] {x, 0f, 0f, y}; }
    static RoundedPolygon mat(RoundedPolygon poly, float[] m) {
        return tr(poly, (x, y) -> new float[] {m[0] * x + m[2] * y + 0f, m[1] * x + m[3] * y + 0f});
    }

    static final float PIF = (float) Math.PI;
    static float toRadians(float d) { return d / 360f * 2 * PIF; }
    static float angleDegrees(float x, float y) { return (float) Math.atan2(y, x) * 180f / PIF; }

    static List<PNR> doRepeat(List<PNR> points, int reps, float cx, float cy, boolean mirroring) {
        List<PNR> out = new ArrayList<>();
        if (mirroring) {
            float[] angles = new float[points.size()];
            float[] dists = new float[points.size()];
            for (int k = 0; k < points.size(); k++) {
                float ox = points.get(k).x - cx, oy = points.get(k).y - cy;
                angles[k] = angleDegrees(ox, oy);
                dists[k] = (float) Math.sqrt(ox * ox + oy * oy);
            }
            int actualReps = reps * 2;
            float sectionAngle = 360f / actualReps;
            for (int it = 0; it < actualReps; it++) {
                for (int index = 0; index < points.size(); index++) {
                    int i = (it % 2 == 0) ? index : points.size() - 1 - index;
                    if (i > 0 || it % 2 == 0) {
                        float a = toRadians(sectionAngle * it + ((it % 2 == 0) ? angles[i] : sectionAngle - angles[i] + 2 * angles[0]));
                        float fx = (float) Math.cos(a) * dists[i] + cx;
                        float fy = (float) Math.sin(a) * dists[i] + cy;
                        out.add(new PNR(fx, fy, points.get(i).r));
                    }
                }
            }
        } else {
            int np = points.size();
            for (int it = 0; it < np * reps; it++) {
                PNR s = points.get(it % np);
                float angle = (it / np) * 360f / reps;
                float a = toRadians(angle);
                float ox = s.x - cx, oy = s.y - cy;
                float rx = ox * (float) Math.cos(a) - oy * (float) Math.sin(a) + cx;
                float ry = ox * (float) Math.sin(a) + oy * (float) Math.cos(a) + cy;
                out.add(new PNR(rx, ry, s.r));
            }
        }
        return out;
    }

    static RoundedPolygon custom(List<PNR> pnr, int reps, boolean mirroring) {
        List<PNR> pts = doRepeat(pnr, reps, 0.5f, 0.5f, mirroring);
        float[] v = new float[pts.size() * 2];
        List<CornerRounding> pv = new ArrayList<>();
        for (int i = 0; i < pts.size(); i++) { v[2 * i] = pts.get(i).x; v[2 * i + 1] = pts.get(i).y; pv.add(pts.get(i).r); }
        return RoundedPolygonKt.RoundedPolygon(v, CornerRounding.Unrounded, pv, 0.5f, 0.5f);
    }
    static RoundedPolygon custom(List<PNR> pnr, int reps) { return custom(pnr, reps, false); }

    static RoundedPolygon star(int n, float inner, CornerRounding r) {
        return ShapesKt.star(RoundedPolygon.Companion, n, 1f, inner, r, null, null, 0f, 0f);
    }

    static List<RoundedPolygon> shapes() {
        CornerRounding r15 = cr(.15f), r20 = cr(.2f), r30 = cr(.3f), r50 = cr(.5f), r100 = cr(1f);
        List<RoundedPolygon> s = new ArrayList<>();
        s.add(ShapesKt.circle(RoundedPolygon.Companion, 10, 1f, 0f, 0f));
        s.add(ShapesKt.rectangle(RoundedPolygon.Companion, 1f, 1f, r30, null, 0f, 0f));
        s.add(custom(List.of(p(0.926f, 0.970f, cr(0.189f, 0.811f)), p(-0.021f, 0.967f, cr(0.187f, 0.057f))), 2));
        s.add(mat(RoundedPolygonKt.RoundedPolygon(4, 1f, 0f, 0f, CornerRounding.Unrounded, List.of(r100, r100, r20, r20)), rot(-135f)));
        s.add(custom(List.of(p(1.004f, 1.000f, cr(0.148f, 0.417f)), p(0.000f, 1.000f, cr(0.151f)), p(0.000f, -0.003f, cr(0.148f)), p(0.978f, 0.020f, cr(0.803f))), 1));
        s.add(custom(List.of(p(0.500f, 0.892f, cr(0.313f)), p(-0.216f, 1.050f, cr(0.207f)), p(0.499f, -0.160f, cr(0.215f, 1.000f)), p(1.225f, 1.060f, cr(0.211f))), 1));
        s.add(ShapesKt.rectangle(RoundedPolygon.Companion, 1.6f, 1f, CornerRounding.Unrounded, List.of(r20, r20, r100, r100), 0f, 0f));
        s.add(mat(mat(ShapesKt.circle(RoundedPolygon.Companion, 8, 1f, 0f, 0f), scale(1f, 0.64f)), rot(-45f)));
        s.add(custom(List.of(p(0.961f, 0.039f, cr(0.426f)), p(1.001f, 0.428f), p(1.000f, 0.609f, cr(1.000f))), 2, true));
        s.add(mat(RoundedPolygonKt.RoundedPolygon(3, 1f, 0f, 0f, r20, null), rot(-90f)));
        s.add(custom(List.of(p(0.500f, 1.096f, cr(0.151f, 0.524f)), p(0.040f, 0.500f, cr(0.159f))), 2));
        s.add(custom(List.of(p(0.171f, 0.841f, cr(0.159f)), p(-0.020f, 0.500f, cr(0.140f)), p(0.170f, 0.159f, cr(0.159f))), 2));
        s.add(custom(List.of(p(0.500f, -0.009f, cr(0.172f)), p(1.030f, 0.365f, cr(0.164f)), p(0.828f, 0.970f, cr(0.169f))), 1, true));
        s.add(custom(List.of(p(0.499f, 1.023f, cr(0.241f, 0.778f)), p(-0.005f, 0.792f, cr(0.208f)), p(0.073f, 0.258f, cr(0.228f)), p(0.433f, -0.000f, cr(0.491f))), 1, true));
        s.add(star(8, .8f, r15));
        s.add(custom(List.of(p(0.500f, 1.080f, cr(0.085f)), p(0.358f, 0.843f, cr(0.085f))), 8));
        s.add(custom(List.of(p(1.237f, 1.236f, cr(0.258f)), p(0.500f, 0.918f, cr(0.233f))), 4));
        s.add(custom(List.of(p(0.723f, 0.884f, cr(0.394f)), p(0.500f, 1.099f, cr(0.398f))), 6));
        s.add(mat(star(7, .75f, r50), rot(-90f)));
        s.add(mat(star(9, .8f, r50), rot(-90f)));
        s.add(mat(star(12, .8f, r50), rot(-90f)));
        s.add(custom(List.of(p(0.500f, 0f, cr(1.000f)), p(1f, 0f, cr(1.000f)), p(1f, 1.140f, cr(0.254f, 0.106f)), p(0.575f, 0.906f, cr(0.253f))), 1, true));
        s.add(custom(List.of(p(0.500f, 0.074f), p(0.725f, -0.099f, cr(0.476f))), 4, true));
        s.add(custom(List.of(p(0.500f, 0.036f), p(0.758f, -0.101f, cr(0.209f))), 8));
        s.add(custom(List.of(p(0.500f, -0.006f, cr(0.006f)), p(0.592f, 0.158f, cr(0.006f))), 12));
        s.add(custom(List.of(p(0.193f, 0.277f, cr(0.053f)), p(0.176f, 0.055f, cr(0.053f))), 10));
        s.add(custom(List.of(p(0.457f, 0.296f, cr(0.007f)), p(0.500f, -0.051f, cr(0.007f))), 15));
        s.add(custom(List.of(p(0.733f, 0.454f), p(0.839f, 0.437f, cr(0.532f)), p(0.949f, 0.449f, cr(0.439f, 1.000f)), p(0.998f, 0.478f, cr(0.174f))), 16, true));
        s.add(custom(List.of(p(0.370f, 0.187f), p(0.416f, 0.049f, cr(0.381f)), p(0.479f, 0.001f, cr(0.095f))), 8, true));
        s.add(mat(custom(List.of(p(0.500f, 0.053f), p(0.545f, -0.040f, cr(0.405f)), p(0.670f, -0.035f, cr(0.426f)), p(0.717f, 0.066f, cr(0.574f)), p(0.722f, 0.128f), p(0.777f, 0.002f, cr(0.360f)), p(0.914f, 0.149f, cr(0.660f)), p(0.926f, 0.289f, cr(0.660f)), p(0.881f, 0.346f), p(0.940f, 0.344f, cr(0.126f)), p(1.003f, 0.437f, cr(0.255f))), 2, true), scale(1f, 0.742f)));
        s.add(custom(List.of(p(0.870f, 0.130f, cr(0.146f)), p(0.818f, 0.357f), p(1.000f, 0.332f, cr(0.853f))), 4, true));
        s.add(custom(List.of(p(0.500f, 0.000f), p(0.704f, 0.000f), p(0.704f, 0.065f), p(0.843f, 0.065f), p(0.843f, 0.148f), p(0.926f, 0.148f), p(0.926f, 0.296f), p(1.000f, 0.296f)), 2, true));
        s.add(custom(List.of(p(0.110f, 0.500f), p(0.113f, 0.000f), p(0.287f, 0.000f), p(0.287f, 0.087f), p(0.421f, 0.087f), p(0.421f, 0.170f), p(0.560f, 0.170f), p(0.560f, 0.265f), p(0.674f, 0.265f), p(0.675f, 0.344f), p(0.789f, 0.344f), p(0.789f, 0.439f), p(0.888f, 0.439f)), 1, true));
        s.add(custom(List.of(p(0.796f, 0.500f), p(0.853f, 0.518f, cr(1f)), p(0.992f, 0.631f, cr(1f)), p(0.968f, 1.000f, cr(1f))), 2, true));
        s.add(custom(List.of(p(0.500f, 0.268f, cr(0.016f)), p(0.792f, -0.066f, cr(0.958f)), p(1.064f, 0.276f, cr(1.000f)), p(0.501f, 0.946f, cr(0.129f))), 1, true));
        List<RoundedPolygon> n = new ArrayList<>();
        for (RoundedPolygon r : s) n.add(r.normalized());
        return n;
    }

    static void dump(String tag, List<Cubic> cubics) {
        StringBuilder sb = new StringBuilder(tag).append(' ').append(cubics.size());
        for (Cubic c : cubics) {
            for (float v : new float[] {c.getAnchor0X(), c.getAnchor0Y(), c.getControl0X(), c.getControl0Y(), c.getControl1X(), c.getControl1Y(), c.getAnchor1X(), c.getAnchor1Y()}) {
                sb.append(' ').append(String.format(Locale.ROOT, "%.9g", v));
            }
        }
        System.out.println(sb);
    }

    public static void main(String[] a) {
        List<RoundedPolygon> s = shapes();
        for (int i = 0; i < s.size(); i++) dump("shape" + i, s.get(i).getCubics());
        for (int i = 0; i + 1 < s.size(); i++) {
            Morph m = new Morph(s.get(i), s.get(i + 1));
            for (float pr : new float[] {0f, 0.3f, 1f}) dump("morph" + i + "@" + pr, m.asCubics(pr));
        }
        dump("smooth", ShapesKt.rectangle(RoundedPolygon.Companion, 2f, 2f, cr(0.5f, 1f), null, 0f, 0f).getCubics());
        dump("pillstar", ShapesKt.pillStar(RoundedPolygon.Companion, 2f, 1f, 8, 0.5f, cr(0.1f), null, null, 0.5f, 0f, 0f, 0f).getCubics());
        dump("pill", ShapesKt.pill(RoundedPolygon.Companion, 3f, 1f, 0.5f, 0f, 0f).getCubics());
    }
}
