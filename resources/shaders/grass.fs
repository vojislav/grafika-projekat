#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D basicTex;

void main()
{
    vec4 texColor = texture(basicTex, TexCoords);
    if(texColor.a < 0.01)
        discard;
    FragColor = texColor;
}