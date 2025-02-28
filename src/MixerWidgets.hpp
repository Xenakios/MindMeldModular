//***********************************************************************************************
//Mixer module for VCV Rack by Steve Baker and Marc Boulé 
//
//Based on code from the Fundamental plugin by Andrew Belt 
//See ./LICENSE.md for all licenses
//***********************************************************************************************

#ifndef MMM_MIXERWIDGETS_HPP
#define MMM_MIXERWIDGETS_HPP


#include "Mixer.hpp"
#include "MixerMenus.hpp"
#include <time.h>


static const NVGcolor DISP_COLORS[] = {
	nvgRGB(0xff, 0xd7, 0x14),// yellow
	nvgRGB(240, 240, 240),// light-gray			
	nvgRGB(140, 235, 107),// green
	nvgRGB(102, 245, 207),// aqua
	nvgRGB(102, 207, 245),// cyan
	nvgRGB(102, 183, 245),// blue
	nvgRGB(177, 107, 235)// purple
};

// --------------------
// VU meters
// --------------------

// Colors
static const int numThemes = 5;
static const NVGcolor VU_THEMES_TOP[numThemes][2] =  
									   {{nvgRGB(110, 130, 70), 	nvgRGB(178, 235, 107)}, // green: peak (darker), rms (lighter)
										{nvgRGB(68, 164, 122), 	nvgRGB(102, 245, 182)}, // teal: peak (darker), rms (lighter)
										{nvgRGB(64, 155, 160), 	nvgRGB(102, 233, 245)}, // light blue: peak (darker), rms (lighter)
										{nvgRGB(68, 125, 164), 	nvgRGB(102, 180, 245)}, // blue: peak (darker), rms (lighter)
										{nvgRGB(110, 70, 130), 	nvgRGB(178, 107, 235)}};// purple: peak (darker), rms (lighter)
static const NVGcolor VU_THEMES_BOT[numThemes][2] =  
									   {{nvgRGB(50, 130, 70), 	nvgRGB(97, 235, 107)}, // green: peak (darker), rms (lighter)
										{nvgRGB(68, 164, 156), 	nvgRGB(102, 245, 232)}, // teal: peak (darker), rms (lighter)
										{nvgRGB(64, 108, 160), 	nvgRGB(102, 183, 245)}, // light blue: peak (darker), rms (lighter)
										{nvgRGB(68,  92, 164), 	nvgRGB(102, 130, 245)}, // blue: peak (darker), rms (lighter)
										{nvgRGB(85,  70, 130), 	nvgRGB(135, 107, 235)}};// purple: peak (darker), rms (lighter)
static const NVGcolor VU_YELLOW[2] = {nvgRGB(136,136,37), nvgRGB(247, 216, 55)};// peak (darker), rms (lighter)
static const NVGcolor VU_ORANGE[2] = {nvgRGB(136,89,37), nvgRGB(238, 130, 47)};// peak (darker), rms (lighter)
static const NVGcolor VU_RED[2] =    {nvgRGB(136, 37, 37), 	nvgRGB(229, 34, 38)};// peak (darker), rms (lighter)

static const float sepYtrack = 0.3f * SVG_DPI / MM_PER_IN;// height of separator at 0dB. See include/app/common.hpp for constants
static const float sepYmaster = 0.4f * SVG_DPI / MM_PER_IN;// height of separator at 0dB. See include/app/common.hpp for constants


// Base struct
struct VuMeterBase : OpaqueWidget {
	static constexpr float epsilon = 0.0001f;// don't show VUs below 0.1mV
	static constexpr float peakHoldThick = 1.0f;// in px
	
	// instantiator must setup:
	VuMeterAllDual *srcLevels;// from 0 to 10 V, with 10 V = 0dB (since -10 to 10 is the max)
	int8_t *colorThemeGlobal;
	int8_t *colorThemeLocal;
	
	// derived class must setup:
	float gapX;// in px
	float barX;// in px
	float barY;// in px
	// box.size // inherited from OpaqueWidget, no need to declare
	float faderMaxLinearGain;
	float faderScalingExponent;
	float zeroDbVoltage;
	
	// local 
	float peakHold[2] = {0.0f, 0.0f};
	long oldTime = -1;
	float yellowThreshold;// in px, before vertical inversion
	float redThreshold;// in px, before vertical inversion
	int colorTheme;
	
	
	VuMeterBase() {
		faderMaxLinearGain = GlobalInfo::trkAndGrpFaderMaxLinearGain;
		faderScalingExponent = GlobalInfo::trkAndGrpFaderScalingExponent;
	}
	
	void prepareYellowAndRedThresholds(float yellowMinDb, float redMinDb) {
		float maxLin = std::pow(faderMaxLinearGain, 1.0f / faderScalingExponent);
		float yellowLin = std::pow(std::pow(10.0f, yellowMinDb / 20.0f), 1.0f / faderScalingExponent);
		yellowThreshold = barY * (yellowLin / maxLin);
		float redLin = std::pow(std::pow(10.0f, redMinDb / 20.0f), 1.0f / faderScalingExponent);
		redThreshold = barY * (redLin / maxLin);
	}
	
	
	void processPeakHold() {
		long newTime = time(0);
		if ( (newTime != oldTime) && ((newTime & 0x1) == 0) ) {
			oldTime = newTime;
			peakHold[0] = 0.0f;
			peakHold[1] = 0.0f;
		}		
		for (int i = 0; i < 2; i++) {
			if (srcLevels->getPeak(i) > peakHold[i]) {
				peakHold[i] = srcLevels->getPeak(i);
			}
		}
	}

	
	void draw(const DrawArgs &args) override {
		processPeakHold();
		
		colorTheme = (*colorThemeGlobal >= numThemes) ? *colorThemeLocal : *colorThemeGlobal;
		
		// PEAK
		drawVu(args, srcLevels->getPeak(0), 0, 0);
		drawVu(args, srcLevels->getPeak(1), barX + gapX, 0);

		// RMS
		drawVu(args, srcLevels->getRms(0), 0, 1);
		drawVu(args, srcLevels->getRms(1), barX + gapX, 1);
		
		// PEAK_HOLD
		drawPeakHold(args, peakHold[0], 0);
		drawPeakHold(args, peakHold[1], barX + gapX);
				
		Widget::draw(args);
	}
	
	// used for RMS or PEAK
	virtual void drawVu(const DrawArgs &args, float vuValue, float posX, int colorIndex) {};

	virtual void drawPeakHold(const DrawArgs &args, float holdValue, float posX) {};
};

// 2.8mm x 42mm VU for tracks and groups
// --------------------

