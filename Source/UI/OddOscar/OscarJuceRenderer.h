#pragma once

// Rive C++ runtime — add rive-cpp to Libs/ and link in CMakeLists.txt
#include <rive/renderer.hpp>
#include <rive/math/mat2d.hpp>
#include <rive/math/vec2d.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <stack>

namespace xolokun {

//==============================================================================
// OscarJucePath — implements rive::RenderPath using juce::Path.
//
// Rive calls moveTo / lineTo / cubicTo / close to build each shape.
// We accumulate those into a juce::Path and return it for painting.
//==============================================================================
class OscarJucePath : public rive::RenderPath
{
public:
    void rewind() override                      { path.clear(); }
    void fillRule (rive::FillRule rule) override { fillEvenOdd = (rule == rive::FillRule::evenOdd); }

    void moveTo (float x, float y) override     { path.startNewSubPath (x, y); }
    void lineTo (float x, float y) override     { path.lineTo (x, y); }
    void close()  override                      { path.closeSubPath(); }

    void cubicTo (float ox, float oy,
                  float ix, float iy,
                  float x,  float y) override
    {
        path.cubicTo (ox, oy, ix, iy, x, y);
    }

    void addPath (rive::CommandPath* source, const rive::Mat2D& xform) override
    {
        auto* juceSource = static_cast<OscarJucePath*> (source);
        auto affine = toJuce (xform);
        path.addPath (juceSource->path, affine);
    }

    const juce::Path& getPath() const   { return path; }
    bool   isEvenOdd()          const   { return fillEvenOdd; }

private:
    juce::Path path;
    bool fillEvenOdd = false;

    static juce::AffineTransform toJuce (const rive::Mat2D& m)
    {
        // rive::Mat2D is stored as [m00, m01, m10, m11, tx, ty]
        return { m[0], m[2], m[4],
                 m[1], m[3], m[5] };
    }
};

//==============================================================================
// OscarJucePaint — implements rive::RenderPaint.
//
// Collects fill/stroke style, color, and gradient info from Rive.
// OscarJuceRenderer calls applyTo() when drawing a path.
//==============================================================================
class OscarJucePaint : public rive::RenderPaint
{
public:
    enum class Kind { Solid, LinearGradient, RadialGradient };

    void style (rive::RenderPaintStyle s) override
    {
        isStroke = (s == rive::RenderPaintStyle::stroke);
    }

    void color (unsigned int argb) override
    {
        kind = Kind::Solid;
        juce::uint8 a = (argb >> 24) & 0xFF;
        juce::uint8 r = (argb >> 16) & 0xFF;
        juce::uint8 g = (argb >>  8) & 0xFF;
        juce::uint8 b = (argb >>  0) & 0xFF;
        solidColor = juce::Colour (r, g, b, a);
    }

    void thickness (float t) override          { strokeWidth = t; }
    void join (rive::StrokeJoin j) override    { strokeJoin = j; }
    void cap  (rive::StrokeCap  c) override    { strokeCap  = c; }
    void blendMode (rive::BlendMode) override  { /* normal blend only for now */ }

    void linearGradient (float x0, float y0, float x1, float y1) override
    {
        kind = Kind::LinearGradient;
        gx0 = x0; gy0 = y0; gx1 = x1; gy1 = y1;
        stops.clear();
    }

    void radialGradient (float x0, float y0, float x1, float y1) override
    {
        kind = Kind::RadialGradient;
        gx0 = x0; gy0 = y0; gx1 = x1; gy1 = y1;
        stops.clear();
    }

    void addStop (unsigned int argb, float position) override
    {
        juce::uint8 a = (argb >> 24) & 0xFF;
        juce::uint8 r = (argb >> 16) & 0xFF;
        juce::uint8 g = (argb >>  8) & 0xFF;
        juce::uint8 b = (argb >>  0) & 0xFF;
        stops.push_back ({ juce::Colour (r, g, b, a), position });
    }

    void applyTo (juce::Graphics& gc, const juce::Path& p, bool evenOdd) const
    {
        if (kind == Kind::Solid)
            gc.setColour (solidColor);
        else
            gc.setGradientFill (buildGradient (p));

        if (isStroke)
        {
            auto joinStyle = toJuceJoin (strokeJoin);
            auto capStyle  = toJuceCap  (strokeCap);
            gc.strokePath (p, juce::PathStrokeType (strokeWidth, joinStyle, capStyle));
        }
        else
        {
            if (evenOdd)
                gc.fillPath (p, juce::AffineTransform(), juce::FillType::evenOdd());
            else
                gc.fillPath (p);
        }
    }

private:
    struct GradStop { juce::Colour color; float position; };

