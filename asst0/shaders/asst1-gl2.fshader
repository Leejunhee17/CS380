uniform float uColorTemp;
uniform float uTime;

varying vec3 vColor;

void main(void) {
  if(uColorTemp == 1.){
	vColor.x = 1.;
	vColor.y = 0.0;
	vColor.z = 0.0;
  }
  else if(uColorTemp == 2.){
	vColor.x = 0.0;
	vColor.y = 1.;
	vColor.z = 0.0;
  }
  else if(uColorTemp == 3.){
	vColor.x = 0.0;
	vColor.y = 0.0;
	vColor.z = 1.0;
  }
  else {
	vColor.x = sin(uTime / 500.);
	vColor.y = cos(uTime / 500.);
	vColor.z = 1 - sin(uTime / 500.);
  }

  vec4 color = vec4(vColor.x, vColor.y, vColor.z, 1);
  gl_FragColor = color;
}