struct VuMeterTrack : VuMeterBase {//
	VuMeterTrack() {
		gapX = mm2px(0.4);
		barX = mm2px(1.2);
		barY = mm2px(42.0);
		box.size = Vec(barX * 2 + gapX, barY);
		zeroDbVoltage = 5.0f;// V
		prepareYellowAndRedThresholds(-6.0f, 0.0f);// dB
	}

	// used for RMS or PEAK
	void drawVu(const DrawArgs &args, float vuValue, float posX, int colorIndex) override {
		if (vuValue >= epsilon) {

			float vuHeight = vuValue / (faderMaxLinearGain * zeroDbVoltage);
			vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
			vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
			vuHeight *= barY;

			NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY, VU_THEMES_TOP[colorTheme][colorIndex], VU_THEMES_BOT[colorTheme][colorIndex]);
			if (vuHeight > redThreshold) {
				// Yellow-Red gradient
				NVGpaint gradTop = nvgLinearGradient(args.vg, 0, 0, 0, barY - redThreshold - sepYtrack, VU_RED[colorIndex], VU_YELLOW[colorIndex]);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight - sepYtrack, barX, vuHeight - redThreshold);
				nvgFillPaint(args.vg, gradTop);
				nvgFill(args.vg);
				// Green
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - redThreshold, barX, redThreshold);
				//nvgFillColor(args.vg, VU_GREEN[colorIndex]);
				nvgFillPaint(args.vg, gradGreen);
				nvgFill(args.vg);			
			}
			else {
				// Green
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
				//nvgFillColor(args.vg, VU_GREEN[colorIndex]);
				nvgFillPaint(args.vg, gradGreen);
				nvgFill(args.vg);
			}

		}
	}
	
	void drawPeakHold(const DrawArgs &args, float holdValue, float posX) override {
		if (holdValue >= epsilon) {
			float vuHeight = holdValue / (faderMaxLinearGain * zeroDbVoltage);
			vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
			vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
			vuHeight *= barY;
			
			if (vuHeight > redThreshold) {
				// Yellow-Red gradient
				NVGpaint gradTop = nvgLinearGradient(args.vg, 0, 0, 0, barY - redThreshold - sepYtrack, VU_RED[1], VU_YELLOW[1]);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight - sepYtrack - peakHoldThick, barX, peakHoldThick);
				nvgFillPaint(args.vg, gradTop);
				nvgFill(args.vg);	
			}
			else {
				// Green
				NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY, VU_THEMES_TOP[colorTheme][1], VU_THEMES_BOT[colorTheme][1]);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight, barX, peakHoldThick);
				nvgFillPaint(args.vg, gradGreen);
				//nvgFillColor(args.vg, VU_GREEN[1]);
				nvgFill(args.vg);
			}
		}		
	}
};



// 3.8mm x 60mm VU for master
// --------------------

struct VuMeterMaster : VuMeterBase {
	int* clippingPtr;
	int oldClipping = 1;
	float hardRedVoltage;
	
	VuMeterMaster() {
		gapX = mm2px(0.6);
		barX = mm2px(1.6);
		barY = mm2px(60.0);
		box.size = Vec(barX * 2 + gapX, barY);
		zeroDbVoltage = 10.0f;// V
		hardRedVoltage = 10.0f;
		prepareYellowAndRedThresholds(-6.0f, 0.0f);// dB
	}
	
	// used for RMS or PEAK
	void drawVu(const DrawArgs &args, float vuValue, float posX, int colorIndex) override {
		if (posX == 0) { // draw the separator for master since depends on softclip on/off. draw only once per vu pair
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0 - 1, barY - redThreshold - sepYmaster, box.size.x + 2, sepYmaster);
			nvgFillColor(args.vg, nvgRGB(53, 53, 53));
			nvgFill(args.vg);
		}

		if (vuValue >= epsilon) {
			float vuHeight = vuValue / (faderMaxLinearGain * zeroDbVoltage);
			vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
			vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
			vuHeight *= barY;
			
			float peakHoldVal = (posX == 0 ? peakHold[0] : peakHold[1]);
			if (vuHeight > redThreshold || peakHoldVal > hardRedVoltage) {
				// Full red
				nvgBeginPath(args.vg);
				if (vuHeight > redThreshold) {
					nvgRect(args.vg, posX, barY - vuHeight - sepYmaster, barX, vuHeight - redThreshold);
					nvgRect(args.vg, posX, barY - redThreshold, barX, redThreshold);
				}
				else {
					nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
				}
				nvgFillColor(args.vg, VU_RED[colorIndex]);
				nvgFill(args.vg);
			}
			else {
				NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - yellowThreshold, 0, barY, VU_THEMES_TOP[colorTheme][colorIndex], VU_THEMES_BOT[colorTheme][colorIndex]);
				if (vuHeight > yellowThreshold) {
					// Yellow-Orange gradient
					NVGpaint gradTop = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY - yellowThreshold, VU_ORANGE[colorIndex], VU_YELLOW[colorIndex]);
					nvgBeginPath(args.vg);
					nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight - yellowThreshold);
					nvgFillPaint(args.vg, gradTop);
					nvgFill(args.vg);
					// Green
					nvgBeginPath(args.vg);
					nvgRect(args.vg, posX, barY - yellowThreshold, barX, yellowThreshold);
					//nvgFillColor(args.vg, VU_GREEN[colorIndex]);
					nvgFillPaint(args.vg, gradGreen);
					nvgFill(args.vg);			
				}
				else {
					// Green
					nvgBeginPath(args.vg);
					nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
					//nvgFillColor(args.vg, VU_GREEN[colorIndex]);
					nvgFillPaint(args.vg, gradGreen);
					nvgFill(args.vg);
				}
			}
		}
	}	
	
	void drawPeakHold(const DrawArgs &args, float holdValue, float posX) override {
		if (holdValue >= epsilon) {
			float vuHeight = holdValue / (faderMaxLinearGain * zeroDbVoltage);
			vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
			vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
			vuHeight *= barY;
			
			float peakHoldVal = (posX == 0 ? peakHold[0] : peakHold[1]);
			if (vuHeight > redThreshold || peakHoldVal > hardRedVoltage) {
				// Full red
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight - sepYmaster - peakHoldThick, barX, peakHoldThick);
				nvgFillColor(args.vg, VU_RED[1]);
				nvgFill(args.vg);
			} else if (vuHeight > yellowThreshold) {
				// Yellow-Orange gradient
				NVGpaint gradTop = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY - yellowThreshold, VU_ORANGE[1], VU_YELLOW[1]);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight, barX, 1.0);
				nvgFillPaint(args.vg, gradTop);
				nvgFill(args.vg);	
			}
			else {
				// Green
				NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - yellowThreshold, 0, barY, VU_THEMES_TOP[colorTheme][1], VU_THEMES_BOT[colorTheme][1]);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight, barX, 1.0);
				//nvgFillColor(args.vg, VU_GREEN[1]);
				nvgFillPaint(args.vg, gradGreen);
				nvgFill(args.vg);
			}
		}
	}
	void step() override {
		if (*clippingPtr != oldClipping) {
			oldClipping = *clippingPtr;
			if (*clippingPtr == 0) {// soft
				prepareYellowAndRedThresholds(-4.43697499f, 1.58362492f);// dB (6V and 12V respectively)
				hardRedVoltage = 12.0f;
			}
			else {// hard
				prepareYellowAndRedThresholds(-6.0f, 0.0f);// dB (5V and 0V respectively)
				hardRedVoltage = 10.0f;
			}
		}
	}
};



