#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

// ----------------------------------------------------------------------------
// Texture vertex shader
// Simple position
// Texture coordinates are passed from the attribute array into vTexCoords for use
// in the fragment shaders

uniform   mat4 uMatrix;
uniform   float uOriginX;
uniform   float uOriginY;

attribute vec4 aPosition;
attribute vec2 aTexCoords;
varying   vec2 vTexCoords;

void main(void)
{
  // Copy over the texture coordintes
  vTexCoords = aTexCoords;

  // remove the origin from the value
  vec4 pos = aPosition;
  pos.x -= uOriginX;
  pos.y -= uOriginY;

  gl_Position = uMatrix * pos;
}

