#version 460 core
out vec4 color;

in vec2 loc;
in vec4 col;

void main()
{
	color = col;
}