    bool isStroke   = false;
    Kind kind       = Kind::Solid;
    juce::Colour solidColor { juce::Colours::white };
    float strokeWidth = 1.0f;
    rive::StrokeJoin strokeJoin = rive::StrokeJoin::miter;
    rive::StrokeCap  strokeCap  = rive::StrokeCap::butt;
    float gx0 = 0, gy0 = 0, gx1 = 0, gy1 = 0;
    std::vector<GradStop> stops;

    juce::ColourGradient buildGradient (const juce::Path& p) const
    {
        bool isRadial = (kind == Kind::RadialGradient);
        juce::ColourGradient grad (
            stops.empty() ? juce::Colours::white : stops.front().color,
            gx0, gy0,
            stops.empty() ? juce::Colours::black : stops.back().color,
            gx1, gy1,
            isRadial);

        for (size_t i = 1; i + 1 < stops.size(); ++i)
            grad.addColour (stops[i].position, stops[i].color);

        return grad;
    }

    static juce::PathStrokeType::JointStyle toJuceJoin (rive::StrokeJoin j)
    {
        switch (j) {
            case rive::StrokeJoin::round: return juce::PathStrokeType::curved;
            case rive::StrokeJoin::bevel: return juce::PathStrokeType::beveled;
            default:                      return juce::PathStrokeType::mitered;
        }
    }

    static juce::PathStrokeType::EndCapStyle toJuceCap (rive::StrokeCap c)
    {
        switch (c) {
            case rive::StrokeCap::round:  return juce::PathStrokeType::rounded;
            case rive::StrokeCap::square: return juce::PathStrokeType::square;
            default:                      return juce::PathStrokeType::butt;
        }
    }
};

//==============================================================================
// OscarJuceRenderer — implements rive::Renderer against a juce::Graphics
// context. Handles save/restore stack, transform accumulation, and routing
// draw calls to OscarJucePath + OscarJucePaint.
//
// Usage: construct fresh each frame inside paint(), pass to artboard->draw().
//==============================================================================
class OscarJuceRenderer : public rive::Renderer
{
public:
    explicit OscarJuceRenderer (juce::Graphics& g) : gc (g) {}

    //--------------------------------------------------------------------------
    // Transform stack
    //--------------------------------------------------------------------------

    void save() override
    {
        gc.saveState();
        transformStack.push (currentTransform);
    }

    void restore() override
    {
        gc.restoreState();
        if (!transformStack.empty())
        {
            currentTransform = transformStack.top();
            transformStack.pop();
        }
    }

    void transform (const rive::Mat2D& matrix) override
    {
        auto juceXform = toJuce (matrix);
        currentTransform = currentTransform.followedBy (juceXform);
        gc.addTransform (juceXform);
    }

    //--------------------------------------------------------------------------
    // Draw calls
    //--------------------------------------------------------------------------

    void drawPath (rive::RenderPath* renderPath, rive::RenderPaint* renderPaint) override
    {
        auto* juceP = static_cast<OscarJucePath*> (renderPath);
        auto* juceR = static_cast<OscarJucePaint*> (renderPaint);

        juceR->applyTo (gc, juceP->getPath(), juceP->isEvenOdd());
    }

    void clipPath (rive::RenderPath* renderPath) override
    {
        auto* juceP = static_cast<OscarJucePath*> (renderPath);
        gc.reduceClipRegion (juceP->getPath());
    }

    //--------------------------------------------------------------------------
    // Factory
    //--------------------------------------------------------------------------

    rive::RenderPaint* makeRenderPaint() override { return new OscarJucePaint(); }
    rive::RenderPath*  makeRenderPath()  override { return new OscarJucePath();  }

private:
    juce::Graphics& gc;
    juce::AffineTransform currentTransform;
    std::stack<juce::AffineTransform> transformStack;

    static juce::AffineTransform toJuce (const rive::Mat2D& m)
    {
        return { m[0], m[2], m[4],
                 m[1], m[3], m[5] };
    }
};

} // namespace xolokun
