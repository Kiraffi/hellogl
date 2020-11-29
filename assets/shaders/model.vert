#version 450 core

layout (location = 0) uniform vec2 windowSize;

struct VData
{
	vec3 vpos;
	uint vModelIndex;
};

struct IData
{
	vec3 iPos;
	float iRotation;

	uint iColor;
	float iSize;
	uint iModelVertexStartIndex;
	float iPadding;
};

layout (std430, binding=1) buffer vertex_data
{
	VData vertexValues[];
};

layout (std430, binding=2) buffer instance_data
{
	IData instanceValues[];
};

layout (location = 0) out vec4 colOut;

void main()
{
		
	uint modelInstance = (gl_VertexID >> 8);
	int indice = (gl_VertexID & 0xff);
	IData iData = instanceValues[modelInstance];
	VData vData = vertexValues[iData.iModelVertexStartIndex + indice];
		
	vec2 p = vData.vpos.xy;
	p *= iData.iSize;
	p += iData.iPos.xy;

	p /= windowSize * 0.5f;
	p -= 1.0f;
		
	gl_Position = vec4(p.xy, 0.5, 1.0);
	vec4 c = vec4(0, 0, 0, 0);
	c.r = float((iData.iColor >> 0u) & 255u) / 255.0f;
	c.g = float((iData.iColor >> 8u) & 255u) / 255.0f;
	c.b = float((iData.iColor >> 16u) & 255u) / 255.0f;
	c.a = float((iData.iColor >> 24u) & 255u) / 255.0f;
	colOut = c;
}