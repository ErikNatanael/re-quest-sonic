#version 330
precision highp float;


uniform sampler2DRect tex0;
uniform float amount;
uniform vec2 resolution;

in vec2 texCoordVarying;

out vec4 outputColor;


float luma(vec3 color) {
  return dot(color, vec3(0.299, 0.587, 0.114));
}

void main()
{
	vec2 st = gl_FragCoord.xy;
  // st.y = resolution.y - st.y; // flip y
	// vec3 color = vec3(amount) -  texture(tex0, st).rgb;// * vec4(1., 0., 0. ,1.);
  vec4 color = texture(tex0, st).rgba;

  outputColor = color;
}
