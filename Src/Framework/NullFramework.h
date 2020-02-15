#pragma once
#include "../Resources/ResourceManager.h"

class NullFramework : public SingletonResource<NullFramework>
{
public:
	NullFramework() {}
	~NullFramework() {}

	int initImGui() { return 1; }
	void newFrameImGui() {}

	//void uploadTexture(Texture*) {}

	void update() {}
	void render() {}

	bool shouldQuit() const { return false; }
};