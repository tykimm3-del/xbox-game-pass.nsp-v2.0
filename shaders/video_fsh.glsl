#version 460

// H.264 NV12 -> RGB. plane0 is the R8 luma image, plane1 is the RG8 chroma
// image (U in .r, V in .g), both zero-copy views of the NVTEGRA decoder
// surface. uv_data letterboxes the frame into the screen viewport.
layout (location = 0) in vec2 vTextureCoord;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D plane0;
layout (binding = 1) uniform sampler2D plane1;

layout (std140, binding = 0) uniform Transformation
{
    mat3 yuvmat;
    vec3 offset;
    vec4 uv_data;
    vec4 sharp_data;  // x=strength, y=overshoot, zw=one luma texel in UV
} u;

void main()
{
    vec2 uv = (vTextureCoord - u.uv_data.xy) * u.uv_data.zw;

    // The decoder surface is taller than the picture (32-row alignment:
    // 1080 visible in 1088, 720 in 736) and the rows below the image are
    // uninitialized padding. Clamp each plane's sample to half a texel
    // inside the visible region, or bilinear filtering on the last screen
    // line blends real chroma with padding garbage -- a green shimmer on
    // the bottom edge. The top/left need nothing: the image starts at 0.
    // The texel size comes from the CPU side: uam's textureSize() returns
    // junk on hardware and scrambled the whole picture (issue #40). The
    // chroma texel is exactly twice the luma texel (NV12 half-res planes).
    vec2 px = u.sharp_data.zw;
    vec2 lmax = u.uv_data.zw - 0.5 * px;
    vec2 cmax = u.uv_data.zw - px;
    vec2 luv = min(uv, lmax);

    // Luma-only unsharp mask (chroma untouched, so no color fringing). The
    // base stream is soft -- H.264 at streaming bitrates smears fine detail.
    // sharp_data.x is the strength; .y bounds the overshoot: the result may
    // exceed the local neighborhood min/max only by that allowance, which is
    // what keeps hard edges from growing halos.
    float y = texture(plane0, luv).r;
    if (u.sharp_data.x > 0.0) {
        float yl = texture(plane0, min(luv - vec2(px.x, 0.0), lmax)).r;
        float yr = texture(plane0, min(luv + vec2(px.x, 0.0), lmax)).r;
        float yu = texture(plane0, min(luv - vec2(0.0, px.y), lmax)).r;
        float yd = texture(plane0, min(luv + vec2(0.0, px.y), lmax)).r;
        float average = 0.25 * (yl + yr + yu + yd);
        float lo = min(y, min(min(yl, yr), min(yu, yd)));
        float hi = max(y, max(max(yl, yr), max(yu, yd)));
        y = clamp(y + (y - average) * u.sharp_data.x,
                  max(0.0, lo - u.sharp_data.y),
                  min(1.0, hi + u.sharp_data.y));
    }

    vec2 cuv = min(uv, cmax);
    vec3 yuv = vec3(y,
                    texture(plane1, cuv).r,
                    texture(plane1, cuv).g) - u.offset;
    vec3 rgb = u.yuvmat * yuv;

    outColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
