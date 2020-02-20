#version 330

#ifdef GL_ES
precision mediump float;
#else
precision highp float;
#endif

uniform vec2 resolution;
uniform vec2 mouse;
uniform float time;
uniform sampler2DRect tex0;

out vec4 outColor;

// From Book of Shaders https://thebookofshaders.com/edit.php#11/3d-noise.frag START
float random (in float x) {
    return fract(sin(x)*1e4);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec3 p) {
    const vec3 step = vec3(110.0, 241.0, 171.0);

    vec3 i = floor(p);
    vec3 f = fract(p);

    // For performance, compute the base input to a
    // 1D random from the integer part of the
    // argument and the incremental change to the
    // 1D based on the 3D -> 1D wrapping
    float n = dot(i, step);

    vec3 u = f * f * (3.0 - 2.0 * f);
    return mix( mix(mix(random(n + dot(step, vec3(0,0,0))),
                        random(n + dot(step, vec3(1,0,0))),
                        u.x),
                    mix(random(n + dot(step, vec3(0,1,0))),
                        random(n + dot(step, vec3(1,1,0))),
                        u.x),
                u.y),
                mix(mix(random(n + dot(step, vec3(0,0,1))),
                        random(n + dot(step, vec3(1,0,1))),
                        u.x),
                    mix(random(n + dot(step, vec3(0,1,1))),
                        random(n + dot(step, vec3(1,1,1))),
                        u.x),
                u.y),
            u.z);
}
// From Book of Shaders https://thebookofshaders.com/edit.php#11/3d-noise.frag END

float sum(vec4 v) {
  return v.x + v.y + v.z;
}

void main(){
  vec2 co = gl_FragCoord.xy;
	vec2 st = gl_FragCoord.xy/resolution;
  vec4 c = texture(tex0, co).rgba;

  //st*=5;
  st *= sin(time*0.1)*3+4;
  vec2 offset = vec2(noise(vec3(st+time, time)), noise(vec3(st+10.+time, time)));
  offset = round(offset*4-2);
  vec4 nc = texture(tex0, co+offset).rgba;
  //c = vec4(c.rgb*.8, c.a) + vec4(nc.rgb*.2, nc.a); // spreading black fields

  // slowly growing color fields
  //if(sum(nc) > sum(c)) {
  //  c = c*.5 + nc*.5;
  //}

  // slowly growing color fields
  if(sum(nc) > sum(c)) {
    c = vec4(c.rgb*.2, c.a*.9) + vec4(nc.rgb*.8, 0.1);
  } else {
    c *= 0.99;
  }
  c.a = min(c.a, 1.);
  //c *= 0.9;

  // dark colors get less alpha
  c.a *= smoothstep(0.1, .5, sum(c));

	outColor = vec4(c);
}
