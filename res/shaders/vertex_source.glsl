#version 110
attribute vec2 position;
varying mediump vec2 tex_position;

void main() {
  gl_Position = vec4(position, 0, 1);
  tex_position = vec2((position.x + 1.) / 2., (1. - position.y) / 2.);
}
