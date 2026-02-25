#pragma once

struct ColorRGBA {
    float r, g, b, a;

    ColorRGBA() : r(0), g(0), b(0), a(1) {}
    ColorRGBA(float r, float g, float b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {
    }
};

enum class DiffPatternType {
    AbsDiff,
    ColorMask,
    OverlayTemplate
};

struct DiffPattern {
    DiffPatternType type = DiffPatternType::ColorMask;
    ColorRGBA maskColor = ColorRGBA(0.0f, 1.0f, 0.0f, 1.0f);
};
