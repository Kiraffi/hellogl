#pragma once
#include <stdint.h>

class Shader
{
public:
	~Shader();
	bool initShader(const char *vertSrc, const char *fragSrc);
	void useProgram();
private:
	uint32_t programId = 0u;
};