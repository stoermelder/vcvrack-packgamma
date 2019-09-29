#include "plugin.hpp"
#include "Gamma/Envelope.h"

namespace Decay {

struct DecayMk1Module : Module {
	enum ParamIds {
		DECAY_PARAM,
		AMP_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		DECAY_INPUT,
		AMP_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	gam::Decay<> env;	// Exponentially decaying envelope
	dsp::SchmittTrigger gateTrigger;

	DecayMk1Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DECAY_PARAM, 0.1f, 60.f, 10.f, "Decay time in seconds");
		configParam(AMP_PARAM, 0.f, 10.f, 10.f, "Start voltage", "V");
		onReset();
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);

		if (gateTrigger.process(inputs[GATE_INPUT].getVoltage())) {
			float ampParam = params[AMP_PARAM].getValue();
			float amp = inputs[AMP_INPUT].isConnected() ? inputs[AMP_INPUT].getVoltage() * ampParam / 10.f : ampParam;

			float decayParam = params[DECAY_PARAM].getValue();
			float decay = inputs[DECAY_INPUT].isConnected() ? inputs[DECAY_INPUT].getVoltage() * decayParam / 10.f : decayParam;

			env.decay(decay);		// Set decay length in seconds
			env.reset(ampParam);	// Reset envelope and specify amplitude
		}

		float s = env();
		outputs[ENV_OUTPUT].setVoltage(s);
	}
};

struct DecayMk1Widget : ModuleWidget {
	DecayMk1Widget(DecayMk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DecayMk1.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 128.6f), module, DecayMk1Module::DECAY_INPUT));
		addParam(createParamCentered<Trimpot>(Vec(22.5f, 153.7f), module, DecayMk1Module::DECAY_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 195.2f), module, DecayMk1Module::AMP_INPUT));
		addParam(createParamCentered<Trimpot>(Vec(22.5f, 220.3f), module, DecayMk1Module::AMP_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 261.2f), module, DecayMk1Module::GATE_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(22.5f, 304.5f), module, DecayMk1Module::ENV_OUTPUT));
	}
};

} // namespace Decay

Model* modelDecayMk1 = createModel<Decay::DecayMk1Module, Decay::DecayMk1Widget>("Decay-Mk1");