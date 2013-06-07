#include <cmath>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_canvasTexture(0),
	_prefetchLeft(1024),
	_prefetchRight(1024),
	_prefetchTop(1024),
	_prefetchBottom(1024),
	_state(Moving) {}

void
CanvasPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_strokes) {

		LOG_DEBUG(canvaspainterlog) << "no strokes to paint (yet)" << std::endl;
		return;
	}

	//LOG_ALL(canvaspainterlog) << "redrawing in " << roi << " with resolution " << resolution << std::endl;

	// Compute the closest canvas pixel roi to the desired roi in device units.  
	// Later, we are only using our discrete pixel roi for drawing the texture 
	// and displaying the texture. Just the transformation of canvas pixels to 
	// device units will depend on the roi the texture was aligned with 
	// initially (with a call to initiateFullRedraw()).
	util::rect<int> canvasPixelRoi = snapToPixelGrid(roi, resolution);

	//LOG_ALL(canvaspainterlog) << "snapped to pixel grid gives " << canvasPixelRoi << std::endl;

	gui::OpenGl::Guard guard;

	if (prepareTexture(canvasPixelRoi))
		initiateFullRedraw(canvasPixelRoi, roi, resolution);

	// canvasPixelRoi did not change
	if (canvasPixelRoi == _previousCanvasPixelRoi) {

		LOG_ALL(canvaspainterlog) << "canvas pixel roi did not change -- I just quickly update the strokes" << std::endl;

		// when we are entering incremental mode, we have to tell the texture 
		// that we have a working area, now
		if (_state == Moving) {

			_canvasTexture->setWorkingArea(canvasPixelRoi);
			_cairoPainter.resetIncrementalMemory();
		}

		_state = IncrementalDrawing;

		updateStrokes(*_strokes, canvasPixelRoi);

	// size of canvas pixel roi changed -- texture needs to be redrawn 
	// completely
	} else if (sizeChanged(canvasPixelRoi, resolution)) {

		LOG_ALL(canvaspainterlog) << "canvas pixel roi changed in size" << std::endl;

		_state = Moving;

		initiateFullRedraw(canvasPixelRoi, roi, resolution);
		updateStrokes(*_strokes, canvasPixelRoi);

	// canvas pixel roi was moved
	} else {

		LOG_ALL(canvaspainterlog) << "canvas pixel roi was moved by " << (canvasPixelRoi.upperLeft() - _previousCanvasPixelRoi.upperLeft()) << std::endl;

		_state = Moving;

		// show a different part of the canvas texture
		_canvasTexture->shift(canvasPixelRoi.upperLeft() - _previousCanvasPixelRoi.upperLeft());
	}

	// TODO: do this in a separate thread
	_canvasTexture->cleanDirtyAreas(_cairoPainter);

	drawTexture(canvasPixelRoi);

	_previousCanvasPixelRoi = canvasPixelRoi;
	_previousResolution     = resolution;
}

util::rect<int>
CanvasPainter::snapToPixelGrid(const util::rect<double>& roi, const util::point<double>& resolution) {

	// estimate width of roi in pixels
	unsigned int width  = (unsigned int)round(roi.width()*resolution.x);
	unsigned int height = (unsigned int)round(roi.height()*resolution.y);

	// estimate pixel coordinates of upper left corner of roi
	int x = (int)round(roi.minX*resolution.x);
	int y = (int)round(roi.minY*resolution.y);

	return util::rect<int>(x, y, x + width, y + height);
}

bool
CanvasPainter::prepareTexture(const util::rect<int>& canvasPixelRoi) {

	unsigned int textureWidth  = canvasPixelRoi.width()  + _prefetchLeft + _prefetchRight;
	unsigned int textureHeight = canvasPixelRoi.height() + _prefetchTop  + _prefetchBottom;

	LOG_ALL(canvaspainterlog) << "with pre-fetch areas, canvas texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	if (_canvasTexture != 0 && (_canvasTexture->width() != textureWidth || _canvasTexture->height() != textureHeight)) {

		LOG_ALL(canvaspainterlog) << "canvas texture is of different size, create a new one" << std::endl;

		delete _canvasTexture;
		_canvasTexture = 0;
	}

	if (_canvasTexture == 0) {

		_canvasTexture = new PrefetchTexture(canvasPixelRoi, _prefetchLeft, _prefetchRight, _prefetchTop, _prefetchBottom);

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

	_canvasTexture->render(roi, _deviceUnitsPerPixel, _deviceOffset);
}

void
CanvasPainter::initiateFullRedraw(
		const util::rect<int>&     canvasPixelRoi,
		const util::rect<double>&  deviceRoi,
		const util::point<double>& resolution) {

	_canvasTexture->reset(canvasPixelRoi);

	// remember the mapping from texture coordinates to device units
	_deviceUnitsPerPixel = util::point<double>(1.0/resolution.x, 1.0/resolution.y);
	_deviceOffset        = deviceRoi.upperLeft() - _deviceUnitsPerPixel*canvasPixelRoi.upperLeft();
	_pixelsPerDeviceUnit = resolution;
	_pixelOffset         = util::point<double>(-_deviceOffset.x*resolution.x, -_deviceOffset.y*resolution.y);

	_cairoPainter.setDeviceTransformation(_pixelsPerDeviceUnit, _pixelOffset);
	_cairoPainter.resetIncrementalMemory();
}

bool
CanvasPainter::sizeChanged(const util::rect<int>& canvasPixelRoi, const util::point<double>& resolution) {

	return (
			resolution != _previousResolution ||
			canvasPixelRoi.width()  != _previousCanvasPixelRoi.width() ||
			canvasPixelRoi.height() != _previousCanvasPixelRoi.height());
}

