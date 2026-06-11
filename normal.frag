#version 100

precision mediump float;
varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;

void main() {
    vec4 color = texture2D(texture0, fragTexCoord);
    if (color.a < 0.1) discard;
    gl_FragColor = color;
}
