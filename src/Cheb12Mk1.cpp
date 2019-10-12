#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "Gamma/Oscillator.h"
#include "Gamma/Effects.h"
#pragma GCC diagnostic pop

namespace Cheb12Mk1 {

// based on examples/effects/cheby.cpp
struct Cheb12Mk1Module : Module {
	enum ParamIds {
		ENUMS(HARM_PARAM, 12),
		FREQ_PARAM,
		FINE_PARAM,
		OCT_PARAM,
		ROT_PARAM,
		DETUNE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(HARM_INPUT, 12),
		VOCT_INPUT,
		ROT_INPUT,
		DETUNE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(HARM_LIGHT, 12 * 8),
		NUM_LIGHTS
	};

	gam::Sine<> osc[8];			// Source sine
	gam::ChebyN<12> cheby[8];	// Chebyshev waveshaper with 12 harmonics
	float cvs[8] = {};

	dsp::ClockDivider lightDivider;

	Cheb12Mk1Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int k = 0; k < 12; k++)
			configParam(HARM_PARAM + k, 0.f, 1.f, 1.f, "Harmonic");
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine frequency");
		configParam(OCT_PARAM, -3.f, 3.f, 0.f, "Octave");
		configParam(ROT_PARAM, -12.f, 12.f, 0.f, "Rotate harmonics per voice");
		configParam(DETUNE_PARAM, -1.f, 1.f, 0.f, "Detune");
		onReset();
		lightDivider.setDivision(1024);
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);
		int c = std::min(std::max(inputs[VOCT_INPUT].getChannels(), 1), 8);
		outputs[OUTPUT].setChannels(c);

		float freqParam = params[FREQ_PARAM].getValue() / 12.f;
		freqParam += params[OCT_PARAM].getValue();
		freqParam += dsp::quadraticBipolar(params[FINE_PARAM].getValue()) * 3.f / 12.f;

		int rotParam = inputs[ROT_INPUT].isConnected() ? int(std::floor(inputs[ROT_INPUT].getVoltage() / 0.833f)) : params[ROT_PARAM].getValue();

		float detuneParam = params[DETUNE_PARAM].getValue();
		float detune = inputs[DETUNE_INPUT].isConnected() ? inputs[DETUNE_INPUT].getVoltage() / 5.f * detuneParam : detuneParam;
		detune = dsp::quadraticBipolar(detuneParam) * 3.f / 12.f;

		float harm[12];
		for (int k = 0; k < 12; k++) {
			float harmParam = params[HARM_PARAM + k].getValue();
			harm[k] = inputs[HARM_INPUT + k].isConnected() ? inputs[HARM_INPUT + k].getVoltage() / 10.f * harmParam : harmParam;
		}

		int rot = 0;
		for (int i = 0; i < c; i++) {
			float vol = 0.f;
			for (int k = 0; k < 12; k++) {
				// Set amplitude of kth harmonic
				cheby[i].coef(k) = harm[(k - rot + 60) % 12];
				vol += cheby[i].coef(k);
			}

			float pitch = freqParam + (detune * i) + inputs[VOCT_INPUT].getVoltage(i);
			if (pitch != cvs[i]) {
				cvs[i] = pitch;
				//float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
				float freq = dsp::FREQ_C4 * powf(2.0, pitch);
				osc[i].freq(freq);
			}

			float s = osc[i]();

			// Generate harmonics
			s = cheby[i](s);

			// Divide by number of harmonics to prevent clipping
			s /= vol;

			float o = rescale(s, -1.f, 1.f, -5.f, 5.f);
			outputs[OUTPUT].setVoltage(o, i);

			rot += rotParam;
		}

		// Set channel lights infrequently
		if (lightDivider.process()) {
			for (int i = 0; i < 8; i++) {
				for (int k = 0; k < 12; k++) {
					float l = i >= c ? 0.f : cheby[i].coef(k);
					lights[HARM_LIGHT + i * 12 + k].setBrightness(l);
				}
			}
		}
	}
};