// 2.8mm x 30mm VU for aux returns
// --------------------

struct VuMeterAux : VuMeterTrack {//
	VuMeterAux() {
		barY = mm2px(30.0);
		box.size = Vec(barX * 2 + gapX, barY);
		prepareYellowAndRedThresholds(-6.0f, 0.0f);// dB
	}
};



// Fade pointer
// --------------------

static const float prtHeight = 2.72f  * SVG_DPI / MM_PER_IN;// height of pointer, width is determined by box.size.x in derived struct
static const NVGcolor FADE_POINTER_FILL = nvgRGB(255, 106, 31);

struct CvAndFadePointerBase : OpaqueWidget {
	// instantiator must setup:
	Param *srcParam;// to know where the fader is
	float *srcParamWithCV = NULL;// for cv pointer, NULL indicates when cv pointers are not used (no cases so far)
	PackedBytes4* colorAndCloak;// cv pointers have same color as displays
	float *srcFadeGain = NULL;// to know where to position the pointer, NULL indicates when fader pointers are not used (aux ret faders for example)
	float *srcFadeRate;// mute when < minFadeRate, fade when >= minFadeRate
	int8_t* dispColorLocalPtr;
	
	// derived class must setup:
	// box.size // inherited from OpaqueWidget, no need to declare
	float faderMaxLinearGain;
	int faderScalingExponent;
	
	// local 
	float maxTFader;

	
	void prepareMaxFader() {
		maxTFader = std::pow(faderMaxLinearGain, 1.0f / faderScalingExponent);
	}


	void draw(const DrawArgs &args) override {
		// cv pointer (draw only when cv has en effect)
		if (srcParamWithCV != NULL && *srcParamWithCV != -1.0f && (colorAndCloak->cc4[detailsShow] & ~colorAndCloak->cc4[cloakedMode] & 0x4) != 0) {// -1.0f indicates not to show cv pointer
			float cvPosNormalized = *srcParamWithCV / maxTFader;
			float vertPos = box.size.y - box.size.y * cvPosNormalized;// in px
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, 0, vertPos - prtHeight / 2.0f);
			nvgLineTo(args.vg, box.size.x, vertPos);
			nvgLineTo(args.vg, 0, vertPos + prtHeight / 2.0f);
			nvgClosePath(args.vg);
			int colorIndex = (colorAndCloak->cc4[dispColor] < 7) ? colorAndCloak->cc4[dispColor] : (*dispColorLocalPtr);
			nvgFillColor(args.vg, DISP_COLORS[colorIndex]);
			nvgFill(args.vg);
			nvgStrokeColor(args.vg, SCHEME_BLACK);
			nvgStrokeWidth(args.vg, mm2px(0.11f));
			nvgStroke(args.vg);
		}
		// fade pointer (draw only when in mute mode, or when in fade mode and less than unity gain)
		if (srcFadeGain != NULL && *srcFadeRate >= GlobalInfo::minFadeRate && *srcFadeGain < 1.0f  && colorAndCloak->cc4[cloakedMode] == 0) {
			float fadePosNormalized;
			if (srcParamWithCV == NULL || *srcParamWithCV == -1.0f) {
				fadePosNormalized = srcParam->getValue();
			}
			else {
				fadePosNormalized = *srcParamWithCV;
			}
			fadePosNormalized /= maxTFader;
			float vertPos = box.size.y - box.size.y * fadePosNormalized * (*srcFadeGain);// in px
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, 0, vertPos - prtHeight / 2.0f);
			nvgLineTo(args.vg, box.size.x, vertPos);
			nvgLineTo(args.vg, 0, vertPos + prtHeight / 2.0f);
			nvgClosePath(args.vg);
			nvgFillColor(args.vg, FADE_POINTER_FILL);
			nvgFill(args.vg);
			nvgStrokeColor(args.vg, SCHEME_BLACK);
			nvgStrokeWidth(args.vg, mm2px(0.11f));
			nvgStroke(args.vg);
		}
	}	
};


struct CvAndFadePointerTrack : CvAndFadePointerBase {
	CvAndFadePointerTrack() {
		box.size = mm2px(math::Vec(2.24, 42));
		faderMaxLinearGain = GlobalInfo::trkAndGrpFaderMaxLinearGain;
		faderScalingExponent = GlobalInfo::trkAndGrpFaderScalingExponent;
		prepareMaxFader();
	}
};
struct CvAndFadePointerGroup : CvAndFadePointerBase {
	CvAndFadePointerGroup() {
		box.size = mm2px(math::Vec(2.24, 42));
		faderMaxLinearGain = GlobalInfo::trkAndGrpFaderMaxLinearGain;
		faderScalingExponent = GlobalInfo::trkAndGrpFaderScalingExponent;
		prepareMaxFader();
	}
};
struct CvAndFadePointerMaster : CvAndFadePointerBase {
	CvAndFadePointerMaster() {
		box.size = mm2px(math::Vec(2.24, 60));
		faderMaxLinearGain = MixerMaster::masterFaderMaxLinearGain;
		faderScalingExponent = MixerMaster::masterFaderScalingExponent;
		prepareMaxFader();
	}
};
struct CvAndFadePointerAuxRet : CvAndFadePointerBase {
	CvAndFadePointerAuxRet() {
		box.size = mm2px(math::Vec(2.24, 30));
		faderMaxLinearGain = GlobalInfo::globalAuxReturnMaxLinearGain;
		faderScalingExponent = GlobalInfo::globalAuxReturnScalingExponent;
		prepareMaxFader();
	}
};



	

// --------------------
// Displays 
// --------------------

static const Vec DISP_SIZE = Vec(38, 16);
static const Vec DISP_OFFSET = Vec(2.6f, -2.2f);


// Non-Editable track and group (used by auxspander)
// --------------------

struct TrackAndGroupLabel : LedDisplayChoice {
	int8_t* dispColorPtr = NULL;
	int8_t* dispColorLocalPtr;
	
	TrackAndGroupLabel() {
		box.size = DISP_SIZE;
		textOffset = Vec(7.5, 12);
		text = "-00-";
	};
	
