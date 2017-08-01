#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inRay;
uniform sampler2D inLight;

uniform float roughness = 0.24;
uniform float mipLevels = 11;
uniform vec2 bufferSize = vec2(1920, 1080);

///////////////////////////////////////////////////////////////////////////////////////
// Cone tracing methods
///////////////////////////////////////////////////////////////////////////////////////

const float CNST_MAX_SPECULAR_EXP = 1000;

float specularPowerToConeAngle(float specularPower)
{
    // based on phong distribution model
    if(specularPower >= exp2(CNST_MAX_SPECULAR_EXP))
    {
        return 0.0;
    }
    const float xi = 0.244;
    float exponent = 1.0 / (specularPower + 1.0);
    return acos(pow(xi, exponent));
}

float isoscelesTriangleOpposite(float adjacentLength, float coneTheta)
{
    // simple trig and algebra - soh, cah, toa - tan(theta) = opp/adj, opp = tan(theta) * adj, then multiply * 2.0f for isosceles triangle base
    return 2.0f * tan(coneTheta) * adjacentLength;
}

float isoscelesTriangleInRadius(float a, float h)
{
    float a2 = a * a;
    float fh2 = 4.0 * h * h;
    return (a * (sqrt(a2 + fh2) - a)) / (4.0 * h);
}

vec4 coneSampleWeightedColor(vec2 samplePos, float mipChannel, float gloss)
{
    vec3 sampleColor = textureLod(inLight, samplePos, mipChannel).rgb;
    return vec4(sampleColor * gloss, gloss);
}

float isoscelesTriangleNextAdjacent(float adjacentLength, float incircleRadius)
{
    // subtract the diameter of the incircle to get the adjacent side of the next level on the cone
    return adjacentLength - (incircleRadius * 2.0);
}

void main()
{
	vec4 ray = texture(inRay, TexCoord);
	vec4 light = texture(inLight, TexCoord);
	
	/*
	float mipLevel = 0;
	vec3 finalColor = vec3(0);
	vec2 hitCoord = ray.xy;
	
	if (ray.w <= 0.0)
	{
		finalColor = light.rgb;
		outColor = vec4(finalColor, light.a);
		return;
	}
	
	vec3 reflectedColor = textureLod(inLight, hitCoord, mipLevel).rgb;
	finalColor = reflectedColor;
	*/
	
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	
	float gloss = 0.85;
    float specularPower = 2000;
	
	// convert to cone angle (maximum extent of the specular lobe aperture)
    // only want half the full cone angle since we're slicing the isosceles triangle in half to get a right triangle
    float coneTheta = specularPowerToConeAngle(specularPower) * 0.5;
	
	// P1 = positionSS, P2 = ray, adjacent length = ||P2 - P1||
	vec2 deltaP = ray.xy - position.xy;
    float adjacentLength = length(deltaP);
    vec2 adjacentUnit = normalize(deltaP);
	
	vec4 totalColor = vec4(0);
    float remainingAlpha = 1.0;
    float maxMipLevel = mipLevels - 1.0;
    float glossMult = gloss;
    // cone-tracing using an isosceles triangle to approximate a cone in screen space
    for(int i = 0; i < 14; ++i)
    {
        // intersection length is the adjacent side, get the opposite side using trig
        float oppositeLength = isoscelesTriangleOpposite(adjacentLength, coneTheta);

        // calculate in-radius of the isosceles triangle
        float incircleSize = isoscelesTriangleInRadius(oppositeLength, adjacentLength);

        // get the sample position in screen space
        vec2 samplePos = position.xy + adjacentUnit * (adjacentLength - incircleSize);

        // convert the in-radius into screen size then check what power N to raise 2 to reach it - that power N becomes mip level to sample from
        float mipChannel = clamp(log2(incircleSize * max(bufferSize.x, bufferSize.y)), 0.0, maxMipLevel);

        /*
         * Read color and accumulate it using trilinear filtering and weight it.
         * Uses pre-convolved image (color buffer) and glossiness to weigh color contributions.
         * Visibility is accumulated in the alpha channel. Break if visibility is 100% or greater (>= 1.0f).
         */
        vec4 newColor = coneSampleWeightedColor(samplePos, mipChannel, glossMult);

        remainingAlpha -= newColor.a;
        if(remainingAlpha < 0.0f)
        {
            newColor.rgb *= (1.0f - abs(remainingAlpha));
        }
        totalColor += newColor;

        if(totalColor.a >= 1.0f)
        {
            break;
        }

        adjacentLength = isoscelesTriangleNextAdjacent(adjacentLength, incircleSize);
        glossMult *= gloss;
    }
	
	vec3 reflectedColor = textureLod(inLight, ray.xy, 0).rgb;
	vec3 finalColor = mix(light.rgb, totalColor.rgb, remainingAlpha);
	//finalColor = reflectedColor;
	finalColor = vec3(-remainingAlpha);
	
	outColor = vec4(finalColor, light.a);
}