#version 450

in vec3 WorldPos;

out vec4 FragColor;

uniform sampler2D colorMap;
uniform int recapture = 0;

void main()
{		
    vec3 dir = normalize(WorldPos);
	vec2 uv = dir.xy;
	
	uv.x *= 1080 / 1920.0;
	uv = uv * 0.5 + 0.5;
	if (recapture > 0) uv.y = 1 - uv.y;
	
    vec3 color = texture(colorMap, uv).rgb;
	if (recapture > 0) color = pow(color, vec3(2.2));
    
    FragColor = vec4(color, 1.0);
}