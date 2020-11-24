#include "shader.h"

#include <cstdio>
#include <cstdlib>

#include "../../external/glad/glad.h"

Shader::~Shader()
{
	if(programId)
		glDeleteProgram(programId);
	programId = 0u;
	printf("deleting program\n");
}

static unsigned int shaderFromSource(const char *src, unsigned int shaderType)
{
	unsigned int shader = 0;
	shader = glCreateShader(shaderType);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	
	int  success = 0;
	char infoLog[1024];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(shader, 1024, NULL, infoLog);
		switch(shaderType)
		{
			case GL_VERTEX_SHADER:
			{
				printf("ERROR::Vertex shader compiler error: %s\n", infoLog);
			}
			break;

			case GL_FRAGMENT_SHADER:
			{
				printf("ERROR::Fragment shader compiler error: %s\n", infoLog);
			}
			break;

			case GL_COMPUTE_SHADER:
			{
				printf("ERROR::Compute shader compiler error: %s\n", infoLog);
			}
		}
		return 0;
	}	

	return shader;
}

bool Shader::initShader(const char *vertSrc, const char *fragSrc)
{
	unsigned int vertexShader = shaderFromSource(vertSrc, GL_VERTEX_SHADER);
	if(vertexShader == 0)
	{
		printf("Error at compiling vertex shader\n");
		return false;
	}
	
	unsigned int fragmentShader = shaderFromSource(fragSrc, GL_FRAGMENT_SHADER);
	if(fragmentShader == 0)
	{
		glDeleteShader(vertexShader);
		printf("Error at compiling fragment shader\n");
		return false;
	}

	programId = glCreateProgram();

	glAttachShader(programId, vertexShader);
	glAttachShader(programId, fragmentShader);
	glLinkProgram(programId);

	int  success = 0;
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if(!success)
	{
		char infoLog[1024];
		glGetShaderInfoLog(programId, 1024, NULL, infoLog);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		printf("ERROR::Shader linking failed: %s\n", infoLog);
		return false;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return true;

}

void Shader::useProgram()
{
	glUseProgram(programId);
}

