#pragma once
#include "plugin.hpp"

struct CKSSH : CKSS {
	CKSSH() {
		shadow->opacity = 0.0f;
		fb->removeChild(sw);

		TransformWidget* tw = new TransformWidget();
		tw->addChild(sw);
		fb->addChild(tw);

		Vec center = sw->box.getCenter();
		tw->translate(center);
		tw->rotate(M_PI/2.0f);
		tw->translate(Vec(center.y, sw->box.size.x).neg());

		tw->box.size = sw->box.size.flip();
		box.size = tw->box.size;
	}
};