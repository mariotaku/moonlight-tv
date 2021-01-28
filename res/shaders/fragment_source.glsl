#version 110
uniform lowp sampler2D ymap;
uniform lowp sampler2D umap;
uniform lowp sampler2D vmap;
varying mediump vec2 tex_position;

void main() {
  mediump float y = texture2D(ymap, tex_position).r;
  mediump float u = texture2D(umap, tex_position).r - .5;
  mediump float v = texture2D(vmap, tex_position).r - .5;
  lowp float r = y + 1.28033 * v;
  lowp float g = y - .21482 * u - .38059 * v;
  lowp float b = y + 2.12798 * u;
  gl_FragColor = vec4(r, g, b, 1.0);
}
