#include "plugin.hpp"
#include "components.hpp"
#include "Gamma/Oscillator.h"

namespace OphMk1 {

// based on examples/synthesis/pmFeedback.cpp
struct OphMk1Module : Module {
	enum ParamIds {
		FBK_PARAM,
		FBKTAPER_PARAM,
		FREQ_PARAM,
		FINE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT,
		FBK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	gam::Sine<> osc[PORT_MAX_CHANNELS];		// Source sine
	float prev[PORT_MAX_CHANNELS];
	float cvs[PORT_MAX_CHANNELS] = {};

	OphMk1Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FBK_PARAM, 0.f, 1.f, 1.f, "Feedback amount");
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine frequency");
		onReset();
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);
		int c = std::max(inputs[VOCT_INPUT].getChannels(), 1);
		outputs[OUTPUT].setChannels(c);

		float freqParam = params[FREQ_PARAM].getValue() / 12.f;
		freqParam += dsp::quadraticBipolar(params[FINE_PARAM].getValue()) * 3.f / 12.f;

		float fbkParam = params[FBK_PARAM].getValue();

		for (int i = 0; i < c; i++) {
			int c_fbk = inputs[FBK_INPUT].getChannels() == c ? i : 0;
			float fbk = inputs[FBK_INPUT].isConnected() ? inputs[FBK_INPUT].getVoltage(c_fbk) * fbkParam / 10.f : fbkParam;
			if (params[FBKTAPER_PARAM].getValue() == 1.f)
				fbk = powf(fbk, 0.5f);

			float pitch = freqParam + inputs[VOCT_INPUT].getVoltage(i);
			if (pitch != cvs[i]) {
				cvs[i] = pitch;
				//float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
				float freq = dsp::FREQ_C4 * powf(2.0, pitch);
				osc[i].freq(freq);
			}

			fbk = prev[i] * fbk * 0.4f;
			// Add feedback to phase
			osc[i].phaseAdd(fbk);

			prev[i] = osc[i]();

			// Subtract feedback from phase to avoid changing pitch
			osc[i].phaseAdd(-fbk);
			float o = rescale(prev[i], -1.f, 1.f, -5.f, 5.f);
			outputs[OUTPUT].setVoltage(o, i);
		}
	}
};

struct OphMk1Widget : ModuleWidget {
	OphMk1Widget(OphMk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/OphMk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 83.4f), module, OphMk1Module::FBK_INPUT));
		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 108.5f), module, OphMk1Module::FBK_PARAM));
		addParam(createParamCentered<CKSSH>(Vec(22.5f, 139.8f), module, OphMk1Module::FBKTAPER_PARAM));

		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 205.2f), module, OphMk1Module::FREQ_PARAM));
		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 235.2f), module, OphMk1Module::FINE_PARAM));

		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 280.6f), module, OphMk1Module::VOCT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(22.5f, 323.8f), module, OphMk1Module::OUTPUT));
	}
};

} // namespace OphMk1

Model* modelOphMk1 = createModel<OphMk1::OphMk1Module, OphMk1::OphMk1Widget>("Oph-Mk1");