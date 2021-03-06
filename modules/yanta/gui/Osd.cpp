#include <util/Logger.h>
#include "Osd.h"

logger::LogChannel osdlog("osdlog", "[Osd] ");

Osd::Osd() :
	_penMode(boost::make_shared<PenMode>()),
	_previousWidth(Osd::Normal),
	_widthTapTime(0),
	_erase(false),
	_lasso(false),
	_modeTapTime(0),
	_erasorTapTime(0),
	_penAway(false) {

	registerOutput(_penMode, "pen mode");
	registerOutput(_osdRequest, "osd request");
	registerOutput(_painter, "osd painter");

	_osdRequest.registerForwardSlot(_add);
	_osdRequest.registerForwardSlot(_remove);

	_painter.registerForwardCallback(&Osd::onFingerDown, this);
	_painter.registerForwardCallback(&Osd::onFingerUp, this);
	_painter.registerForwardCallback(&Osd::onPenDown, this);
	_painter.registerForwardCallback(&Osd::onPenUp, this);

	_penMode->getStyle().setWidth(Osd::Normal);
	_penMode->setMode(PenMode::Draw);
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

		_previousRed = _penMode->getStyle().getRed();
		_redTapTime  = signal.timestamp;
		_penMode->getStyle().setRed(_previousRed == RedOn ? RedOff : RedOn);
		setDirty(_penMode);

	} else if (signal.position.y < 200) {

		_previousGreen = _penMode->getStyle().getGreen();
		_greenTapTime  = signal.timestamp;
		_penMode->getStyle().setGreen(_previousGreen == GreenOn ? GreenOff : GreenOn);
		setDirty(_penMode);

	} else if (signal.position.y < 300) {

		_previousBlue = _penMode->getStyle().getBlue();
		_blueTapTime  = signal.timestamp;
		_penMode->getStyle().setBlue(_previousBlue == BlueOn ? BlueOff : BlueOn);
		setDirty(_penMode);

	} else if (signal.position.y < 400) {

		_previousWidth = _penMode->getStyle().width();
		_widthTapTime  = signal.timestamp;
		_penMode->getStyle().setWidth(Small);
		setDirty(_penMode);

	} else if (signal.position.y < 500) {

		_previousWidth = _penMode->getStyle().width();
		_widthTapTime  = signal.timestamp;
		_penMode->getStyle().setWidth(Normal);
		setDirty(_penMode);

	} else if (signal.position.y < 600) {

		_previousWidth = _penMode->getStyle().width();
		_widthTapTime  = signal.timestamp;
		_penMode->getStyle().setWidth(Big);
		setDirty(_penMode);

	} else if (signal.position.y < 700) {

		_previousWidth = _penMode->getStyle().width();
		_widthTapTime  = signal.timestamp;
		_penMode->getStyle().setWidth(Large);
		setDirty(_penMode);

	} else if (signal.position.y < 800) {

		toggleLasso();

		_modeTapTime  = signal.timestamp;

	} else if (signal.position.y < 900) {

		toggleErasorMode();

		_erasorTapTime  = signal.timestamp;

	} else if (signal.position.y < 1000) {

		_add();

	} else if (signal.position.y < 1100) {

		_remove();
	}

	signal.processed = true;
	setDirty(_painter);
}

void
Osd::onFingerUp(gui::FingerUp& signal) {

	const unsigned int maxTapTime = 300;

	if (signal.position.x > 100)
		return;

	if (signal.position.y < 100) {

		if (signal.timestamp - _redTapTime > maxTapTime) {

			_penMode->getStyle().setRed(_previousRed);
			setDirty(_penMode);
		}

	} else if (signal.position.y < 200) {

		if (signal.timestamp - _greenTapTime > maxTapTime) {

			_penMode->getStyle().setGreen(_previousGreen);
			setDirty(_penMode);
		}

	} else if (signal.position.y < 300) {

		if (signal.timestamp - _blueTapTime > maxTapTime) {

			_penMode->getStyle().setBlue(_previousBlue);
			setDirty(_penMode);
		}

	} else if (signal.position.y < 700) {

		if (signal.timestamp - _widthTapTime > maxTapTime) {

			_penMode->getStyle().setWidth(_previousWidth);
			setDirty(_penMode);
		}

	} else if (signal.position.y < 800) {

		if (signal.timestamp - _modeTapTime > maxTapTime)
			toggleLasso();

	} else if (signal.position.y < 900) {

		if (signal.timestamp - _erasorTapTime > maxTapTime)
			toggleErasorMode();

	} else {

		return;
	}

	setDirty(_painter);
}

void
Osd::onPenDown(const gui::PenDown& signal) {

	if (signal.button == gui::buttons::Right) {

		_erase = true;

		_penMode->getStyle().setWidth(_penMode->getStyle().width()*2);

		updatePenMode();

		setDirty(_penMode);
	}
}

void
Osd::onPenUp(const gui::PenUp& signal) {

	if (signal.button == gui::buttons::Right && _penMode->getMode() == PenMode::Erase) {

		_erase = false;

		_penMode->getStyle().setWidth(_penMode->getStyle().width()/2);

		updatePenMode();

		setDirty(_penMode);
	}
}

void
Osd::onPenIn(const gui::PenIn& /*signal*/) {

	_penAway = false;
}

void
Osd::onPenAway(const gui::PenAway& /*signal*/) {

	_penAway = true;
}

void
Osd::toggleLasso() {

	_lasso = !_lasso;

	updatePenMode();

	setDirty(_penMode);
}

void
Osd::toggleErasorMode() {

	if (_penMode->getErasorMode() == Erasor::SphereErasor) {

		_penMode->setErasorMode(Erasor::ElementErasor);

	} else {

		_penMode->setErasorMode(Erasor::SphereErasor);
	}

	setDirty(_penMode);
}

void
Osd::updatePenMode() {

	// erasing has highest precedence...
	if (_erase) {

		_penMode->setMode(PenMode::Erase);
		return;
	}

	// ...then lasso...
	if (_lasso) {

		_penMode->setMode(PenMode::Lasso);
		return;
	}

	// ...finally, we just draw
	_penMode->setMode(PenMode::Draw);
}
