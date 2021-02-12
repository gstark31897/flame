#version 460 core
layout(location = 0) in vec3 pos;

out vec4 col;

uniform float maxValue;

float handleLog(float x, float b)
{
	return log(max(x, 0.0)) / log(max(b, 0.0));
}

float scale(float base, float value, float max)
{
	return handleLog(value, base) / handleLog(max, base);
}

vec3 rgb2hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 hsv(float h)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(h + K.xyz) * 6.0 - K.www);
	return p;
}

void main()
{
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	col = vec4(1.0, 1.0, 1.0, 0.5);
	col.rgb = hsv2rgb(vec3(pos.z, 1.0, 1.0));
	col.a = 0.2;
}
