#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif


// ----------------------------------------------------------------------------
// Luminance fragment shader
// Uses texture colour the aplha value is set to the luminace of the colour

uniform sampler2D uTexUnit;
varying vec2      vTexCoords;
uniform vec4      uColKey;

void main(void)
{
  // Get the texture colour
  vec4 col = texture2D(uTexUnit, vTexCoords);

  if (col == uColKey)
    col.a = 0.0;

  gl_FragColor = col;
}

