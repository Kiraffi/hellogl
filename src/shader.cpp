#include "shader.h"

#include <cstdio>
#include <cstdlib>

#include "glad.h"

Shader::~Shader()
{
	if(programId)
		glDeleteProgram(programId);
	programId = 0u;
	printf("deleting program\n");
}

bool Shader::initShader(const char *vertSrc, const char *fragSrc)
{
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);


	glShaderSource(vertexShader, 1, &vertSrc, NULL);
	glCompileShader(vertexShader);
	
	int  success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		printf("ERROR::Vertex shader compiler error: %s\n", infoLog);
		return false;
	}	
	
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragSrc, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		printf("ERROR::Fragment shader compile error: %s\n", infoLog);
		return false;
	}	


	programId = glCreateProgram();

	glAttachShader(programId, vertexShader);
	glAttachShader(programId, fragmentShader);
	glLinkProgram(programId);

	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(programId, 512, NULL, infoLog);
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

