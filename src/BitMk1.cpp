#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "Gamma/Effects.h"
#pragma GCC diagnostic pop

namespace BitMk1 {

struct BitMk1Module : Module {
	enum ParamIds {
		FREQ_PARAM,
		FREQTAPER_PARAM,
		STEP_PARAM,
		STEPTAPER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT,
		FREQ_INPUT,
		STEP_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	gam::Quantizer<> qnt[PORT_MAX_CHANNELS];	// Quantization modulator

	BitMk1Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQ_PARAM, 0.f, 1.f, 1.f);
		configParam(FREQTAPER_PARAM, 0.f, 1.f, 1.f);
		configParam(STEP_PARAM, 0.f, 1.f, 1.f);
		configParam(STEPTAPER_PARAM, 0.f, 1.f, 1.f);
		onReset();
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);
		int c = inputs[INPUT].getChannels();
		outputs[OUTPUT].setChannels(c);

		if (c > 0) {
			float freqParam = params[FREQ_PARAM].getValue();
			float freq = inputs[FREQ_INPUT].isConnected() ? inputs[FREQ_INPUT].getVoltage() * freqParam / 10.f : freqParam;
			if (params[FREQTAPER_PARAM].getValue() == 1.f)
				freq = args.sampleRate * (1.f - powf(1.f - freq, 0.5f));
			else
				freq = args.sampleRate * freq;
			
			float stepParam = params[STEP_PARAM].getValue();
			float step = inputs[STEP_INPUT].isConnected() ? inputs[STEP_INPUT].getVoltage() * stepParam / 10.f : stepParam;
			if (params[STEPTAPER_PARAM].getValue() == 1.f)
				step = 1.f - powf(step, 0.5f);
			else
				step = 1.f - step;

			for (int i = 0; i < c; i++) {
				qnt[i].freq(freq);		// Set sample rate quantization
				qnt[i].step(step);		// Set amplitude quantization

				float s = rescale(inputs[INPUT].getVoltage(i), -10.f, 10.f, -1.f, 1.f);
				s = qnt[i](s);			// Apply the bitcrush
				s = rescale(s, -1.f, 1.f, -10.f, 10.f);
				outputs[OUTPUT].setVoltage(s, i);
			}
		}
	}
};

struct BitMk1Widget : ModuleWidget {
	BitMk1Widget(BitMk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BitMk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 74.8f), module, BitMk1Module::FREQ_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 99.9f), module, BitMk1Module::FREQ_PARAM));
		addParam(createParamCentered<CKSSH>(Vec(22.5f, 131.2f), module, BitMk1Module::FREQTAPER_PARAM));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 174.1f), module, BitMk1Module::STEP_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 199.2f), module, BitMk1Module::STEP_PARAM));
		addParam(createParamCentered<CKSSH>(Vec(22.5f, 230.7f), module, BitMk1Module::STEPTAPER_PARAM));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 278.4f), module, BitMk1Module::INPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(22.5f, 323.8f), module, BitMk1Module::OUTPUT));
	}
};

} // namespace BitMk1

Model* modelBitMk1 = createModel<BitMk1::BitMk1Module, BitMk1::BitMk1Widget>("Bit-Mk1");