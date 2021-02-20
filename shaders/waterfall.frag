#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform vec2        u_resolution;

#include "hue2float.glsl"

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

    vec3 sample = texture2D(u_tex0,  st).rgb;
    // color += sample;
    color += hue2float(sample);

    gl_FragColor = vec4(color,1.0);
}
