#version 330

#ifdef GL_ES
precision mediump float;
#else
precision highp float;
#endif
uniform sampler2DRect tDiffuse;

uniform float fX;
uniform float fY;
uniform float fExposure;
uniform float fDecay;
uniform float fDensity;
uniform float fWeight;
uniform float fClamp;
uniform vec2 resolution;

const int iSamples = 20;

out vec4 outColor;

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    // vec2 vUv = (gl_FragCoord.xy*2.-resolution.xy)/resolution.y;
    vec2 vUv = gl_FragCoord.xy/resolution;
    vec2 deltaTextCoord = vec2(vUv - vec2(fX, fY));
    deltaTextCoord *= 1.0 /  float(iSamples) * fDensity;
    vec2 coord = vUv;
    float illuminationDecay = 1.0;
    outColor = vec4(0.0);

    for(int i=0; i < iSamples ; i++)
    {
        coord -= deltaTextCoord;
        vec4 texel = texture(tDiffuse, coord * resolution);
        texel *= illuminationDecay * fWeight;

        outColor += texel;

        illuminationDecay *= fDecay;
    }
    outColor *= fExposure;
    outColor = clamp(outColor, 0.0, fClamp);
    //outColor = texture(tDiffuse, vUv*resolution);
}