#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "Gamma/DFT.h"
#pragma GCC diagnostic pop

namespace Pitch {

// based on examples/spectral/pitchShift.cpp
struct PitchModule : Module {
	enum ParamIds {
		PARAM_SHIFT,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_SHIFT,
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

	PitchModule() :
		// STFT(winSize, hopSize, padSize, winType, sampType, auxBufs)
		stft(4096, 4096/4, 0, gam::HAMMING, gam::MAG_FREQ, 3)
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PARAM_SHIFT, -3.f, 3.f, 0.f, "Pitch shift");
		onReset();
		stft.precise(true);
	}

	void onSampleRateChange() override {
		gam::Domain::master().spu(APP->engine->getSampleRate());
	}

	void process(const ProcessArgs &args) override {
		if (inputs[INPUT].isConnected()) {
			float pshift = std::exp2(params[PARAM_SHIFT].getValue() + inputs[INPUT_SHIFT].getVoltage());
			float s = inputs[INPUT].getVoltage();

			if (stft(s)){
				enum {
					PREV_MAG=0,
					TEMP_MAG,
					TEMP_FRQ
				};

				// Compute spectral flux (L^1 norm on positive changes)
				float flux = 0;
				for (unsigned k=0; k<stft.numBins(); ++k){
					float mcurr = stft.bin(k)[0];
					float mprev = stft.aux(PREV_MAG)[k];

					if (mcurr > mprev){
						flux += mcurr - mprev;
					}
				}

				//printf("%g\n", flux);
				//gam::printPlot(flux); printf("\n");

				// Store magnitudes for next frame
				stft.copyBinsToAux(0, PREV_MAG);

				// Given an onset, we would like the phases of the output frame
				// to match the input frame in order to preserve transients.
				if (flux > 0.2){
					stft.resetPhases();
				}

				// Initialize buffers to store pitch-shifted spectrum
				for(unsigned k=0; k<stft.numBins(); ++k){
					stft.aux(TEMP_MAG)[k] = 0.;
					stft.aux(TEMP_FRQ)[k] = k*stft.binFreq();
				}

				// Perform the pitch shift:
				// Here we contract or expand the bins. For overlapping bins,
				// we simply add the magnitudes and replace the frequency.
				// Reference:
				// http://oldsite.dspdimension.com/dspdimension.com/src/smbPitchShift.cpp
				if (pshift > 0){
					unsigned kmax = stft.numBins() / pshift;
					if (kmax >= stft.numBins()) kmax = stft.numBins()-1;
					for (unsigned k=1; k<kmax; ++k){
						unsigned j = k*pshift;
						stft.aux(TEMP_MAG)[j] += stft.bin(k)[0];
						stft.aux(TEMP_FRQ)[j] = stft.bin(k)[1]*pshift;
					}
				}

				// Copy pitch-shifted spectrum over to bins
				stft.copyAuxToBins(TEMP_MAG, 0);
				stft.copyAuxToBins(TEMP_FRQ, 1);
			}

			s = stft();
			outputs[OUTPUT].setVoltage(s);
		}
	}
};

struct PitchWidget : ModuleWidget {
	PitchWidget(PitchModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Pitch.svg")));

		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<MyBlackScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<StoermelderTrimpot>(Vec(22.5f, 73.0f), module, PitchModule::PARAM_SHIFT));
		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 97.9f), module, PitchModule::INPUT_SHIFT));

		addInput(createInputCentered<StoermelderPort>(Vec(22.5f, 280.6f), module, PitchModule::INPUT));
		addOutput(createOutputCentered<StoermelderPort>(Vec(22.5f, 323.8f), module, PitchModule::OUTPUT));
	}
};

} // namespace Pitch

Model* modelPitch = createModel<Pitch::PitchModule, Pitch::PitchWidget>("Pitch");