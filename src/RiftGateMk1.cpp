#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "Gamma/DFT.h"
#pragma GCC diagnostic pop

namespace RiftGate {

// based on examples/spectral/brickwall.cpp
struct RiftGateModule : Module {
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
		IN_INPUT,
		OUTER_INPUT,
		INNER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTER_OUTPUT,
		INNER_OUTPUT,
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	gam::STFT in1_stft, in2_stft;
	gam::STFT out1_stft, out2_stft;
	float prev[PORT_MAX_CHANNELS];
	float cvs[PORT_MAX_CHANNELS] = {};

	RiftGateModule() :
		in1_stft(2048, 2048/4, 0, gam::HANN, gam::COMPLEX),
		in2_stft(2048, 2048/4, 0, gam::HANN, gam::COMPLEX),
		out1_stft(2048, 2048/4, 0, gam::HANN, gam::COMPLEX),
		out2_stft(2048, 2048/4, 0, gam::HANN, gam::COMPLEX)
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LO_PARAM, 0.f, 2.f, 1.f, "Low CV Attenuation");
		configParam(LO_OFFSET_PARAM, -42.f, 78.f, 0.f, "Low Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(HI_PARAM, 0.f, 2.f, 1.f, "High CV Attenuation");
		configParam(HI_OFFSET_PARAM, -42.f, 78.f, 0.f, "High Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		onReset();
		in1_stft.precise(true);
		in2_stft.precise(true);
		out1_stft.precise(true);
		out2_stft.precise(true);
	}

	void process(const ProcessArgs &args) override {
		gam::Domain::master().spu(args.sampleRate);

		float s1 = inputs[IN_INPUT].getVoltage();
		float s2 = inputs[OUTER_INPUT].getVoltage();
		float s3 = inputs[INNER_INPUT].getVoltage();

		if (in1_stft(s1) || out1_stft(s2) || out2_stft(s3)) {
			// Define the band edges, in Hz
			float freqLoParam = params[LO_OFFSET_PARAM].getValue() / 12.f;
			freqLoParam += inputs[LO_INPUT].isConnected() ? inputs[LO_INPUT].getVoltage() * params[LO_PARAM].getValue() / 5.f : 0.f;
			float freqLo = dsp::FREQ_C4 * powf(2.0, freqLoParam);

			float freqHiParam = params[HI_OFFSET_PARAM].getValue() / 12.f;
			freqHiParam += inputs[HI_INPUT].isConnected() ? inputs[HI_INPUT].getVoltage() * params[HI_PARAM].getValue() / 5.f : 0.f;
			float freqHi = dsp::FREQ_C4 * powf(2.0, freqHiParam);

			for(unsigned k = 0; k < in1_stft.numBins(); ++k){
				// Compute the frequency, in Hz, of this bin
				float freq = k * in1_stft.binFreq();

				// If the bin frequency is outside of our band, then zero the bin.
				if (freq < freqLo || freq > freqHi) {
					in2_stft.bin(k)[0] = in1_stft.bin(k)[0];
					in2_stft.bin(k)[1] = in1_stft.bin(k)[1];
					in1_stft.bin(k) = 0;
				} else {
					in2_stft.bin(k) = 0;
					out1_stft.bin(k)[0] = out2_stft.bin(k)[0];
					out1_stft.bin(k)[1] = out2_stft.bin(k)[1];
				}
			}
		}

		float o1 = in1_stft();
		float o2 = in2_stft();
		outputs[INNER_OUTPUT].setVoltage(o1);
		outputs[OUTER_OUTPUT].setVoltage(o2);

		float o = out1_stft();
		outputs[OUT_OUTPUT].setVoltage(o);
	}
};

struct RiftGateMk1Widget : ModuleWidget {
	RiftGateMk1Widget(RiftGateModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RiftGateMk1.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 81.6f), module, RiftGateModule::LO_OFFSET_PARAM));
		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 106.5f), module, RiftGateModule::LO_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 131.5f), module, RiftGateModule::LO_PARAM));

		addParam(createParamCentered<StoermelderTrimpot>(Vec(52.5f, 81.6f), module, RiftGateModule::HI_OFFSET_PARAM));
		addInput(createInputCentered<StoermelderPort>(Vec(52.5f, 106.5f), module, RiftGateModule::HI_INPUT));
		addParam(createParamCentered<StoermelderTrimpot>(Vec(52.5f, 131.5f), module, RiftGateModule::HI_PARAM));

		addInput(createInputCentered<StoermelderPort>(Vec(37.5f, 183.4f), module, RiftGateModule::IN_INPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(22.5f, 226.9f), module, RiftGateModule::OUTER_OUTPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(52.5f, 226.9f), module, RiftGateModule::INNER_OUTPUT));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 280.6f), module, RiftGateModule::OUTER_INPUT));
		addInput(createInputCentered<StoermelderPort>(Vec(52.5f, 280.6f), module, RiftGateModule::INNER_INPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(37.5f, 323.8f), module, RiftGateModule::OUT_OUTPUT));
	}
};

} // namespace RiftGateMk1

Model* modelRiftGateMk1 = createModel<RiftGate::RiftGateModule, RiftGate::RiftGateMk1Widget>("RiftGate-Mk1");