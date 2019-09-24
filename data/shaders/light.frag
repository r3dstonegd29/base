#version 440
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;
in vec3 vPosition;
in vec2 vUv;

layout (location = 0) uniform sampler2D diffuse_txt;
layout (location = 1) uniform sampler2D position_txt;
layout (location = 2) uniform sampler2D normal_txt;
uniform vec3 l_pos;
uniform vec3 la;
uniform vec3 ld;
uniform vec3 ls;
uniform vec3 att_factor;
uniform int window_width;
uniform int window_height;
subroutine void Render_Type();
subroutine uniform Render_Type render_pass;

layout (location = 0) out vec4 out_color;



subroutine (Render_Type) void render_diffuse_specular()
{
	vec2 new_uvs = vec2(gl_FragCoord.x/window_width, gl_FragCoord.y/window_height);
	
	vec4 diffuse_value = texture(diffuse_txt, new_uvs);
	vec4 position_value = texture(position_txt, new_uvs);
	vec4 normal_value = texture(normal_txt, new_uvs);

	vec3 kd = diffuse_value.rgb;
	float ks = diffuse_value.a-1.0;
	vec3 frag_pos = position_value.rgb;
	float ka = position_value.a-1.0;
	vec3 normal_v = normalize(normal_value.rgb);
	float ns = normal_value.a-1.0;

	vec3 light_v = normalize(l_pos - frag_pos);
	vec3 view_v = normalize(-frag_pos);
	vec3 half_v = normalize(view_v + light_v);

	float d = length(l_pos - frag_pos);
	float att = min(1/(att_factor.x + att_factor.y*d + att_factor.z*d*d) , 1.0);

	float id = att * max(dot(normal_v, light_v), 0.0);
	float is = att * pow(max(dot(normal_v, half_v), 0.0),ns);

	out_color = vec4((id*ld+ka*la)*kd + ls * ks * is, 1.0);
}

subroutine (Render_Type) void render_ambient()
{
	vec2 new_uvs = vec2(gl_FragCoord.x/window_width, gl_FragCoord.y/window_height);
	
	vec4 diffuse_value = texture(diffuse_txt, new_uvs);
	vec4 position_value = texture(position_txt, new_uvs);
	
	vec3 kd = diffuse_value.rgb;
	float ka = position_value.a-1.0;

	out_color = vec4((ka*la)*kd, 1.0);
}

void main()
{
	render_pass();
}