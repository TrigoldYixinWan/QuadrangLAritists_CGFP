#version 330 core

in vec2 v_TexCoord;
out vec4 FragColor;
uniform vec3 u_Color;
uniform sampler2D u_Texture;
uniform bool u_UseTexture;

void main() {
    if(u_UseTexture) {
        FragColor = texture(u_Texture, v_TexCoord);
    } else {
        FragColor = vec4(u_Color, 1.0);
    }
}
