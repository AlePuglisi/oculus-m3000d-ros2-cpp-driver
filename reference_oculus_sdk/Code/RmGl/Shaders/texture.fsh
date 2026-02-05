#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

// ----------------------------------------------------------------------------
// Uses texture colour, vTexCoords is set by corresponding texture vertex shader

uniform sampler2D uTexUnit;
varying vec2      vTexCoords;

void main(void)
{
  gl_FragColor = texture2D(uTexUnit, vTexCoords);
}

