uniform float uVertexTemp;
uniform float uTime;

attribute vec2 aPosition;
attribute vec3 aColor;

varying vec3 vColor;

void main() {
	if (uVertexTemp == 1.) {
		aPosition.x += 0.5;
  }
  gl_Position = vec4(aPosition.x, aPosition.y, 0, 1);
  vColor = aColor;
}
