#include "plugin.hpp"


Plugin* pluginInstance;

void init(rack::Plugin* p) {
	pluginInstance = p;

	p->addModel(modelFreezeMk1);
	p->addModel(modelDecayMk1);
	p->addModel(modelBitMk1);
	p->addModel(modelSineMk1);
	p->addModel(modelWallMk1);
}