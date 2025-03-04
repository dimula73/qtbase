#version 440

layout(location = 0) in vec2 v_texcoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 vertexTransform;
    mat3 textureTransform;
    float opacity;
    int textureSwizzle;
};

layout(binding = 1) uniform sampler2D textureSampler;

#if defined SCRGB_TO_SRGB
vec4 linearToSRGB(vec4 value)
{
   bvec4 cutoff = lessThan(value, vec4(0.0031308));
   const vec2 a1 = vec2(0.055, 0.0);
   const vec2 c2 = vec2(1.055, 1.0);
   const vec2 m3 = vec2(2.4, 1.0);
   const vec2 c4 = vec2(12.92, 1.0);
   vec4 higher = c2.xxxy * pow(value, 1.0 / m3.xxxy) - a1.xxxy;
   vec4 lower = value * c4.xxxy;
   return mix(higher, lower, vec4(cutoff));
}
#endif

#if defined SRGB_TO_SCRGB || defined SRGB_TO_BT2020PQ
vec4 sRgbToLinear(vec4 sRGB)
{
   bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
   const vec2 a1 = vec2(0.055, 0.0);
   const vec2 c2 = vec2(1.055, 1.0);
   const vec2 m3 = vec2(2.4, 1.0);
   const vec2 c4 = vec2(12.92, 1.0);
   vec4 higher = pow((sRGB + a1.xxxy) / c2.xxxy, m3.xxxy);
   vec4 lower = sRGB / c4.xxxy;
   return mix(higher, lower, vec4(cutoff));
}
#endif

#if defined SRGB_TO_BT2020PQ || defined SCRGB_TO_BT2020PQ
vec4 applySmpte2084Curve(vec4 L)
{
   float oldAlpha = L.a;
   const vec4 m1 = vec4(2610.0 / 4096.0 / 4.0);
   const vec4 m2 = vec4(2523.0 / 4096.0 * 128.0);
   const vec4 a1 = vec4(3424.0 / 4096.0);
   const vec4 c2 = vec4(2413.0 / 4096.0 * 32.0);
   const vec4 c3 = vec4(2392.0 / 4096.0 * 32.0);
   const vec4 a4 = vec4(1.0);
   vec4 Lp = pow(L, m1);
   vec4 res = pow((a1 + c2 * Lp) / (a4 + c3 * Lp), m2);
   res.a = oldAlpha;
   return res;
}
#endif

#if defined SRGB_TO_BT2020PQ || defined SCRGB_TO_BT2020PQ
vec4 scRgbToBt2020pq(vec4 value)
{
   const mat4 convMat =
      mat4(0.627402, 0.069095, 0.016394, 0.0,
           0.329292, 0.919544, 0.088028, 0.0,
           0.043306, 0.011360, 0.895578, 0.0,
           0.0,      0.0,      0.0,      1.0);

   value = convMat * value;
   const vec4 whitePointScale = vec4(0.008, 0.008, 0.008, 1.0);
   return applySmpte2084Curve(whitePointScale * value);
}
#endif

#if defined SRGB_TO_BT2020PQ
vec4 sRgbToBt2020pq(vec4 value)
{
   value = sRgbToLinear(value);
   return scRgbToBt2020pq(value);
}
#endif

void main()
{
    vec4 tmpFragColor = texture(textureSampler, v_texcoord);
    tmpFragColor.a *= opacity;
    if (textureSwizzle == 2)
        tmpFragColor.argb = tmpFragColor;
    else if (textureSwizzle == 1)
        tmpFragColor.bgra = tmpFragColor;
#if defined SRGB_TO_SCRGB
    tmpFragColor = sRgbToLinear(tmpFragColor);
#elif defined SRGB_TO_BT2020PQ
    tmpFragColor = sRgbToBt2020pq(tmpFragColor);
#elif defined SCRGB_TO_BT2020PQ
    tmpFragColor = scRgbToBt2020pq(tmpFragColor);
#elif defined SCRGB_TO_SRGB
    tmpFragColor = linearToSRGB(tmpFragColor);
#endif
    fragColor = tmpFragColor;
}