struct Cheb12Mk1Widget : ModuleWidget {
	Cheb12Mk1Widget(Cheb12Mk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Cheb12Mk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<MyBlackScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		Vec c = Vec(90.0f, 143.1f);
		for (int i = 0; i < 8; i++) {
			for (int k = 0; k < 12; k++) {
				Vec p = Vec(sin(k * 2.f * M_PI / 12.f), -cos(k * 2.f * M_PI / 12.f)).mult(5.7f + (7 - i) * 3.4f).plus(c);
				addChild(createLightCentered<TinyLight<GreenLight>>(p, module, Cheb12Mk1Module::HARM_LIGHT + i * 12 + k));
			}
		}

		addInput(createInputCentered<StoermelderPort>(Vec(90.0f, 75.5f),   module, Cheb12Mk1Module::HARM_INPUT + 0));
		addInput(createInputCentered<StoermelderPort>(Vec(123.8f, 84.5f),  module, Cheb12Mk1Module::HARM_INPUT + 1));
		addInput(createInputCentered<StoermelderPort>(Vec(148.6f, 109.3f), module, Cheb12Mk1Module::HARM_INPUT + 2));
		addInput(createInputCentered<StoermelderPort>(Vec(157.7f, 143.1f), module, Cheb12Mk1Module::HARM_INPUT + 3));
		addInput(createInputCentered<StoermelderPort>(Vec(148.6f, 177.0f), module, Cheb12Mk1Module::HARM_INPUT + 4));
		addInput(createInputCentered<StoermelderPort>(Vec(123.8f, 201.7f), module, Cheb12Mk1Module::HARM_INPUT + 5));
		addInput(createInputCentered<StoermelderPort>(Vec(90.0f, 210.8f),  module, Cheb12Mk1Module::HARM_INPUT + 6));
		addInput(createInputCentered<StoermelderPort>(Vec(56.2f, 201.7f),  module, Cheb12Mk1Module::HARM_INPUT + 7));
		addInput(createInputCentered<StoermelderPort>(Vec(31.4f, 177.0f),  module, Cheb12Mk1Module::HARM_INPUT + 8));
		addInput(createInputCentered<StoermelderPort>(Vec(22.3f, 143.1f),  module, Cheb12Mk1Module::HARM_INPUT + 9));
		addInput(createInputCentered<StoermelderPort>(Vec(31.4f, 109.3f),  module, Cheb12Mk1Module::HARM_INPUT + 10));
		addInput(createInputCentered<StoermelderPort>(Vec(56.2f, 84.5f),   module, Cheb12Mk1Module::HARM_INPUT + 11));

		addParam(createParamCentered<StoermelderTrimpot>(Vec(90.0f, 100.2f),  module, Cheb12Mk1Module::HARM_PARAM + 0));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(111.5f, 105.9f), module, Cheb12Mk1Module::HARM_PARAM + 1));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(127.2f, 121.7f), module, Cheb12Mk1Module::HARM_PARAM + 2));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(133.0f, 143.1f), module, Cheb12Mk1Module::HARM_PARAM + 3));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(127.2f, 164.6f), module, Cheb12Mk1Module::HARM_PARAM + 4));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(111.5f, 180.3f), module, Cheb12Mk1Module::HARM_PARAM + 5));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(90.0f, 186.1f),  module, Cheb12Mk1Module::HARM_PARAM + 6));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(68.5f, 180.3f),  module, Cheb12Mk1Module::HARM_PARAM + 7));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(52.8f, 164.6f),  module, Cheb12Mk1Module::HARM_PARAM + 8));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(47.0f, 143.1f),  module, Cheb12Mk1Module::HARM_PARAM + 9));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(52.8f, 121.7f),  module, Cheb12Mk1Module::HARM_PARAM + 10));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(68.5f, 105.9f),  module, Cheb12Mk1Module::HARM_PARAM + 11));

		addInput(createInputCentered<StoermelderPort>(Vec(20.8f, 257.7f), module, Cheb12Mk1Module::ROT_INPUT));
		StoermelderTrimpot* tp1 = createParamCentered<StoermelderTrimpot>(Vec(20.8f, 280.7f), module, Cheb12Mk1Module::ROT_PARAM);
		tp1->snap = true;
		addParam(tp1);

		addInput(createInputCentered<StoermelderPort>(Vec(158.5f, 257.7f), module, Cheb12Mk1Module::DETUNE_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(158.5f, 280.7f), module, Cheb12Mk1Module::DETUNE_PARAM));

		addParam(createParamCentered<StoermelderTrimpot>(Vec(63.2f, 323.8f), module, Cheb12Mk1Module::FREQ_PARAM));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(90.0f, 323.8f), module, Cheb12Mk1Module::FINE_PARAM));
		StoermelderTrimpot* tp2 = createParamCentered<StoermelderTrimpot>(Vec(116.8f, 323.8f), module, Cheb12Mk1Module::OCT_PARAM);
		tp2->snap = true;
		addParam(tp2);

		addInput(createInputCentered<StoermelderPort>(Vec(20.8f, 323.8f), module, Cheb12Mk1Module::VOCT_INPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(158.5f, 323.8f), module, Cheb12Mk1Module::OUTPUT));
	}
};

} // namespace Cheb12Mk1

Model* modelCheb12Mk1 = createModel<Cheb12Mk1::Cheb12Mk1Module, Cheb12Mk1::Cheb12Mk1Widget>("Cheb12-Mk1");