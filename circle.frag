precision mediump float;

uniform int circles_n;
uniform float acceleration, t;
uniform int xs[CIRCLES_MAX], ys[CIRCLES_MAX];
uniform float start_times[CIRCLES_MAX];

void main() {
    vec4 color = vec4(1);
    for (int i = 0; i < CIRCLES_MAX; i++) {
        if (i >= circles_n) break;
        vec2 delta = gl_FragCoord.xy - vec2(xs[i], ys[i]);
        float local_time = t - start_times[i];
        float r = acceleration * local_time * local_time / 2.;
        if (dot(delta, delta) <= r * r) {
            color = vec4(vec3(1) - color.rgb, 1);
        }
    }
    gl_FragColor = color;
}
