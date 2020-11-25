#pragma once

class Shader
{
public:
	~Shader();
	bool initShader(const char *vertSrc, const char *fragSrc);
	void useProgram();
private:
	unsigned int programId = 0u;
};