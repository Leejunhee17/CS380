#version 130

uniform float uVertexScale;
uniform sampler2D uTexUnit0, uTexUnit1;

uniform float uTime;

in vec2 vTexCoord0, vTexCoord1;
in vec3 vColor;

out vec4 fragColor;

void main(void) {

  vec3 nColor;
  nColor.x = sin(uTime / 500.);
  nColor.y = cos(uTime / 500.);
  nColor.z = 1 - sin(uTime / 500.);

  vec4 color = vec4(nColor.x, nColor.y, nColor.z, 1);
  vec4 texColor0 = texture(uTexUnit0, vTexCoord0);
  vec4 texColor1 = texture(uTexUnit1, vTexCoord1);

  float lerper = clamp(.5 * uVertexScale, 0., 1.);
  float lerper2 = clamp(.5 * uVertexScale + 1., 0., 1.);

  fragColor = ((lerper)*texColor1 + (1.0-lerper)*texColor0) * lerper2 + color * (1.0-lerper2);
}
