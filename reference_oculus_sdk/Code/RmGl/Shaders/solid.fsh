#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

// ----------------------------------------------------------------------------
// Solid fragment shader
// Uses solid colour

uniform vec4 uColour;

void main(void)
{
  gl_FragColor = uColour;
}

