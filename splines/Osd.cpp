#include <util/Logger.h>
#include "Osd.h"

logger::LogChannel osdlog("osdlog", "[Osd] ");

Osd::Osd() {

	registerOutput(_penMode, "pen mode");
	registerOutput(_painter, "osd painter");

	_painter.registerForwardCallback(&Osd::onFingerDown, this);
	_painter.registerForwardCallback(&Osd::onFingerUp, this);
}

void
Osd::onFingerDown(gui::FingerDown& signal) {

	if (signal.position.x > 100)
		return;

	if (signal.position.y < 100) {

		_currentMode.getStyle().setRed(210);

	} else if (signal.position.y < 200) {

		_currentMode.getStyle().setGreen(180);

	} else if (signal.position.y < 300) {

		_currentMode.getStyle().setBlue(200);

	} else if (signal.position.y < 400) {

		_currentMode.getStyle().setWidth(1.0);

	} else if (signal.position.y < 500) {

		_currentMode.getStyle().setWidth(2.0);

	} else if (signal.position.y < 600) {

		_currentMode.getStyle().setWidth(4.0);

	} else if (signal.position.y < 700) {

		_currentMode.getStyle().setWidth(8.0);
	}

	setDirty(_penMode);
	setDirty(_painter);
	signal.processed = true;
}

void
Osd::onFingerUp(gui::FingerUp& signal) {

	if (signal.position.x > 100)
		return;

	if (signal.position.y < 100) {

		_currentMode.getStyle().setRed(0);

	} else if (signal.position.y < 200) {

		_currentMode.getStyle().setGreen(0);

	} else if (signal.position.y < 300) {

		_currentMode.getStyle().setBlue(0);

	} else {

		_currentMode.getStyle().setWidth(1.0);
	}

	setDirty(_penMode);
	setDirty(_painter);
}
