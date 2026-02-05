#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

// ----------------------------------------------------------------------------
// Alpha fragment shader
// Uses solid colour that is modulated using the alpha values in the current texture

uniform sampler2D uTexUnit;
varying vec2      vTexCoords;
uniform vec4      uColour;

void main(void)
{
  vec4 col = uColour;
  col.a = texture2D(uTexUnit, vTexCoords).a;

  gl_FragColor = col;
}

