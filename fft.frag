#ifdef GL_ES
precision mediump float;
#endif

uniform vec2        u_resolution;

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;

#include "glslLib/color/space/all.glsl"

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec2 pixel = 1.0/u_resolution.xy;

    float press = 256.0;
    vec2 st_i = floor(st * press) / press;


    vec3 hue = texture2D(u_tex0,  pixel * 0.5 + vec2(st_i.x, 0.0)).rgb;
    float value = rgb2depth(hue);

    color += step(st.y, value) * 0.75;
    color += texture2D(u_tex0,  st).rgb;

    gl_FragColor = vec4(color,1.0);
}
