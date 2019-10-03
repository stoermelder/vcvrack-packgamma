#include "plugin.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include <Gamma/DFT.h>
#include <Gamma/Oscillator.h>
#include <Gamma/Filter.h>
#pragma GCC diagnostic pop


const int WINDOW_SIZE = 2048;

struct FreezeMk1 : Module {
	enum ParamIds {
		HOP_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SRC_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    gam::STFT stft1;
	int captureCount;
	int size = WINDOW_SIZE/4;

	dsp::SchmittTrigger clockTrigger;

    FreezeMk1() :
		// STFT(winSize, hopSize, padSize, winType, spectralFormat)
		stft1(WINDOW_SIZE, size, 0, gam::HANN, gam::MAG_FREQ)
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(HOP_PARAM, 0.f, 7.f, 4.f, "Hop Size", "", 2.f, 16.f);

		//stft1.precise(true);
        onReset();
	}

  	void process(const ProcessArgs &args) override {
        gam::Domain::master().spu(args.sampleRate);
		
        float s = inputs[SRC_INPUT].getVoltage();
        s = rescale(s, -5.f, 5.f, -0.8f, 0.8f);

       	if (clockTrigger.process(inputs[TRIG_INPUT].getVoltage())) {
			captureCount=0;
			//size = 512; pow(2, params[HOP_PARAM].getValue()) * 16;
			//stft.sizeHop(size);
		}


		// Capture 4 hops to make up the size of one window
		if(captureCount<(WINDOW_SIZE/size) && stft1(s)) {
			captureCount++;
			if (captureCount<(WINDOW_SIZE/size)) 
				stft1.resetPhases();
		}

		// Resynthesis based on internal spectral data
		s = stft1();
		s = rescale(s, -0.8f, 0.8f, -5.f, 5.f);
		outputs[OUTPUT].setVoltage(s);
    }

	void onReset() override {
		Module::onReset();
		stft1.sizeHop(size);
		stft1.resetPhases();
		captureCount = 0;
	}
};

struct FreezeMk1Widget : ModuleWidget {
	FreezeMk1Widget(FreezeMk1 *module) {	
		setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Blank6.svg")));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.5f, 21.f)), module, FreezeMk1::SRC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.5f, 51.f)), module, FreezeMk1::TRIG_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.5f, 21.f)), module, FreezeMk1::OUTPUT));

		addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(18.5f, 51.f)), module, FreezeMk1::HOP_PARAM));
    }
};

Model *modelFreezeMk1 = createModel<FreezeMk1, FreezeMk1Widget>("FreezeMk1");