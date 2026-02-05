#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform sampler2D uTexUnit;
uniform sampler2D uPalUnit;
varying vec2      vTexCoords;
uniform float     uPalIndex;

void main(void)
{
  float i = texture2D(uTexUnit, vTexCoords).a;
  gl_FragColor = texture2D(uPalUnit, vec2(i, uPalIndex));
}

