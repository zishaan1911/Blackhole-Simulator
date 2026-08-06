#version 330 core
// Tone maps accumulated linear radiance to the screen.
out vec4 FragColour;

uniform sampler2D uImage;
uniform vec2      uResolution;   // window resolution
uniform float     uExposure;

vec3 acesFilm(vec3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 c = texture(uImage, gl_FragCoord.xy / uResolution).rgb;
    c = acesFilm(c * uExposure);
    c = pow(c, vec3(1.0 / 2.2));
    FragColour = vec4(c, 1.0);
}
