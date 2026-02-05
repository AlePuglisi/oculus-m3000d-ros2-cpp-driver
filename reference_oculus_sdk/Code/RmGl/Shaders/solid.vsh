#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

// ----------------------------------------------------------------------------
// Solid vertex shader
// Simple position

uniform   mat4  uMatrix;
uniform   float uOriginX;
uniform   float uOriginY;
attribute vec4  aPosition;

void main(void)
{
  // remove the origin from the value
  vec4 pos = aPosition;
  pos.x -= uOriginX;
  pos.y -= uOriginY;

  gl_PointSize = 5.0;
  gl_Position = uMatrix * pos;

}

