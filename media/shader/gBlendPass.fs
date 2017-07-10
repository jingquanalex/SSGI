#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

uniform sampler2D gInPosition;
uniform sampler2D gInNormal;
uniform sampler2D gInColor;
uniform sampler2D dsColor;
uniform sampler2D dsDepth;

// MEDIAN FILTER //
// adapted: https://github.com/ryocchin/QtShaderExample/blob/master/shaders/median3.frag

#define small2(a,b) tmp=a; a=min(a,b); b=max(tmp,b);
#define small3(a,b,c) small2(a,b); small2(a,c);
#define small4(a,b,c,d) small3(a,b,c); small2(a,d);
#define small5(a,b,c,d,e) small4(a,b,c,d); small2(a,e);
#define small6(a,b,c,d,e,f) small5(a,b,c,d,e); small2(a,f);
#define small7(a,b,c,d,e,f,g) small6(a,b,c,d,e,f); small2(a,g);
#define small8(a,b,c,d,e,f,g,h) small7(a,b,c,d,e,f,g); small2(a,h);
#define small9(a,b,c,d,e,f,g,h,i) small8(a,b,c,d,e,f,g,h); small2(a,i);
#define small10(a,b,c,d,e,f,g,h,i,j) small9(a,b,c,d,e,f,g,h,i); small2(a,j);
#define small11(a,b,c,d,e,f,g,h,i,j,k) small9(a,b,c,d,e,f,g,h,i,j); small2(a,k);


const float imgWidth = 640;
const float imgHeight = 480;
const float cx = imgWidth / 2;
const float cy = imgHeight / 2;
const float fx = tan(radians(61.9999962) / 2) * 2;
const float fy = tan(radians(48.5999985) / 2) * 2;

vec3 dsDepthToWorldPosition(sampler2D samplerDepth, vec2 texcoord)
{
	float z = texture(samplerDepth, texcoord).r * 5;
	float x = (texcoord.x * imgWidth - cx) * z * fx / 440;
	float y = (cy - texcoord.y * imgHeight) * z * fy / 440;
	z -= 7;
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
	vec3 position = texture(gInPosition, TexCoord).xyz;
    vec3 normal = texture(gInNormal, TexCoord).xyz;
    vec4 color = texture(gInColor, TexCoord);
	
	// Depth sensor textures
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	float dsdepth = texture(dsDepth, dsTexCoord).r;
	
	// Reconstructed position from depth (kinect)
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, dsTexCoord);
	
	// Test: Fill hole in depth buffer - pick nearest adjacent neighbor
	if (dsdepth < 0.00001)
	{
		int kernelRadius = 25;
		vec3 nearestPos = vec3(0, 0, 0);
		for (int i = 1; i < kernelRadius; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				vec2 offset = i * neighborOffsets[j];
				float offsetDepth = texture(dsDepth, dsTexCoord + offset).r;
				
				if (offsetDepth > 0.001)
				{
					nearestPos = dsDepthToWorldPosition(dsDepth, dsTexCoord + offset);
					break;
				}
			}
			
			if (nearestPos != vec3(0, 0, 0)) break;
		}
		dsposition = nearestPos;
	}
	
	// 3x3 Median filter kinect position
	/*int i, j, k;
	int kernelRadius = 1;
	int center = kernelRadius * 2 + 1 + kernelRadius;
	vec3 c[25];
	vec3 tmp;

	k = 0;
	for( i = -kernelRadius ; i <= kernelRadius ; i++ )
	{
		for( j = -kernelRadius ; j <= kernelRadius ; j++ )
		{
			vec2 offset = vec2( i / float( imgWidth ), j / float( imgHeight ) );
			c[k++] = dsDepthToWorldPosition(dsDepth, dsTexCoord + offset);
		}
	}

	// 3X3 kernel
	small9( c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8] );
	small8( c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8] );
	small7( c[2], c[3], c[4], c[5], c[6], c[7], c[8] );
	small6( c[3], c[4], c[5], c[6], c[7], c[8] );
	small5( c[4], c[5], c[6], c[7], c[8] );

	dsposition = c[center];*/
	
	
	vec3 mixPosition = dsposition;
	vec4 mixColor = dscolor;
	
	if (mixPosition.z < position.z)
	{
		mixPosition = position;
		mixColor = color;
	}
	
	if (color.w == 0)
	{
		mixPosition = dsposition;
		mixColor = dscolor;
	}
	
	color = mixColor;
	position = mixPosition;
	
	gPosition = position;
	gNormal = normal;
	gColor = color;
}