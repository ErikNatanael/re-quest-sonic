#version 330
precision highp float;


uniform sampler2DRect tex0;
uniform vec2 imgRes;
uniform vec2 resMult;
uniform vec2 resolution;
uniform float alphaCurve;
uniform float brightness;
uniform float time;

in vec2 texCoordVarying;

out vec4 outputColor;

//	Simplex 3D Noise 
//	by Ian McEwan, Ashima Arts
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}

float snoise(vec3 v){ 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //  x0 = x0 - 0. + 0.0 * C 
  vec3 x1 = x0 - i1 + 1.0 * C.xxx;
  vec3 x2 = x0 - i2 + 2.0 * C.xxx;
  vec3 x3 = x0 - 1. + 3.0 * C.xxx;

// Permutations
  i = mod(i, 289.0 ); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients
// ( N*N points uniformly over a square, mapped onto an octahedron.)
  float n_ = 1.0/7.0; // N=7
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z *ns.z);  //  mod(p,N*N)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}

float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

#define NUM_OCTAVES 5

float fbm ( in vec2 _st, in float depth) {
    float v = 0.0;
    float a = 0.25;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));
    v = 0.5 * snoise(vec3(_st, depth));
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_st);
        _st = rot * _st * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main()
{
	vec2 uv = (gl_FragCoord.xy*2.0-resolution.xy)/resolution.y;
	vec2 nc = (gl_FragCoord.xy/resolution.xy);
	vec2 st = gl_FragCoord.xy;
	st *= resMult;
	// st.y =  imgRes.y - st.y; // inverts the coordinates, not necessary if drawing to an FBO
	// st.x = imgRes.x - st.x; // invert x coordinates for mirror effect
	st.x = imgRes.x * smoothstep(0, 1, nc.x*1.8);

	// apply fbm to texture coordinates
	// float fbmEffect = resolution.x * 0.002;
	// st += vec2(
	// 	fbm(vec2(st.x*0.1, time * 1.1)) * fbmEffect - (fbmEffect*.5),
	// 	fbm(vec2(st.y*0.1, time * 1.3)) * fbmEffect - (fbmEffect*.5));
	// fbmEffect = resolution.x * 0.005;
	// st += vec2(
	// 	fbm(vec2(st.x*0.01, time * 0.1)) * fbmEffect - (fbmEffect*.5),
	// 	fbm(vec2(st.y*0.01, time * 0.3)) * fbmEffect - (fbmEffect*.5));
	
	vec3 normalPixel =  texture(tex0, st).rgb;
	
	float t = time * 0.7;
	vec2 q = vec2(0.);
  q.x = fbm( uv , t*2.2);
  q.y = fbm( uv*4. + vec2(1.0), t*3.5);
  
  vec2 r = vec2(0.);
  r.x = fbm( uv + q + vec2(1.7,9.2), 0.55 * t);
  r.y = fbm( uv + q + vec2(8.3,2.8), 0.426 * t);
	// st += r*resolution*0.003;
	st += r*resolution*0.01;
	vec3 warpedPixel1 =  texture(tex0, st).rgb;// * vec4(1., 0., 0. ,1.);
	st += r*resolution*0.1;
	vec3 warpedPixel2 =  texture(tex0, st).rgb;// * vec4(1., 0., 0. ,1.);
	// vec3 color = normalPixel*.5 + warpedPixel1*.25 + warpedPixel2*.15;
  vec3 color = normalPixel;
  // fade to white at the left side
  color += smoothstep(0.9, 1.0, nc.x*1.8);
  color += smoothstep(0.67, 1.0, nc.x*1.8)*0.3;
	

	// use the mask for alpha
  // color = vec3(1., 1., 1.) - color; // inverse colours
  
	// color = vec3(r.x, r.y, q.x) - color;
	// color = (vec3(noise(fract(nc*100)*40)*0.5+.5) - color);

  outputColor = vec4(color, 1.0);
}
