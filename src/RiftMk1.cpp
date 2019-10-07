#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "Gamma/DFT.h"
#pragma GCC diagnostic pop

namespace RiftMk1 {

// based on examples/spectral/brickwall.cpp
struct RiftMk1Module : Module {
	enum ParamIds {
		LO_PARAM,
		LO_OFFSET_PARAM,
		HI_PARAM,
		HI_OFFSET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		LO_INPUT,
		HI_INPUT,
		INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	gam::STFT stft;
	float prev[PORT_MAX_CHANNELS];
	float cvs[PORT_MAX_CHANNELS] = {};

	RiftMk1Module() :
		stft(2048, 2048/4, 0, gam::HANN, gam::COMPLEX)
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LO_PARAM, 0.f, 2.f, 1.f, "Low CV Attenuation");
		configParam(LO_OFFSET_PARAM, -42.f, 78.f, 0.f, "Low Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(HI_PARAM, 0.f, 2.f, 1.f, "High CV Attenuation");
		configParam(HI_OFFSET_PARAM, -42.f, 78.f, 0.f, "High Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		onReset();
		stft.precise(true);
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);

		if (inputs[INPUT].isConnected()) {
			float s = inputs[INPUT].getVoltage();

			if (stft(s)) {
				// Define the band edges, in Hz
				float freqLoParam = params[LO_OFFSET_PARAM].getValue() / 12.f;
				freqLoParam += inputs[LO_INPUT].isConnected() ? inputs[LO_INPUT].getVoltage() * params[LO_PARAM].getValue() / 5.f : 0.f;
				float freqLo = dsp::FREQ_C4 * powf(2.0, freqLoParam);

				float freqHiParam = params[HI_OFFSET_PARAM].getValue() / 12.f;
				freqHiParam += inputs[HI_INPUT].isConnected() ? inputs[HI_INPUT].getVoltage() * params[HI_PARAM].getValue() / 5.f : 0.f;
				float freqHi = dsp::FREQ_C4 * powf(2.0, freqHiParam);

				for(unsigned k = 0; k < stft.numBins(); ++k){
					// Compute the frequency, in Hz, of this bin
					float freq = k*stft.binFreq();

					// If the bin frequency is outside of our band, then zero the bin.
					if (freq < freqLo || freq > freqHi){
						stft.bin(k) = 0;
					}
				}
			}

			s = stft();
			outputs[OUTPUT].setVoltage(s);
		}
	}
};

struct RiftMk1Widget : ModuleWidget {
	RiftMk1Widget(RiftMk1Module* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RiftMk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 81.6f), module, RiftMk1Module::LO_OFFSET_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 106.5f), module, RiftMk1Module::LO_INPUT));
		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 131.5f), module, RiftMk1Module::LO_PARAM));

		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 171.9f), module, RiftMk1Module::HI_OFFSET_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 196.8f), module, RiftMk1Module::HI_INPUT));
		addParam(createParamCentered<MyTrimpot>(Vec(22.5f, 221.8f), module, RiftMk1Module::HI_PARAM));

		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 280.6f), module, RiftMk1Module::INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(22.5f, 323.8f), module, RiftMk1Module::OUTPUT));
	}
};

} // namespace RiftMk1

Model* modelRiftMk1 = createModel<RiftMk1::RiftMk1Module, RiftMk1::RiftMk1Widget>("Rift-Mk1");