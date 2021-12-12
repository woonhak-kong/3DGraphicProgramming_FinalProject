#version 430 core

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 5
#endif
in vec3 color;
in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;
out vec4 frag_color;

struct Light
{
	vec3 diffuseColor;
	float diffuseStrength;
};

struct AmbientLight
{
	Light base;
};

struct DirectionalLight
{
	Light base;
	vec3 direction;
};

struct PointLight
{
	Light base;
	vec3 position;
	float constant;
	float linear;
	float quadratic;
};

struct Material
{
	float specularStrength;
	float shininess;
};

uniform sampler2D texture0;
uniform vec3 eyePosition;

uniform AmbientLight aLight;
uniform DirectionalLight dLight;
uniform PointLight pLights[NUM_POINT_LIGHTS];
uniform Material mat;

vec4 calcAmbientLight(Light a)
{
	vec4 ambient = vec4(a.diffuseColor, 1.0f) * a.diffuseStrength;
	return ambient;
}

vec4 calcLightByDirection(Light l, vec3 dir)
{
	float diffuseFactor = max( dot( normalize(normal), normalize(dir) ), 0.0f);
	vec4 diffuse = vec4(l.diffuseColor, 1.0f) * l.diffuseStrength * diffuseFactor;

	vec4 specular = vec4(0,0,0,0);
	if (diffuseFactor > 0.0f && l.diffuseStrength > 0.0f)
	{
		vec3 fragToEye = normalize(eyePosition - fragPos);
		vec3 reflectedVertex = normalize(reflect(dir, normalize(normal)));

		float specularFactor = dot(fragToEye, reflectedVertex);
		if (specularFactor > 0.0f)
		{
			specularFactor = pow(specularFactor, mat.shininess);
			specular = vec4(l.diffuseColor * mat.specularStrength * specularFactor, 1.0f);
		}
	}
	return (diffuse + specular);
}

vec4 calcDirectionalLight()
{
	return calcLightByDirection(dLight.base, dLight.direction);
}

vec4 calcPointLight(PointLight p)
{
	vec3 direction = fragPos - p.position;
	float distance = length(direction);
	direction = normalize(direction);

	vec4 color = calcLightByDirection(p.base, direction);
	float attenuation = p.quadratic * distance * distance +
		p.linear * distance +
		p.constant;
	//attenuation = 5.0;
	return (color / attenuation);
}

//vec4 calcPointLight()
//{
//	vec3 direction = pLight.position - fragPos;
//	float distance = length(direction);
//	direction = normalize(direction);
//
//	vec4 color = calcLightByDirection(pLight.base, direction);
//	float attenuation = 1 / (pLight.quadratic * distance * distance +
//		pLight.linear * distance +
//		pLight.constant);
//
//	return (color * attenuation);
//}

void main()
{
	// Calculate lighting.
	vec4 calcColor;
	calcColor += calcAmbientLight(aLight.base);
	calcColor += calcDirectionalLight();
	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
		calcColor += calcPointLight(pLights[i]);

	frag_color = texture(texture0, texCoord) * vec4(color, 1.0f) * calcColor;
}