	void draw(const DrawArgs &args) override {
		if (dispColorPtr) {
			int colorIndex = *dispColorPtr < 7 ? *dispColorPtr : *dispColorLocalPtr;
			color = DISP_COLORS[colorIndex];
		}	
		LedDisplayChoice::draw(args);
	}
};

// Editable track and group displays base struct
// --------------------

struct EditableDisplayBase : LedDisplayTextField {
	int numChars = 4;
	int textSize = 12;
	bool doubleClick = false;
	GlobalInfo *gInfo = NULL;
	PackedBytes4* colorAndCloak = NULL; // make this separate so that we can use EditableDisplayBase for Aux displays
	int8_t* dispColorLocal;

	EditableDisplayBase() {
		box.size = DISP_SIZE;
		textOffset = DISP_OFFSET;
		text = "-00-";
	};
	
	// don't want background so implement adapted version here
	void draw(const DrawArgs &args) override {
		if (colorAndCloak) {
			int colorIndex = colorAndCloak->cc4[dispColor] < 7 ? colorAndCloak->cc4[dispColor] : *dispColorLocal;
			color = DISP_COLORS[colorIndex];
		}
		if (cursor > numChars) {
			text.resize(numChars);
			cursor = numChars;
			selection = numChars;
		}
		
		// the code below is LedDisplayTextField.draw() without the background rect
		nvgScissor(args.vg, RECT_ARGS(args.clipBox));
		if (font->handle >= 0) {
			bndSetFont(font->handle);

			NVGcolor highlightColor = color;
			highlightColor.a = 0.5;
			int begin = std::min(cursor, selection);
			int end = (this == APP->event->selectedWidget) ? std::max(cursor, selection) : -1;
			bndIconLabelCaret(args.vg, textOffset.x, textOffset.y,
				box.size.x - 2*textOffset.x, box.size.y - 2*textOffset.y,
				-1, color, textSize, text.c_str(), highlightColor, begin, end);

			bndSetFont(APP->window->uiFont->handle);
		}
		nvgResetScissor(args.vg);
	}
	
	// don't want spaces since leading spaces are stripped by nanovg (which oui-blendish calls), so convert to dashes
	void onSelectText(const event::SelectText &e) override {
		if (e.codepoint < 128) {
			char letter = (char) e.codepoint;
			if (letter == 0x20) {// space
				letter = 0x2D;// hyphen
			}
			std::string newText(1, letter);
			insertText(newText);
		}
		e.consume(this);	
		
		if (text.length() > (unsigned)numChars) {
			text = text.substr(0, numChars);
		}
	}
	
	void onDoubleClick(const event::DoubleClick& e) override {
		doubleClick = true;
	}
	
	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_RELEASE) {
			if (doubleClick) {
				doubleClick = false;
				selectAll();
			}
		}
		LedDisplayTextField::onButton(e);
	}

}; 


// Master display editable label with menu
// --------------------

struct MasterDisplay : EditableDisplayBase {
	MixerMaster *srcMaster;
	
	MasterDisplay() {
		numChars = 6;
		textSize = 13;
		box.size.x = mm2px(18.3f);
		textOffset.x = 1.4f;// 2.4f
		text = "-0000-";
	}
	
	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			ui::Menu *menu = createMenu();

			MenuLabel *mastSetLabel = new MenuLabel();
			mastSetLabel->text = "Master settings: ";
			menu->addChild(mastSetLabel);
			
			FadeRateSlider *fadeSlider = new FadeRateSlider(&(srcMaster->fadeRate));
			fadeSlider->box.size.x = 200.0f;
			menu->addChild(fadeSlider);
			
			FadeProfileSlider *fadeProfSlider = new FadeProfileSlider(&(srcMaster->fadeProfile));
			fadeProfSlider->box.size.x = 200.0f;
			menu->addChild(fadeProfSlider);
			
			DimGainSlider *dimSliderItem = new DimGainSlider(srcMaster);
			dimSliderItem->box.size.x = 200.0f;
			menu->addChild(dimSliderItem);
			
			DcBlockItem *dcItem = createMenuItem<DcBlockItem>("DC blocker", CHECKMARK(srcMaster->dcBlock));
			dcItem->srcMaster = srcMaster;
			menu->addChild(dcItem);
			
			ClippingItem *clipItem = createMenuItem<ClippingItem>("Clipping", RIGHT_ARROW);
			clipItem->srcMaster = srcMaster;
			menu->addChild(clipItem);
				
			if (srcMaster->gInfo->colorAndCloak.cc4[vuColorGlobal] >= numThemes) {	
				VuColorItem *vuColItem = createMenuItem<VuColorItem>("VU Colour", RIGHT_ARROW);
				vuColItem->srcColor = &(srcMaster->vuColorThemeLocal);
				vuColItem->isGlobal = false;
				menu->addChild(vuColItem);
			}
			
			if (srcMaster->gInfo->colorAndCloak.cc4[dispColor] >= 7) {
				DispColorItem *dispColItem = createMenuItem<DispColorItem>("Display colour", RIGHT_ARROW);
				dispColItem->srcColor = &(srcMaster->dispColorLocal);
				dispColItem->isGlobal = false;
				menu->addChild(dispColItem);
			}
			
			e.consume(this);
			return;
		}
		EditableDisplayBase::onButton(e);		
	}
	void onChange(const event::Change &e) override {
		snprintf(srcMaster->masterLabel, 7, "%s", text.c_str());
		EditableDisplayBase::onChange(e);
	};
};



// Track display editable label with menu
// --------------------

struct TrackDisplay : EditableDisplayBase {
	MixerTrack *tracks = NULL;
	int trackNumSrc;
	int *updateTrackLabelRequestPtr;
	int *trackMoveInAuxRequestPtr;
	PortWidget **inputWidgets;
	bool *auxExpanderPresentPtr;

	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			ui::Menu *menu = createMenu();
			MixerTrack *srcTrack = &(tracks[trackNumSrc]);

			MenuLabel *trkSetLabel = new MenuLabel();
			trkSetLabel->text = "Track settings: " + std::string(srcTrack->trackName, 4);
			menu->addChild(trkSetLabel);
			
			GainAdjustSlider *trackGainAdjustSlider = new GainAdjustSlider(srcTrack);
			trackGainAdjustSlider->box.size.x = 200.0f;
			menu->addChild(trackGainAdjustSlider);
			
			HPFCutoffSlider<MixerTrack> *trackHPFAdjustSlider = new HPFCutoffSlider<MixerTrack>(srcTrack);
			trackHPFAdjustSlider->box.size.x = 200.0f;
			menu->addChild(trackHPFAdjustSlider);
			
			LPFCutoffSlider<MixerTrack> *trackLPFAdjustSlider = new LPFCutoffSlider<MixerTrack>(srcTrack);
			trackLPFAdjustSlider->box.size.x = 200.0f;
			menu->addChild(trackLPFAdjustSlider);
			
