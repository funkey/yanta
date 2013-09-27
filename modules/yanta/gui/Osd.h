#ifndef YANTA_OSD_H__
#define YANTA_OSD_H__

#include <pipeline/all.h>
#include <gui/FingerSignals.h>
#include <gui/PenSignals.h>

#include "OsdPainter.h"
#include "OsdRequest.h"
#include "OsdSignals.h"

class Osd : public pipeline::SimpleProcessNode<> {

public:

	Osd();

	static const double Small  = 0.1;
	static const double Normal = 0.3;
	static const double Big    = 1;
	static const double Large  = 3;

private:

	void updateOutputs() { _painter->setPenMode(*_penMode); }

	void onFingerUp(gui::FingerUp& signal);
	void onFingerDown(gui::FingerDown& signal);

	void onPenIn(const gui::PenIn& signal);
	void onPenAway(const gui::PenAway& signal);

	void toggleLasso();
	void toggleErasorMode();

	pipeline::Output<PenMode>    _penMode;
	pipeline::Output<OsdRequest> _osdRequest;
	pipeline::Output<OsdPainter> _painter;

	signals::Slot<Add> _add;

	unsigned char _previousRed;
	unsigned long _redTapTime;
	unsigned char _previousGreen;
	unsigned long _greenTapTime;
	unsigned char _previousBlue;
	unsigned long _blueTapTime;

	double        _previousWidth;
	unsigned long _widthTapTime;

	PenMode::Mode _previousMode;
	unsigned long _modeTapTime;
	unsigned long _erasorTapTime;

	bool _penAway;
};

#endif // YANTA_OSD_H__

