#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_texture;
uniform int u_filterType; // 0: None, 1: Grayscale, 2: Invert, 3: Sharpen, 4: Blur

uniform vec2 u_texelSize; // Size of one texel

void main()
{
    vec4 color = texture(u_texture, v_texCoord);

    if (u_filterType == 1)
    {
        // Grayscale filter
        float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
        color = vec4(vec3(gray), color.a);
    }
    else if (u_filterType == 2)
    {
        // Invert filter
        color.rgb = vec3(1.0) - color.rgb;
    }
    else if (u_filterType == 3) {
    // Adjusted Sharpen filter
    float kernel[9] = float[](
         0, -1,  0,
        -1,  5, -1,
         0, -1,  0
    );
    vec3 result = vec3(0.0);
    int index = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offset = vec2(float(x), float(y)) * u_texelSize;
            vec3 sample = texture(u_texture, v_texCoord + offset).rgb;
            result += sample * kernel[index++];
        }
    }
    // Optionally clamp the result to avoid artifacts
    result = clamp(result, 0.0, 1.0);
    color = vec4(result, color.a);
}
else if (u_filterType == 4) {
    // Gaussian Blur filter
    float kernel[25] = float[](
        1,  4,  7,  4, 1,
        4, 16, 26, 16, 4,
        7, 26, 41, 26, 7,
        4, 16, 26, 16, 4,
        1,  4,  7,  4, 1
    );
    float kernelSum = 273.0;
    vec3 result = vec3(0.0);
    int index = 0;
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 offset = vec2(float(x), float(y)) * u_texelSize;
            vec3 sample = texture(u_texture, v_texCoord + offset).rgb;
            result += sample * kernel[index++];
        }
    }
    color = vec4(result / kernelSum, color.a);
}

    FragColor = color;
}