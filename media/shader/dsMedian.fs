#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

uniform float imgWidth = 640;
uniform float imgHeight = 480;

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


void main()
{
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	
	// 3x3 Median filter kinect position
	int i, j, k;
	int kernelRadius = 1;
	int center = kernelRadius * 2 + 1 + kernelRadius;
	float c[25];
	float tmp;

	k = 0;
	for( i = -kernelRadius ; i <= kernelRadius ; i++ )
	{
		for( j = -kernelRadius ; j <= kernelRadius ; j++ )
		{
			vec2 offset = vec2( i / float( imgWidth ), j / float( imgHeight ) );
			c[k++] = texture(dsDepth, TexCoord + offset).r;
		}
	}

	// 3X3 kernel
	small9( c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8] );
	small8( c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8] );
	small7( c[2], c[3], c[4], c[5], c[6], c[7], c[8] );
	small6( c[3], c[4], c[5], c[6], c[7], c[8] );
	small5( c[4], c[5], c[6], c[7], c[8] );

	dsdepth = c[center];
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}