			FadeRateSlider *fadeSlider = new FadeRateSlider(srcTrack->fadeRate);
			fadeSlider->box.size.x = 200.0f;
			menu->addChild(fadeSlider);
			
			FadeProfileSlider *fadeProfSlider = new FadeProfileSlider(&(srcTrack->fadeProfile));
			fadeProfSlider->box.size.x = 200.0f;
			menu->addChild(fadeProfSlider);
			
			LinkFaderItem<MixerTrack> *linkFadItem = createMenuItem<LinkFaderItem<MixerTrack>>("Link fader and fade", CHECKMARK(srcTrack->isLinked()));
			linkFadItem->srcTrkGrp = srcTrack;
			menu->addChild(linkFadItem);
			
			if (srcTrack->gInfo->directOutsMode >= 4) {
				TapModeItem *directOutsItem = createMenuItem<TapModeItem>("Direct outs", RIGHT_ARROW);
				directOutsItem->tapModePtr = &(srcTrack->directOutsMode);
				directOutsItem->isGlobal = false;
				menu->addChild(directOutsItem);
			}

			if (srcTrack->gInfo->filterPos >= 2) {
				FilterPosItem *filterPosItem = createMenuItem<FilterPosItem>("Filters", RIGHT_ARROW);
				filterPosItem->filterPosSrc = &(srcTrack->filterPos);
				filterPosItem->isGlobal = false;
				menu->addChild(filterPosItem);
			}

			if (srcTrack->gInfo->auxSendsMode >= 4 && *auxExpanderPresentPtr) {
				TapModeItem *auxSendsItem = createMenuItem<TapModeItem>("Aux sends", RIGHT_ARROW);
				auxSendsItem->tapModePtr = &(srcTrack->auxSendsMode);
				auxSendsItem->isGlobal = false;
				menu->addChild(auxSendsItem);
			}

			if (srcTrack->gInfo->panLawStereo >= 3) {
				PanLawStereoItem *panLawStereoItem = createMenuItem<PanLawStereoItem>("Stereo pan mode", RIGHT_ARROW);
				panLawStereoItem->panLawStereoSrc = &(srcTrack->panLawStereo);
				panLawStereoItem->isGlobal = false;
				menu->addChild(panLawStereoItem);
			}

			if (srcTrack->gInfo->colorAndCloak.cc4[vuColorGlobal] >= numThemes) {	
				VuColorItem *vuColItem = createMenuItem<VuColorItem>("VU Colour", RIGHT_ARROW);
				vuColItem->srcColor = &(srcTrack->vuColorThemeLocal);
				vuColItem->isGlobal = false;
				menu->addChild(vuColItem);
			}

			if (srcTrack->gInfo->colorAndCloak.cc4[dispColor] >= 7) {
				DispColorItem *dispColItem = createMenuItem<DispColorItem>("Display colour", RIGHT_ARROW);
				dispColItem->srcColor = &(srcTrack->dispColorLocal);
				dispColItem->isGlobal = false;
				menu->addChild(dispColItem);
			}
			
			menu->addChild(new MenuLabel());// empty line

			MenuLabel *settingsALabel = new MenuLabel();
			settingsALabel->text = "Actions: " + std::string(srcTrack->trackName, 4);
			menu->addChild(settingsALabel);

			CopyTrackSettingsItem *copyItem = createMenuItem<CopyTrackSettingsItem>("Copy track menu settings to:", RIGHT_ARROW);
			copyItem->tracks = tracks;
			copyItem->trackNumSrc = trackNumSrc;
			menu->addChild(copyItem);
			
			TrackReorderItem *reodrerItem = createMenuItem<TrackReorderItem>("Move to:", RIGHT_ARROW);
			reodrerItem->tracks = tracks;
			reodrerItem->trackNumSrc = trackNumSrc;
			reodrerItem->updateTrackLabelRequestPtr = updateTrackLabelRequestPtr;
			reodrerItem->trackMoveInAuxRequestPtr = trackMoveInAuxRequestPtr;
			reodrerItem->inputWidgets = inputWidgets;
			menu->addChild(reodrerItem);
			
			e.consume(this);
			return;
		}
		EditableDisplayBase::onButton(e);
	}
	void onChange(const event::Change &e) override {
		(*((uint32_t*)(tracks[trackNumSrc].trackName))) = 0x20202020;
		for (int i = 0; i < std::min(4, (int)text.length()); i++) {
			tracks[trackNumSrc].trackName[i] = text[i];
		}
		EditableDisplayBase::onChange(e);
	};
};


// Group display editable label with menu
// --------------------

struct GroupDisplay : EditableDisplayBase {
	MixerGroup *srcGroup = NULL;
	bool *auxExpanderPresentPtr;

	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			ui::Menu *menu = createMenu();

			MenuLabel *grpSetLabel = new MenuLabel();
			grpSetLabel->text = "Group settings: " + std::string(srcGroup->groupName, 4);
			menu->addChild(grpSetLabel);
			
			FadeRateSlider *fadeSlider = new FadeRateSlider(srcGroup->fadeRate);
			fadeSlider->box.size.x = 200.0f;
			menu->addChild(fadeSlider);
			
			FadeProfileSlider *fadeProfSlider = new FadeProfileSlider(&(srcGroup->fadeProfile));
			fadeProfSlider->box.size.x = 200.0f;
			menu->addChild(fadeProfSlider);
			
			LinkFaderItem<MixerGroup> *linkFadItem = createMenuItem<LinkFaderItem<MixerGroup>>("Link fader and fade", CHECKMARK(srcGroup->isLinked()));
			linkFadItem->srcTrkGrp = srcGroup;
			menu->addChild(linkFadItem);
			
			if (srcGroup->gInfo->directOutsMode >= 4) {
				TapModeItem *directOutsItem = createMenuItem<TapModeItem>("Direct outs", RIGHT_ARROW);
				directOutsItem->tapModePtr = &(srcGroup->directOutsMode);
				directOutsItem->isGlobal = false;
				menu->addChild(directOutsItem);
			}

			if (srcGroup->gInfo->auxSendsMode >= 4 && *auxExpanderPresentPtr) {
				TapModeItem *auxSendsItem = createMenuItem<TapModeItem>("Aux sends", RIGHT_ARROW);
				auxSendsItem->tapModePtr = &(srcGroup->auxSendsMode);
				auxSendsItem->isGlobal = false;
				menu->addChild(auxSendsItem);
			}

			if (srcGroup->gInfo->panLawStereo >= 3) {
				PanLawStereoItem *panLawStereoItem = createMenuItem<PanLawStereoItem>("Stereo pan mode", RIGHT_ARROW);
				panLawStereoItem->panLawStereoSrc = &(srcGroup->panLawStereo);
				panLawStereoItem->isGlobal = false;
				menu->addChild(panLawStereoItem);
			}

