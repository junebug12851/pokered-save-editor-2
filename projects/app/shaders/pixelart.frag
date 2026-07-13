// Anti-aliased point sampling -- how you zoom pixel art to a FRACTIONAL scale without ruining it.
//
// The problem: at, say, 2.37x, plain nearest-neighbour makes some source pixels land on 2 screen
// pixels and some on 3. The art visibly ripples, and it crawls as you zoom. Plain bilinear is worse:
// the whole map goes soft, which is the one thing a Game Boy map must never do.
//
// The fix (the standard "texel anti-aliasing" / sharp-bilinear formulation): sample with bilinear,
// but first pull the texture coordinate almost all the way onto the nearest texel CENTRE, leaving a
// ramp exactly ONE SCREEN PIXEL wide at the boundary between two texels. `fwidth` is what tells us
// how wide a screen pixel is in texture space, so the ramp is always exactly one pixel no matter the
// zoom.
//
// The result: inside a texel it is perfectly flat (crisp, like nearest); across the seam it is a
// single-pixel blend (smooth, like bilinear). No ripple, no crawl, no blur.
//
// At an INTEGER zoom the ramp lands on the seam and this is pixel-for-pixel identical to nearest --
// so nothing is lost at the zoom levels that were exact before.

#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 sourceSize;   // the source image, in texels
};

layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 texel = qt_TexCoord0 * sourceSize;

    // The seam between texels nearest to us.
    vec2 seam = floor(texel + 0.5);

    // How big one screen pixel is, measured in texels. At 4x zoom this is 0.25; at 0.5x it is 2.
    vec2 pixelWidth = fwidth(texel);

    // Snap onto the texel centre, but keep a one-screen-pixel ramp across the seam. The clamp is
    // what makes it flat everywhere else.
    //
    // max() guards the degenerate case (a fully minified or zero-area fragment), where pixelWidth
    // can be 0 and the divide would blow up.
    vec2 snapped = seam + clamp((texel - seam) / max(pixelWidth, vec2(1e-6)), -0.5, 0.5);

    fragColor = texture(source, snapped / sourceSize) * qt_Opacity;
}
