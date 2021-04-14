#version 110

uniform sampler2D ymap;
uniform sampler2D umap;
uniform sampler2D vmap;
varying vec2 tex_position;

void main() {
  float y = texture2D(ymap, tex_position).r;
  float u = texture2D(umap, tex_position).r - .5;
  float v = texture2D(vmap, tex_position).r - .5;
  float r = y + 1.28033 * v;
  float g = y - .21482 * u - .38059 * v;
  float b = y + 2.12798 * u;
  gl_FragColor = vec4(r, g, b, 1.0);
}