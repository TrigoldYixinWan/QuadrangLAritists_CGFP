#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_Projection;

void main() {
    gl_Position = u_Projection * u_Model * vec4(aPos, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
}
