#include <util/Logger.h>
#include "Osd.h"

logger::LogChannel osdlog("osdlog", "[Osd] ");

Osd::Osd() :
	_previousWidth(Osd::Normal),
	_widthTapTime(0),
	_penAway(false) {

	registerOutput(_penMode, "pen mode");
	registerOutput(_osdRequest, "osd request");
	registerOutput(_painter, "osd painter");

	_osdRequest.registerForwardSlot(_addPage);

	_painter.registerForwardCallback(&Osd::onFingerDown, this);
	_painter.registerForwardCallback(&Osd::onFingerUp, this);

	_currentMode.getStyle().setWidth(Osd::Normal);
}

void
Osd::onFingerDown(gui::FingerDown& signal) {

	const unsigned char RedOn    = 210;
	const unsigned char RedOff   = 0;
	const unsigned char GreenOn  = 180;
	const unsigned char GreenOff = 0;
	const unsigned char BlueOn   = 200;
	const unsigned char BlueOff  = 0;

	if (signal.position.x > 100)
		return;

	if (signal.position.y < 100) {

		_previousRed = _currentMode.getStyle().getRed();
		_redTapTime  = signal.timestamp;
		_currentMode.getStyle().setRed(_previousRed == RedOn ? RedOff : RedOn);

	} else if (signal.position.y < 200) {

		_previousGreen = _currentMode.getStyle().getGreen();
		_greenTapTime  = signal.timestamp;
		_currentMode.getStyle().setGreen(_previousGreen == GreenOn ? GreenOff : GreenOn);

	} else if (signal.position.y < 300) {

		_previousBlue = _currentMode.getStyle().getBlue();
		_blueTapTime  = signal.timestamp;
		_currentMode.getStyle().setBlue(_previousBlue == BlueOn ? BlueOff : BlueOn);

	} else if (signal.position.y < 400) {

		_previousWidth = _currentMode.getStyle().width();
		_widthTapTime  = signal.timestamp;
		_currentMode.getStyle().setWidth(Small);

	} else if (signal.position.y < 500) {

		_previousWidth = _currentMode.getStyle().width();
		_widthTapTime  = signal.timestamp;
		_currentMode.getStyle().setWidth(Normal);

	} else if (signal.position.y < 600) {

		_previousWidth = _currentMode.getStyle().width();
		_widthTapTime  = signal.timestamp;
		_currentMode.getStyle().setWidth(Big);

	} else if (signal.position.y < 700) {

		_previousWidth = _currentMode.getStyle().width();
		_widthTapTime  = signal.timestamp;
		_currentMode.getStyle().setWidth(Large);
	} else if (signal.position.y < 800) {

		_addPage();
	}

	signal.processed = true;
	setDirty(_penMode);
	setDirty(_painter);
}

void
Osd::onFingerUp(gui::FingerUp& signal) {

	const unsigned int maxTapTime = 300;

	if (signal.position.x > 100)
		return;

	if (signal.position.y < 100) {

		if (signal.timestamp - _redTapTime > maxTapTime)
			_currentMode.getStyle().setRed(_previousRed);

	} else if (signal.position.y < 200) {

		if (signal.timestamp - _greenTapTime > maxTapTime)
			_currentMode.getStyle().setGreen(_previousGreen);

	} else if (signal.position.y < 300) {

		if (signal.timestamp - _blueTapTime > maxTapTime)
			_currentMode.getStyle().setBlue(_previousBlue);

	} else if (signal.position.y < 700) {

		if (signal.timestamp - _widthTapTime > maxTapTime) {

			LOG_ALL(osdlog)
					<< "released stroke button after long tap ("
					<< signal.timestamp << "-" << _widthTapTime
					<< "), resetting previous value ("
					<< _previousWidth << ")" << std::endl;
			_currentMode.getStyle().setWidth(_previousWidth);
		}

	} else {

		return;
	}

	setDirty(_penMode);
	setDirty(_painter);
}

void
Osd::onPenIn(const gui::PenIn& /*signal*/) {

	_penAway = false;
}

void
Osd::onPenAway(const gui::PenAway& /*signal*/) {

	_penAway = true;
}
