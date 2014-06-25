precision mediump float;

uniform vec4 u_Color;
uniform sampler2D u_Texture;    // The input texture.
varying vec2 v_TexCoordinate;   // Interpolated texture coordinate per fragment.

void main() {
  // Multiply the color by the diffuse illumination level and texture value to get final output color.
  gl_FragColor = (u_Color * texture2D(u_Texture, v_TexCoordinate));
}