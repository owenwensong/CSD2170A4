#version 450

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec3 v_Col;

const vec2 c_Pos[3] = vec2[3](vec2(1.0, -1.0), vec2(-1.0, -1.0), vec2(-1.0, 1.0));

void main()
{
  uint idx = gl_VertexIndex % 3;
  gl_Position = ubo.projection * (ubo.model * vec4(c_Pos[idx], 0.0, 1.0));
  v_Col = vec3(0.0);
  v_Col[idx] = 1.0;
}