			if (srcGroup->gInfo->colorAndCloak.cc4[vuColorGlobal] >= numThemes) {	
				VuColorItem *vuColItem = createMenuItem<VuColorItem>("VU Colour", RIGHT_ARROW);
				vuColItem->srcColor = &(srcGroup->vuColorThemeLocal);
				vuColItem->isGlobal = false;
				menu->addChild(vuColItem);
			}
			
			if (srcGroup->gInfo->colorAndCloak.cc4[dispColor] >= 7) {
				DispColorItem *dispColItem = createMenuItem<DispColorItem>("Display colour", RIGHT_ARROW);
				dispColItem->srcColor = &(srcGroup->dispColorLocal);
				dispColItem->isGlobal = false;
				menu->addChild(dispColItem);
			}
			
			e.consume(this);
			return;
		}
		EditableDisplayBase::onButton(e);
	}
	void onChange(const event::Change &e) override {
		(*((uint32_t*)(srcGroup->groupName))) = 0x20202020;
		for (int i = 0; i < std::min(4, (int)text.length()); i++) {
			srcGroup->groupName[i] = text[i];
		}
		EditableDisplayBase::onChange(e);
	};
};


// Aux display editable label with menu
// --------------------

struct AuxDisplay : EditableDisplayBase {
	AuxspanderAux *srcAux = NULL;
	int8_t* srcVuColor = NULL;
	int8_t* srcDispColor = NULL;
	int8_t* srcDirectOutsModeLocal = NULL;
	int8_t* srcPanLawStereoLocal = NULL;
	int8_t* srcDirectOutsModeGlobal = NULL;
	int8_t* srcPanLawStereoGlobal = NULL;
	char* auxName;
	int auxNumber = 0;
	
	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			ui::Menu *menu = createMenu();

			MenuLabel *auxSetLabel = new MenuLabel();
			auxSetLabel->text = "Aux settings: " + text;
			menu->addChild(auxSetLabel);
			
			HPFCutoffSlider<AuxspanderAux> *auxHPFAdjustSlider = new HPFCutoffSlider<AuxspanderAux>(srcAux);
			auxHPFAdjustSlider->box.size.x = 200.0f;
			menu->addChild(auxHPFAdjustSlider);
			
			LPFCutoffSlider<AuxspanderAux> *auxLPFAdjustSlider = new LPFCutoffSlider<AuxspanderAux>(srcAux);
			auxLPFAdjustSlider->box.size.x = 200.0f;
			menu->addChild(auxLPFAdjustSlider);
			
			
			if (*srcDirectOutsModeGlobal >= 4) {
				TapModeItem *directOutsItem = createMenuItem<TapModeItem>("Direct outs", RIGHT_ARROW);
				directOutsItem->tapModePtr = srcDirectOutsModeLocal;
				directOutsItem->isGlobal = false;
				menu->addChild(directOutsItem);
			}

			if (*srcPanLawStereoGlobal >= 3) {
				PanLawStereoItem *panLawStereoItem = createMenuItem<PanLawStereoItem>("Stereo pan mode", RIGHT_ARROW);
				panLawStereoItem->panLawStereoSrc = srcPanLawStereoLocal;
				panLawStereoItem->isGlobal = false;
				menu->addChild(panLawStereoItem);
			}

			if (colorAndCloak->cc4[vuColorGlobal] >= numThemes) {	
				VuColorItem *vuColItem = createMenuItem<VuColorItem>("VU Colour", RIGHT_ARROW);
				vuColItem->srcColor = srcVuColor;
				vuColItem->isGlobal = false;
				menu->addChild(vuColItem);
			}
			
			if (colorAndCloak->cc4[dispColor] >= 7) {
				DispColorItem *dispColItem = createMenuItem<DispColorItem>("Display colour", RIGHT_ARROW);
				dispColItem->srcColor = srcDispColor;
				dispColItem->isGlobal = false;
				menu->addChild(dispColItem);
			}
			
			e.consume(this);
			return;
		}
		EditableDisplayBase::onButton(e);
	}
	void onChange(const event::Change &e) override {
		(*((uint32_t*)(auxName))) = 0x20202020;
		for (int i = 0; i < std::min(4, (int)text.length()); i++) {
			auxName[i] = text[i];
		}
		EditableDisplayBase::onChange(e);
	};
};


// Group select parameter with non-editable label, but will respond to hover key press
// --------------------

struct GroupSelectDisplay : ParamWidget {
	PackedBytes4 *srcColor = NULL;
	int8_t* srcColorLocal;
	int oldDispColor = -1;
	LedDisplayChoice ldc;
	
	GroupSelectDisplay() {
		box.size = Vec(18, 16);
		ldc.box.size = box.size;
		ldc.textOffset = math::Vec(6.6f, 11.7f);
		ldc.bgColor.a = 0.0f;
		ldc.text = "-";
	};
	
	void draw(const DrawArgs &args) override {
		int grp = 0;
		if (paramQuantity) {
			grp = (int)(paramQuantity->getValue() + 0.5f);
		}
		ldc.text[0] = (char)(grp >= 1 &&  grp <= 4 ? grp + 0x30 : '-');
		ldc.text[1] = 0;
		if (srcColor) {
			int colorIndex = srcColor->cc4[dispColor] < 7 ? srcColor->cc4[dispColor] : *srcColorLocal;
			if (colorIndex != oldDispColor) {
				ldc.color = DISP_COLORS[colorIndex];
				// arcColor = DISP_COLORS[colorIndex];// arc color, same as displays
				// arcColorDarker = nvgRGB(greyArc, greyArc, greyArc);//calcArcColorDarker(arcCvScale);
				oldDispColor = colorIndex;
			}
		}
		ldc.draw(args);
		ParamWidget::draw(args);
	};

	void onHoverKey(const event::HoverKey &e) override {
		if (paramQuantity) {
			if (e.action == GLFW_PRESS) {
				if (e.key >= GLFW_KEY_1 && e.key <= GLFW_KEY_4) {
					paramQuantity->setValue((float)(e.key - GLFW_KEY_0));
				}
				else if (e.key >= GLFW_KEY_KP_1 && e.key <= GLFW_KEY_KP_4) {
					paramQuantity->setValue((float)(e.key - GLFW_KEY_KP_0));
				}
				else if ( ((e.mods & RACK_MOD_MASK) == 0) && (
						(e.key >= GLFW_KEY_A && e.key <= GLFW_KEY_Z) ||
						e.key == GLFW_KEY_SPACE || e.key == GLFW_KEY_MINUS || 
						e.key == GLFW_KEY_0 || e.key == GLFW_KEY_KP_0 || 
						(e.key >= GLFW_KEY_5 && e.key <= GLFW_KEY_9) || 
						(e.key >= GLFW_KEY_KP_5 && e.key <= GLFW_KEY_KP_9) ) ){
					paramQuantity->setValue(0.0f);
				}
			}
		}
	}
	
