#version 450

layout (location = 0) in vec3 v_Col;

layout (location = 0) out vec4 f_FragColor;

void main()
{
  f_FragColor = vec4(v_Col, 1.0);
}
