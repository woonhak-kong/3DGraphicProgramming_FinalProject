#version 430 core

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 2) in vec2 vertex_texture;
layout(location = 3) in vec3 vertex_normal;

out vec3 color;
out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;

// Values that stay constant for the whole mesh.
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(vertex_position, 1.0f);
	color = vertex_color;
	texCoord = vertex_texture;
	// normal = vertex_normal;
	normal = mat3(transpose(inverse(model))) * vertex_normal; // Only needed if there's non-uniform scaling.
	fragPos = (model * vec4(vertex_position, 1.0f)).xyz;
}