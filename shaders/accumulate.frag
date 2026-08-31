#version 330 core
// Running average of linear radiance across frames.
//
// uSampleIndex is the number of samples ALREADY in uHistory. Weighting the new
// sample by 1/(n+1) makes this an exact running mean rather than an
// exponential fade, so a still camera converges instead of hovering.
out vec4 FragColour;

uniform sampler2D uHistory;
uniform sampler2D uCurrent;
uniform float     uSampleIndex;

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    vec3 current = texelFetch(uCurrent, p, 0).rgb;

    if (uSampleIndex < 0.5) {
        FragColour = vec4(current, 1.0);
        return;
    }

    vec3 history = texelFetch(uHistory, p, 0).rgb;
    float w = 1.0 / (uSampleIndex + 1.0);
    FragColour = vec4(mix(history, current, w), 1.0);
}
