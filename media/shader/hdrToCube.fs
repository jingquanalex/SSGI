#version 450

in vec3 WorldPos;

out vec4 FragColor;

uniform sampler2D colorMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), -asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(colorMap, uv).rgb;
	color = pow(color, vec3(2.2));
    
    FragColor = vec4(color, 1.0);
}