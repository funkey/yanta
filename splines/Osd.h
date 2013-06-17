#ifndef SPLINES_OSD_H__
#define SPLINES_OSD_H__

#include <pipeline/all.h>
#include <gui/FingerSignals.h>

#include "OsdPainter.h"

class Osd : public pipeline::SimpleProcessNode<> {

public:

	Osd();

private:

	void updateOutputs() { *_penMode = _currentMode; _painter->setPenMode(_currentMode); }

	void onFingerDown(gui::FingerDown& signal);

	void onFingerUp(gui::FingerUp& signal);

	pipeline::Output<PenMode>    _penMode;
	pipeline::Output<OsdPainter> _painter;

	PenMode _currentMode;

	unsigned char _previousRed;
	unsigned long _redTapTime;
	unsigned char _previousGreen;
	unsigned long _greenTapTime;
	unsigned char _previousBlue;
	unsigned long _blueTapTime;

	double        _previousWidth;
	unsigned long _widthTapTime;
};

#endif // SPLINES_OSD_H__

