#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "Gamma/Oscillator.h"
#pragma GCC diagnostic pop

namespace SineMk1 {

// based on examples/synthesis/pmFeedback.cpp
struct SineMk1Module : Module {
	enum ParamIds {
		FBK_PARAM,
		FBKTAPER_PARAM,
		FREQ_PARAM,
		FINE_PARAM,
		OCT_PARAM,
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
		ENUMS(PHASE_LIGHT, 3),
		NUM_LIGHTS
	};

	gam::Sine<> osc[PORT_MAX_CHANNELS];		// Source sine
	float prev[PORT_MAX_CHANNELS];
	float cvs[PORT_MAX_CHANNELS] = {};

	dsp::ClockDivider lightDivider;

	SineMk1Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FBK_PARAM, 0.f, 1.f, 0.f, "Feedback amount");
		configParam(FBKTAPER_PARAM, 0.f, 1.f, 1.f, "Feedback taper");
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine frequency");
		configParam(OCT_PARAM, -3.f, 3.f, 0.f, "Octave");
		onReset();
		lightDivider.setDivision(32);
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);
		int channels = std::max(inputs[VOCT_INPUT].getChannels(), 1);
		outputs[OUTPUT].setChannels(channels);

		float freqParam = params[FREQ_PARAM].getValue() / 12.f;
		freqParam += params[OCT_PARAM].getValue();
		freqParam += dsp::quadraticBipolar(params[FINE_PARAM].getValue()) * 3.f / 12.f;

		float fbkParam = params[FBK_PARAM].getValue();

		for (int i = 0; i < channels; i++) {
			int c_fbk = inputs[FBK_INPUT].getChannels() == channels ? i : 0;
			float fbk = inputs[FBK_INPUT].isConnected() ? inputs[FBK_INPUT].getVoltage(c_fbk) * fbkParam / 10.f : fbkParam;
			if (params[FBKTAPER_PARAM].getValue() == 1.f)
				fbk = 1 - powf(1 - fbk, 0.5f);

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

		// Light
		if (lightDivider.process()) {
			if (channels == 1) {
				float lightValue = simd::sin(2 * M_PI * (osc[0].phase() * 2.f - 1.f));
				lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 2].setBrightness(0.f);
			}
			else {
				lights[PHASE_LIGHT + 0].setBrightness(0.f);
				lights[PHASE_LIGHT + 1].setBrightness(0.f);
				lights[PHASE_LIGHT + 2].setBrightness(1.f);
			}
		}
	}
};

struct SineMk1Widget : ModuleWidget {
	SineMk1Widget(SineMk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SineMk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 83.4f), module, SineMk1Module::FBK_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 108.5f), module, SineMk1Module::FBK_PARAM));
		addParam(createParamCentered<CKSSH>(Vec(22.5f, 139.8f), module, SineMk1Module::FBKTAPER_PARAM));

		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 177.0f), module, SineMk1Module::FREQ_PARAM));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 206.5f), module, SineMk1Module::FINE_PARAM));
		StoermelderTrimpot* tp1 = createParamCentered<StoermelderTrimpot>(Vec(22.5f, 236.0f), module, SineMk1Module::OCT_PARAM);
		tp1->snap = true;
		addParam(tp1);

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 276.4f), module, SineMk1Module::VOCT_INPUT));

		addOutput(createOutputCentered<StoermelderPort>(Vec(22.5f, 318.3f), module, SineMk1Module::OUTPUT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(31.5f, 332.7f), module, SineMk1Module::PHASE_LIGHT));
	}
};

} // namespace SineMk1

Model* modelSineMk1 = createModel<SineMk1::SineMk1Module, SineMk1::SineMk1Widget>("Sine-Mk1");