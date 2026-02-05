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

void main(void)
{
  // Get the texture colour
  vec4 col = texture2D(uTexUnit, vTexCoords);

  // Calculate the luminance
  col.a = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;

  // Clip any possible rouding issues
  if (col.a > 1.0)  
    col.a = 1.0;

  // Clip any possible rounding issues
  if (col.a < 0.0)
    col.a = 0.0;

  gl_FragColor = col;
}

