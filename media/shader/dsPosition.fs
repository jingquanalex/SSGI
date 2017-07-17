#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;
layout (location = 2) out vec3 dsOutPosition;
layout (location = 3) out vec3 dsOutNormal;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

uniform float imgWidth = 640;
uniform float imgHeight = 480;

const float fx = tan(radians(61.9999962) / 2) * 2;
const float fy = tan(radians(48.5999985) / 2) * 2;

vec3 dsDepthToWorldPosition(sampler2D samplerDepth, vec2 texcoord)
{
	float normalizedX = 0.5 - texcoord.x;
	float normalizedY = 0.5 - texcoord.y;
	float z = texture(samplerDepth, texcoord).r - 1;
	float x = normalizedX * z * fx;
	float y = normalizedY * z * fy;
	
	return vec3(x, y, z);
}

vec2 neighborOffsets[4] = vec2[]
(
	vec2(1 / imgWidth, 0),
    vec2(-1 / imgWidth, 0),
	vec2(0, 1 / imgHeight),
	vec2(0, -1 / imgHeight)
);

void main()
{
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	
	// Generate position from depth
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, TexCoord);
	
	// Generate sensor normals
	//vec3 dsnormal = normalize(cross(dFdx(dsposition), dFdy(dsposition)));
	float oDist = 1;
	vec3 dx = (dsDepthToWorldPosition(dsDepth, TexCoord + neighborOffsets[0] * oDist) - dsDepthToWorldPosition(dsDepth, TexCoord + neighborOffsets[1] * oDist)) / 2;
	vec3 dy = (dsDepthToWorldPosition(dsDepth, TexCoord + neighborOffsets[2] * oDist) - dsDepthToWorldPosition(dsDepth, TexCoord + neighborOffsets[3] * oDist)) / 2;
	vec3 dsnormal = normalize(cross(dx, dy));
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
	dsOutPosition = dsposition;
	dsOutNormal = dsnormal;
}