	void reset() override {
		if (paramQuantity) {
			paramQuantity->reset();
		}
	}

	void randomize() override {
		if (paramQuantity) {
			float value = paramQuantity->getMinValue() + std::floor(random::uniform() * (paramQuantity->getRange() + 1));
			paramQuantity->setValue(value);
		}
	}
};


// Buttons that don't have their own param but linked to a param, and don't need to be polled by the dsp engine, 
//   they will do thier own processing in here and update their source automatically
// --------------------

struct DynGroupMinusButtonNotify : DynGroupMinusButtonNoParam {
	Param* sourceParam = NULL;// param that is mapped to this
	
	void onChange(const event::Change &e) override {// called after value has changed
		DynGroupMinusButtonNoParam::onChange(e);
		if (sourceParam && state != 0) {
			float group = sourceParam->getValue();// separate variable so no glitch
			if (group < 0.5f) group = 4.0f;
			else group -= 1.0f;
			sourceParam->setValue(group);
		}		
	}
};


struct DynGroupPlusButtonNotify : DynGroupPlusButtonNoParam {
	Param* sourceParam = NULL;// param that is mapped to this
	
	void onChange(const event::Change &e) override {// called after value has changed
		DynGroupPlusButtonNoParam::onChange(e);
		if (sourceParam && state != 0) {
			float group = sourceParam->getValue();// separate variable so no glitch
			if (group > 3.5f) group = 0.0f;
			else group += 1.0f;
			sourceParam->setValue(group);
		}		
	}
};


// Special solo button with mutex feature (ctrl-click)

struct DynSoloButtonMutex : DynSoloButton {
	Param *soloParams;// 19 (or 15) params in here must be cleared when mutex solo performed on a group (track)
	unsigned long soloMutexUnclickMemory;// for ctrl-unclick. Invalid when soloMutexUnclickMemory == -1
	int soloMutexUnclickMemorySize;// -1 when nothing stored

	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			if (((APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL)) {
				int soloParamId = paramQuantity->paramId - TRACK_SOLO_PARAMS;
				bool isTrack = (soloParamId < 16);
				int end = 16 + (isTrack ? 0 : 4);
				bool turningOnSolo = soloParams[soloParamId].getValue() < 0.5f;
				
				
				if (turningOnSolo) {// ctrl turning on solo: memorize solo states and clear all other solos 
					// memorize solo states in case ctrl-unclick happens
					soloMutexUnclickMemorySize = end;
					soloMutexUnclickMemory = 0;
					for (int i = 0; i < end; i++) {
						if (soloParams[i].getValue() > 0.5f) {
							soloMutexUnclickMemory |= (1 << i);
						}
					}
					
					// clear 19 (or 15) solos 
					for (int i = 0; i < end; i++) {
						if (soloParamId != i) {
							soloParams[i].setValue(0.0f);
						}
					}
					
				}
				else {// ctrl turning off solo: recall stored solo states
					// reinstate 19 (or 15) solos 
					if (soloMutexUnclickMemorySize >= 0) {
						for (int i = 0; i < soloMutexUnclickMemorySize; i++) {
							if (soloParamId != i) {
								soloParams[i].setValue((soloMutexUnclickMemory & (1 << i)) != 0 ? 1.0f : 0.0f);
							}
						}
						soloMutexUnclickMemorySize = -1;// nothing in soloMutexUnclickMemory
					}
				}
				e.consume(this);
				return;
			}
			else {
				soloMutexUnclickMemorySize = -1;// nothing in soloMutexUnclickMemory
				if ((APP->window->getMods() & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
					for (int i = 0; i < 20; i++) {
						if (i != paramQuantity->paramId - TRACK_SOLO_PARAMS) {
							soloParams[i].setValue(0.0f);
						}
					}
					e.consume(this);
					return;
				}
			}
		}
		DynSoloButton::onButton(e);		
	}
};


// switch with dual display types (for Mute/Fade buttons)
// --------------------

struct DynamicSVGSwitchDual : SvgSwitch {
	int* mode = NULL;
    float* type = NULL;// mute when < minFadeRate, fade when >= minFadeRate
    int oldMode = -1;
    float oldType = -1.0f;
	std::vector<std::shared_ptr<Svg>> framesAll;
	std::vector<std::string> frameAltNames;
	
	void addFrameAll(std::shared_ptr<Svg> svg) {
		framesAll.push_back(svg);
		if (framesAll.size() == 2) {
			addFrame(framesAll[0]);
			addFrame(framesAll[1]);
		}
	}
    void addFrameAlt(std::string filename) {frameAltNames.push_back(filename);}
	void step() override {
		if( mode != NULL && type != NULL && ((*mode != oldMode) || (*type != oldType)) ) {
			if (!frameAltNames.empty()) {
				for (std::string strName : frameAltNames) {
					framesAll.push_back(APP->window->loadSvg(strName));
				}
				frameAltNames.clear();
			}
			int typeOffset = (*type < GlobalInfo::minFadeRate ? 0 : 2);
			frames[0]=framesAll[(*mode) * 4 + typeOffset + 0];
			frames[1]=framesAll[(*mode) * 4 + typeOffset + 1];
			oldMode = *mode;
			oldType = *type;
			onChange(*(new event::Change()));// required because of the way SVGSwitch changes images, we only change the frames above.
			fb->dirty = true;// dirty is not sufficient when changing via frames assignments above (i.e. onChange() is required)
		}
		SvgSwitch::step();
	}
};



struct DynMuteFadeButton : DynamicSVGSwitchDual {
	DynMuteFadeButton() {
		momentary = false;
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/mute-off.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/mute-on.svg")));
		addFrameAlt(asset::plugin(pluginInstance, "res/comp/fade-off.svg"));
		addFrameAlt(asset::plugin(pluginInstance, "res/comp/fade-on.svg"));
		shadow->opacity = 0.0;
	}
};

struct DynMuteFadeButtonWithClear : DynMuteFadeButton {
	Param *muteParams;// 19 (or 15) params in here must be cleared when mutex mute performed on a group (track)

	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			if ((APP->window->getMods() & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
				for (int i = 0; i < 20; i++) {
					if (i != paramQuantity->paramId - TRACK_MUTE_PARAMS) {
						muteParams[i].setValue(0.0f);
					}
				}
				e.consume(this);
				return;
			}
		}
		DynMuteFadeButton::onButton(e);		
	}	
};

// linked faders
// --------------------

