#include <cmath>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_canvasTexture(0),
	//_prefetchLeft(1024),
	//_prefetchRight(1024),
	//_prefetchTop(1024),
	//_prefetchBottom(1024),
	_prefetchLeft(100),
	_prefetchRight(100),
	_prefetchTop(100),
	_prefetchBottom(100),
	_shift(0, 0),
	_scale(1.0, 1.0),
	_state(Moving),
	_previousShift(0, 0),
	_previousScale(0, 0) {

	_cairoPainter.setDeviceTransformation(_scale, _shift);
}

void
CanvasPainter::drag(const util::point<double>& direction) {

	// the direction is given in (sub)pixel units, but we need integers
	util::point<int> d;
	d.x = (int)round(direction.x);
	d.y = (int)round(direction.y);

	_shift += d;
}

void
CanvasPainter::zoom(double zoomChange, const util::point<double>& anchor) {

	LOG_ALL(canvaspainterlog) << "changing zoom by " << zoomChange << " keeping " << anchor << " where it is" << std::endl;

	// convert the anchor from screen coordinates to texture coordinates
	util::point<double> textureAnchor = anchor - _shift;

	_scale *= zoomChange;
	_shift  = _shift + (1 - zoomChange)*textureAnchor;

	// we are taking care of the translation
	_cairoPainter.setDeviceTransformation(_scale, util::point<double>(0.0, 0.0));
}

util::point<double>
CanvasPainter::pixelToCanvas(const util::point<double>& point) {

	util::point<double> inv = point;

	inv -= _shift;
	inv /= _scale;

	return inv;
}

void
CanvasPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_strokes) {

		LOG_DEBUG(canvaspainterlog) << "no strokes to paint (yet)" << std::endl;
		return;
	}

	LOG_ALL(canvaspainterlog) << "redrawing in " << roi << " with resolution " << resolution << std::endl;

	// the smallest integer pixel roi fitting the desired roi
	util::rect<int> pixelRoi;
	pixelRoi.minX = (int)floor(roi.minX);
	pixelRoi.minY = (int)floor(roi.minY);
	pixelRoi.maxX = (int)ceil(roi.maxX);
	pixelRoi.maxY = (int)ceil(roi.maxY);

	// convert the pixel roi from screen coordinates to texture coordinates
	pixelRoi -= _shift;

	LOG_ALL(canvaspainterlog) << "pixel roi is " << pixelRoi << std::endl;

	gui::OpenGl::Guard guard;

	// TODO: consider only the maximum of all observed pixelRois
	if (prepareTexture(pixelRoi))
		initiateFullRedraw(pixelRoi);

	// transformation did not change
	if (_shift == _previousShift && _scale == _previousScale) {

		LOG_ALL(canvaspainterlog) << "transformation did not change -- I just quickly update the strokes" << std::endl;

		// when we are entering incremental mode, we have to tell the texture 
		// that we have a working area, now
		if (_state == Moving) {

			//_canvasTexture->setWorkingArea(pixelRoi);
			_cairoPainter.resetIncrementalMemory();
		}

		_state = IncrementalDrawing;

		updateStrokes(*_strokes, pixelRoi);

	// scale changed -- texture needs to be redrawn completely
	} else if (_scale != _previousScale) {

		LOG_ALL(canvaspainterlog) << "scale changed" << std::endl;

		_canvasTexture->unsetWorkingArea();

		_state = Moving;

		initiateFullRedraw(pixelRoi);
		updateStrokes(*_strokes, pixelRoi);

	// shift changed -- move prefetch texture
	} else {

		LOG_ALL(canvaspainterlog) << "transformation moved by " << (_shift - _previousShift) << std::endl;

		_canvasTexture->unsetWorkingArea();

		_state = Moving;

		// show a different part of the canvas texture
		_canvasTexture->shift(_shift - _previousShift);
	}

	// TODO: do this in a separate thread
	_canvasTexture->cleanDirtyAreas(_cairoPainter);

	drawTexture(pixelRoi);

	_previousShift = _shift;
	_previousScale = _scale;
}

bool
CanvasPainter::prepareTexture(const util::rect<int>& pixelRoi) {

	unsigned int textureWidth  = pixelRoi.width()  + _prefetchLeft + _prefetchRight;
	unsigned int textureHeight = pixelRoi.height() + _prefetchTop  + _prefetchBottom;

	LOG_ALL(canvaspainterlog) << "with pre-fetch areas, texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	if (_canvasTexture != 0 && (_canvasTexture->width() != textureWidth || _canvasTexture->height() != textureHeight)) {

		LOG_ALL(canvaspainterlog) << "texture is of different size, create a new one" << std::endl;

		delete _canvasTexture;
		_canvasTexture = 0;
	}

	if (_canvasTexture == 0) {

		_canvasTexture = new PrefetchTexture(pixelRoi, _prefetchLeft, _prefetchRight, _prefetchTop, _prefetchBottom);

		return true;
	}

	return false;
}

void
CanvasPainter::refresh() {

	LOG_ALL(canvaspainterlog) << "refresh requested" << std::endl;
	_cairoPainter.resetIncrementalMemory();
}

void
CanvasPainter::updateStrokes(const Strokes& strokes, const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (_cairoPainter.drawnUntilStroke() > 1 &&
	    _cairoPainter.drawnUntilStroke() == strokes.size() &&
	    _cairoPainter.drawnUntilStrokePoint() == strokes[_cairoPainter.drawnUntilStroke() - 1].size()) {

		LOG_ALL(canvaspainterlog) << "nothing changed, skipping redraw" << std::endl;
		return;
	}

	if (_state == IncrementalDrawing) {

		LOG_ALL(canvaspainterlog)
				<< "drawing incrementally in " << roi << ", starting with stroke "
				<< _cairoPainter.drawnUntilStroke() << ", point "
				<< _cairoPainter.drawnUntilStrokePoint() << std::endl;
	} else {

		LOG_ALL(canvaspainterlog)
				<< "drawing everything in " << roi << std::endl;
	}

	_cairoPainter.setIncremental(_state == IncrementalDrawing);
	_canvasTexture->fill(roi, _cairoPainter);
	_cairoPainter.rememberDrawnStrokes();
}

void
CanvasPainter::drawTexture(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	_canvasTexture->render(roi);

	glPopMatrix();
}

void
CanvasPainter::initiateFullRedraw(const util::rect<int>& roi) {

	_canvasTexture->reset(roi);
	_cairoPainter.resetIncrementalMemory();
}

