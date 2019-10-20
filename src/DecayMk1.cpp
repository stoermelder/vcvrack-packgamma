#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "Gamma/Envelope.h"
#pragma GCC diagnostic pop

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

	gam::Decay<> env[PORT_MAX_CHANNELS];	// Exponentially decaying envelope
	dsp::SchmittTrigger gateTrigger[PORT_MAX_CHANNELS];

	DecayMk1Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DECAY_PARAM, 0.1f, 60.f, 10.f, "Decay time in seconds");
		configParam(AMP_PARAM, 0.f, 10.f, 10.f, "Start voltage", "V");
		onReset();
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);
		int c = inputs[GATE_INPUT].getChannels();
		outputs[ENV_OUTPUT].setChannels(c);

		if (c > 0) {
			float ampParam = params[AMP_PARAM].getValue();
			float amp = inputs[AMP_INPUT].isConnected() ? inputs[AMP_INPUT].getVoltage() * ampParam / 10.f : ampParam;

			float decayParam = params[DECAY_PARAM].getValue();
			float decay = inputs[DECAY_INPUT].isConnected() ? inputs[DECAY_INPUT].getVoltage() * decayParam / 10.f : decayParam;

			for (int i = 0; i < c; i++) {
				if (gateTrigger[i].process(inputs[GATE_INPUT].getVoltage(i))) {
					env[i].decay(decay);	// Set decay length in seconds
					env[i].reset(amp);		// Reset envelope and specify amplitude
				}

				float s = env[i]();
				outputs[ENV_OUTPUT].setVoltage(s, i);
			}
		}
	}
};

struct DecayMk1Widget : ModuleWidget {
	DecayMk1Widget(DecayMk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DecayMk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 74.9f), module, DecayMk1Module::DECAY_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 100.0f), module, DecayMk1Module::DECAY_PARAM));
		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 141.4f), module, DecayMk1Module::AMP_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 166.6f), module, DecayMk1Module::AMP_PARAM));
		
		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 280.6f), module, DecayMk1Module::GATE_INPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(22.5f, 323.8f), module, DecayMk1Module::ENV_OUTPUT));
	}
};

} // namespace Decay

Model* modelDecayMk1 = createModel<Decay::DecayMk1Module, Decay::DecayMk1Widget>("Decay-Mk1");