uniform mat4 u_MVP;
attribute vec4 a_Position;
attribute vec2 a_TexCoordinate; // Per-vertex texture coordinate information we will pass in.
varying vec2 v_TexCoordinate;   // This will be passed into the fragment shader.

void main() {
  gl_Position = u_MVP * a_Position;

  // Pass through the texture coordinate.
  v_TexCoordinate = a_TexCoordinate;
}