#version 130

uniform float uVertexScale;

uniform float uwScale;
uniform float uhScale;
uniform float uAngle;

in vec2 aPosition;
in vec3 aColor;
in vec2 aTexCoord0, aTexCoord1;

out vec3 vColor;
out vec2 vTexCoord0, vTexCoord1;

void main() {
  vec2 nPosition, rPosition;
  rPosition.x = aPosition.x * uVertexScale * cos(uAngle) - aPosition.y * sin(uAngle);
  rPosition.y = aPosition.x * uVertexScale * sin(uAngle) + aPosition.y * cos(uAngle);
  nPosition.x = rPosition.x * uwScale;
  nPosition.y = rPosition.y * uhScale;
  gl_Position = vec4(nPosition.x, nPosition.y, 0,1);
  vColor = aColor;
  vTexCoord0 = aTexCoord0;
  vTexCoord1 = aTexCoord1;
}
