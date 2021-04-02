#pragma once

#include "Generator.h"
#include "../Meta/Meta.h"
class Object;
class TextureGenerator : public Generator
{
public:
	TextureGenerator();
	~TextureGenerator();

	void size(int width, int height);
	void clear(const glm::vec4& colour);
	void rect(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& colour);
	void line(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& colour);
	void text(const char* text, const glm::vec2& position, int size, const glm::vec4& colour = { 1.0f, 1.0f, 1.0f, 1.0f });

	Rendering::Texture* generate(unsigned int width, unsigned int height) const;
	Rendering::Texture* generate() const;

	static void test();

	static TextureGenerator* Instance;

protected:
	template<typename T> void push(T);

protected:
	struct Impl;
	std::unique_ptr<Impl> p;
};

template<>
Meta::Object Meta::instanceMeta<TextureGenerator>();