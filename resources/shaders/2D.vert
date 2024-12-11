#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;  // Keep this for other shapes

uniform mat4 u_Projection;
uniform mat4 u_Model;

out vec2 v_TexCoord;
out vec2 v_Position;

void main() {
    gl_Position = u_Projection * u_Model * vec4(position, 0.0, 1.0);
    v_TexCoord = texCoord;
    v_Position = position;
}