struct DynSmallFaderWithLink : DynSmallFader {
	GlobalInfo *gInfo;
	Param *faderParams = NULL;
	float lastValue = -1.0f;
	
	void onButton(const event::Button &e) override {
		int faderIndex = paramQuantity->paramId - TRACK_FADER_PARAMS;
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			if ((APP->window->getMods() & RACK_MOD_MASK) == GLFW_MOD_ALT) {
				gInfo->toggleLinked(faderIndex);
				e.consume(this);
				return;
			}
			else if ((APP->window->getMods() & RACK_MOD_MASK) == (GLFW_MOD_ALT | GLFW_MOD_SHIFT)) {
				gInfo->linkBitMask = 0;
				e.consume(this);
				return;
			}
		}
		DynSmallFader::onButton(e);		
	}

	void draw(const DrawArgs &args) override {
		DynSmallFader::draw(args);
		if (paramQuantity) {
			int faderIndex = paramQuantity->paramId - TRACK_FADER_PARAMS;
			if (gInfo->isLinked(faderIndex)) {
				float v = paramQuantity->getScaledValue();
				float offsetY = handle->box.size.y / 2.0f;
				float ypos = math::rescale(v, 0.f, 1.f, minHandlePos.y, maxHandlePos.y) + offsetY;
				
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, 0, ypos);
				nvgLineTo(args.vg, box.size.x, ypos);
				nvgClosePath(args.vg);
				nvgStrokeColor(args.vg, SCHEME_RED);
				nvgStrokeWidth(args.vg, mm2px(0.4f));
				nvgStroke(args.vg);
			}
		}
	}
};


// knobs with color theme arc
// --------------------


struct DynKnobWithArc : DynKnob {
	NVGcolor arcColor;
	NVGcolor arcColorDarker;
	static constexpr float arcThickness = 1.6f;
	static constexpr float TOP_ANGLE = 3.0f * M_PI / 2.0f;
	float* paramWithCV = NULL;
	PackedBytes4* colorAndCloakPtr = NULL;
	
	DynKnobWithArc() {
	}
	
	void calcArcColorDarker(float scalingFactor) {
		arcColorDarker = arcColor;
		arcColorDarker.r *= scalingFactor;
		arcColorDarker.g *= scalingFactor;
		arcColorDarker.b *= scalingFactor;
	}
	
	void drawArc(const DrawArgs &args, float a0, float a1, NVGcolor* color) {
		int dir = a1 > a0 ? NVG_CW : NVG_CCW;
		Vec cVec = box.size.div(2.0f);
		float r = box.size.x / 2.0f + 2.25f;// arc radius
		nvgBeginPath(args.vg);
		nvgLineCap(args.vg, NVG_ROUND);
		nvgArc(args.vg, cVec.x, cVec.y, r, a0, a1, dir);
		nvgStrokeWidth(args.vg, arcThickness);
		nvgStrokeColor(args.vg, *color);
		nvgStroke(args.vg);		
	}
	
	void draw(const DrawArgs &args) override {
		DynamicSVGKnob::draw(args);
		if (paramQuantity) {
			float normalizedParam = paramQuantity->getScaledValue();
			float aParam = -10000.0f;
			float aBase = TOP_ANGLE;
			if (paramQuantity->getDefaultValue() == 0.0f) {
				aBase += minAngle;
			}
			// param
			if (normalizedParam != 0.5f && (colorAndCloakPtr->cc4[detailsShow] & ~colorAndCloakPtr->cc4[cloakedMode] & 0x3) == 0x3) {
				aParam = TOP_ANGLE + math::rescale(normalizedParam, 0.f, 1.f, minAngle, maxAngle);
				drawArc(args, aBase, aParam, &arcColorDarker);
			}
			// cv
			if (paramWithCV && *paramWithCV != -1.0f && (colorAndCloakPtr->cc4[detailsShow] & ~colorAndCloakPtr->cc4[cloakedMode] & 0x3) != 0) {
				if (aParam == -10000.0f) {
					aParam = TOP_ANGLE + math::rescale(normalizedParam, 0.f, 1.f, minAngle, maxAngle);
				}
				float aCv = TOP_ANGLE + math::rescale(*paramWithCV, paramQuantity->getMinValue(), paramQuantity->getMaxValue(), minAngle, maxAngle);
				drawArc(args, aParam, aCv, &arcColor);
			}
		}
	}
};

static constexpr float arcCvScale = 0.65f;
static const int greyArc = 120;

struct DynSmallKnobGreyWithArc : DynKnobWithArc {
	int oldDispColor = -1;
	int8_t* dispColorLocal;
	
	DynSmallKnobGreyWithArc() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/knob-grey.svg")));
		//addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnob.svg"));
	}
	
	void draw(const DrawArgs &args) override {
		if (colorAndCloakPtr) {
			int colorIndex = colorAndCloakPtr->cc4[dispColor] < 7 ? colorAndCloakPtr->cc4[dispColor] : *dispColorLocal;
			if (colorIndex != oldDispColor) {
				arcColor = DISP_COLORS[colorIndex];// arc color, same as displays
				arcColorDarker = nvgRGB(greyArc, greyArc, greyArc);//calcArcColorDarker(arcCvScale);
				oldDispColor = colorIndex;
			}
		}
		DynKnobWithArc::draw(args);
	}
};

struct DynSmallKnobAuxAWithArc : DynKnobWithArc {
	DynSmallKnobAuxAWithArc() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/knob-auxA.svg")));
		//addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnob.svg"));
		arcColor = nvgRGB(219, 65, 85);
		arcColorDarker = nvgRGB(greyArc, greyArc, greyArc);//calcArcColorDarker(arcCvScale);
	}
};
struct DynSmallKnobAuxBWithArc : DynKnobWithArc {
	DynSmallKnobAuxBWithArc() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/knob-auxB.svg")));
		//addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnob.svg"));
		arcColor = nvgRGB(255, 127, 42);
		arcColorDarker = nvgRGB(greyArc, greyArc, greyArc);//calcArcColorDarker(arcCvScale);
	}
};
struct DynSmallKnobAuxCWithArc : DynKnobWithArc {
	DynSmallKnobAuxCWithArc() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/knob-auxC.svg")));
		//addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnob.svg"));
		arcColor = nvgRGB(113, 160, 255);
		arcColorDarker = nvgRGB(greyArc, greyArc, greyArc);//calcArcColorDarker(arcCvScale);
	}
};
struct DynSmallKnobAuxDWithArc : DynKnobWithArc {
	DynSmallKnobAuxDWithArc() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/comp/knob-auxD.svg")));
		//addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnob.svg"));
		arcColor = nvgRGB(163, 93, 209);
		arcColorDarker = nvgRGB(greyArc, greyArc, greyArc);//calcArcColorDarker(arcCvScale);
	}
};


#endif
