#version 450

in vec3 Normal;
in vec2 Texcoord;

out vec4[3] outBuffer;

uniform sampler2D diffuse1;
uniform sampler2D normal1;
uniform sampler2D specular1;
uniform sampler2DShadow shadowmap1;

struct Material
{
	vec3 emissive;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;

float PCFShadow(vec4 fragPosLight, float NdL)
{
	vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
	projCoords = projCoords * 0.5 + 0.5;
	
	float bias = 0.00007 * tan(acos(NdL));
	//float bias = max(0.00047 * (1.0 - NdL), 0.00007);
	projCoords.z += clamp(bias, 0.00007, 0.00014);
	//projCoords.z *= 1.0 - bias;
	float shadow = texture(shadowmap1, projCoords);
	
	return shadow;
}

void main()
{
	vec3 N = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 halfDir = normalize(lightDir + viewDir);
	
	vec3 diffuseMap = vec3(texture(diffuse1, Texcoord));
	
	vec3 ambient = ambientColor * material.ambient * diffuseMap;
	
	float diffuseMag = max(dot(N, lightDir), 0.0);
	vec3 diffuse = diffuseMag * diffuseColor * material.diffuse * diffuseMap;
	
	float specularMag = pow(max(dot(N, halfDir), 0.0), material.shininess);
	vec3 specular = specularMag * specularColor * material.specular;
	
	float shadow = PCFShadow(FragPosLight, dot(N, lightDir));
	shadow = clamp(shadow, 0.5, 1.0);
	outColor = vec4(material.emissive + ambient + shadow * (diffuse + specular), 1.0);
	//outColor = vec4(vec3(shadow), 1.0);
}