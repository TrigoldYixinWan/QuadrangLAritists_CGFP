#version 330 core

// Input vertex attributes
layout(location = 0) in vec3 aPos;      // Vertex position
layout(location = 1) in vec3 aNormal;   // Vertex normal

// Outputs to the fragment shader
out vec3 FragPos;    // Position in world space
out vec3 Normal;     // Normal in world space

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    // Compute the position of the vertex in clip space
    gl_Position = proj * view * model * vec4(aPos, 1.0);

    // Compute the normal matrix to transform normals correctly
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);

    // Compute the position of the vertex in world space
    FragPos = vec3(model * vec4(aPos, 1.0));
}