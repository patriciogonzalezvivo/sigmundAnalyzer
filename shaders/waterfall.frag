#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution;
uniform vec2        u_resolution;

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

    color += texture2D(u_tex0,  st).rgb;

    gl_FragColor = vec4(color,1.0);
}
