#include <cmath>
#include <boost/make_shared.hpp>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

util::ProgramOption optionDpi(
	util::_long_name        = "dpi",
	util::_description_text = "The dots per inch of the screen.",
	util::_default_value    = 96);

CanvasPainter::CanvasPainter() :
	_canvasChanged(true),
	_cairoPainter(gui::skia_pixel_t(255, 255, 255)),
	_cairoCleanUpPainter(gui::skia_pixel_t(255, 255, 255)),
	_prefetchLeft(1024),
	_prefetchRight(1024),
	_prefetchTop(1024),
	_prefetchBottom(1024),
	_shift(0, 0),
	_scale(optionDpi.as<double>()*0.0393701, optionDpi.as<double>()*0.0393701), // pixel per millimeter
	_scaleChange(1, 1),
	_previousShift(0, 0),
	_previousScale(0, 0),
	_previousPixelRoi(0, 0, 0, 0),
	_mode(IncrementalDrawing),
	_cursorPosition(0, 0) {

	_cairoPainter.setDeviceTransformation(_scale, _shift);
	_cairoCleanUpPainter.setDeviceTransformation(_scale, _shift);
}

void
CanvasPainter::drag(const util::point<CanvasPrecision>& direction) {

	// the direction is given in (sub)pixel units, but we need integers
	util::point<int> d;
	d.x = (int)round(direction.x);
	d.y = (int)round(direction.y);

	_shift += d;

	_mode = Moving;
}

void
CanvasPainter::zoom(double zoomChange, const util::point<CanvasPrecision>& anchor) {

	LOG_ALL(canvaspainterlog) << "changing zoom by " << zoomChange << " keeping " << anchor << " where it is" << std::endl;

	// convert the anchor from screen coordinates to texture coordinates
	util::point<double> textureAnchor = anchor - _shift;

	if (_mode != Zooming)
		_scaleChange = util::point<double>(1.0, 1.0);

	_scaleChange *= zoomChange;
	_scale       *= zoomChange;
	_shift        = _shift + (1 - zoomChange)*textureAnchor;

	// don't change the canvas painter transformations -- we simulate the zoom 
	// by stretching the texture (and repaint when we leave the zoom mode)

	_mode = Zooming;
}

util::point<CanvasPrecision>
CanvasPainter::screenToCanvas(const util::point<double>& point) {

	util::point<CanvasPrecision> inv = point;

	inv -= _shift;
	inv /= _scale;

	return inv;
}

util::point<int>
CanvasPainter::canvasToTexture(const util::point<CanvasPrecision>& point) {

	util::point<double> inv = point;

	inv *= _scale;

	return inv;
}

void
CanvasPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_canvas) {

		LOG_DEBUG(canvaspainterlog) << "no canvas to paint (yet)" << std::endl;
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

	if (prepareTexture(pixelRoi))
		initiateFullRedraw(pixelRoi);

	if (_canvasChanged) {

		LOG_DEBUG(canvaspainterlog) << "the canvas changed entirely -- resetting incremental memories" << std::endl;

		_cairoPainter.resetIncrementalMemory();
		_canvasChanged = false;
	}

	if (_mode == IncrementalDrawing) {

		// scale changed while we are in drawing mode -- texture needs to be 
		// redrawn completely
		if (_scale != _previousScale) {

			LOG_DEBUG(canvaspainterlog) << "scale changed while we are in drawing mode" << std::endl;

			initiateFullRedraw(pixelRoi);
			prepareDrawing(pixelRoi);

			// everything beyond the working area needs to be updated by the 
			// background painter
			_canvasTexture->markDirty(_left);
			_canvasTexture->markDirty(_right);
			_canvasTexture->markDirty(_top);
			_canvasTexture->markDirty(_bottom);

			// update the working area ourselves
			updateCanvas(*_canvas, pixelRoi);

		// shift changed in incremental drawing mode -- move prefetch texture 
		// and get back to incremental drawing
		} else if (_shift != _previousShift) {

			LOG_DEBUG(canvaspainterlog) << "shift changed while we are in drawing mode" << std::endl;

			_canvasTexture->shift(_shift - _previousShift);
			prepareDrawing(pixelRoi);

		// transformation did not change
		} else {

			LOG_ALL(canvaspainterlog) << "transformation did not change -- I just quickly update the canvas" << std::endl;

			if (pixelRoi != _previousPixelRoi) {

				LOG_ALL(canvaspainterlog) << "The ROI changed" << std::endl;
				prepareDrawing(pixelRoi);
			}

			updateCanvas(*_canvas, pixelRoi);
		}
	}

	if (_mode == Moving) {

		// shift changed in move mode -- just move prefetch texture
		if (_shift != _previousShift) {

			LOG_ALL(canvaspainterlog) << "shift changed by " << (_shift - _previousShift) << std::endl;

			// show a different part of the canvas texture
			_canvasTexture->shift(_shift - _previousShift);
		}
	}

	if (_mode == Zooming) {

		// TODO: show scaled version of texture
		//initiateFullRedraw(pixelRoi);
	}

	// draw the canvas
	drawTexture(pixelRoi);

	// draw the cursor
	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	glVertex2d(_cursorPosition.x - 2, _cursorPosition.y - 2);
	glVertex2d(_cursorPosition.x - 2, _cursorPosition.y + 2);
	glVertex2d(_cursorPosition.x + 2, _cursorPosition.y + 2);
	glVertex2d(_cursorPosition.x + 2, _cursorPosition.y - 2);
	glEnd();

	// remember configuration for next draw()
	_previousShift    = _shift;
	_previousPixelRoi = pixelRoi;
	// remember current scale only if we're not simulating the scaling in 
	// zooming mode (this allows us to detect a scale change when we're back in 
	// one of the other modes)
	if (_mode != Zooming)
		_previousScale    = _scale;
}

bool
CanvasPainter::prepareTexture(const util::rect<int>& pixelRoi) {

	unsigned int textureWidth  = pixelRoi.width()  + _prefetchLeft + _prefetchRight;
	unsigned int textureHeight = pixelRoi.height() + _prefetchTop  + _prefetchBottom;

	LOG_ALL(canvaspainterlog) << "with pre-fetch areas, texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	if (_canvasTexture && (_canvasTexture->width() < textureWidth || _canvasTexture->height() < textureHeight)) {

		LOG_DEBUG(canvaspainterlog) << "texture is of different size, create a new one" << std::endl;

		_canvasTexture.reset();
	}

	if (!_canvasTexture) {

		_canvasTexture = boost::make_shared<PrefetchTexture>(pixelRoi, _prefetchLeft, _prefetchRight, _prefetchTop, _prefetchBottom);

		return true;
	}

	return false;
}

void
CanvasPainter::refresh() {

	LOG_DEBUG(canvaspainterlog) << "refresh requested" << std::endl;
	_cairoPainter.resetIncrementalMemory();
}

void
CanvasPainter::updateCanvas(const Canvas& canvas, const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (_cairoPainter.alreadyDrawn(canvas)) {

		LOG_ALL(canvaspainterlog) << "nothing changed, skipping redraw" << std::endl;
		return;
	}

	_canvasTexture->fill(roi, _cairoPainter);
	_cairoPainter.rememberDrawnCanvas();
}

void
CanvasPainter::initiateFullRedraw(const util::rect<int>& roi) {

	LOG_DEBUG(canvaspainterlog) << "initiate full redraw for roi " << roi << std::endl;

	_canvasTexture->reset(roi);
	_cairoPainter.resetIncrementalMemory();
}

void
CanvasPainter::markDirty(const util::rect<CanvasPrecision>& area) {

	// area is in canvas units -- transform it to pixel units
	util::point<int> ul = canvasToTexture(area.upperLeft());
	util::point<int> lr = canvasToTexture(area.lowerRight());

	// add a border of one pixels to compensate for rounding artefacts
	util::rect<int> pixelArea(ul.x - 1, ul.y - 1, lr.x + 1, lr.y +1);

	// are we currently looking at this area?
	if (_mode == IncrementalDrawing && pixelArea.intersects(_previousPixelRoi)) {

		LOG_DEBUG(canvaspainterlog) << "redrawing dirty working area " << (_previousPixelRoi.intersection(pixelArea)) << std::endl;

		gui::OpenGl::Guard guard;

		// in this case, redraw immediately the part that got dirty
		_canvasTexture->fill(_previousPixelRoi.intersection(pixelArea), _cairoCleanUpPainter);

		// send everything beyond the working area to the worker threads
		_canvasTexture->markDirty(_left.intersection(pixelArea));
		_canvasTexture->markDirty(_right.intersection(pixelArea));
		_canvasTexture->markDirty(_top.intersection(pixelArea));
		_canvasTexture->markDirty(_bottom.intersection(pixelArea));

	} else {

		// otherwise, let the background threads do the work
		_canvasTexture->markDirty(pixelArea);
	}
}

bool
CanvasPainter::cleanDirtyAreas(unsigned int maxNumRequests) {

	boost::shared_ptr<PrefetchTexture> texture = _canvasTexture;

	if (!texture || !texture->hasDirtyAreas())
		return false;

	texture->cleanUp(_cairoCleanUpPainter, maxNumRequests);

	return true;
}

void
CanvasPainter::prepareDrawing(const util::rect<int>& roi) {

	LOG_DEBUG(canvaspainterlog) << "resetting the incremental memory" << std::endl;

	util::rect<int> workingArea;

	if (roi.isZero()) {

		if (_mode == IncrementalDrawing)
			return;

		workingArea = _previousPixelRoi;

	} else {

		workingArea = roi;
	}

	_canvasTexture->setWorkingArea(workingArea);

	_left = util::rect<int>(
			workingArea.minX - _prefetchLeft,
			workingArea.minY - _prefetchTop,
			workingArea.minX,
			workingArea.maxY + _prefetchBottom);
	_right = util::rect<int>(
			workingArea.maxX,
			workingArea.minY - _prefetchTop,
			workingArea.maxX + _prefetchRight,
			workingArea.maxY + _prefetchBottom);
	_top = util::rect<int>(
			workingArea.minX,
			workingArea.minY - _prefetchTop,
			workingArea.maxX,
			workingArea.minY);
	_bottom = util::rect<int>(
			workingArea.minX,
			workingArea.maxY,
			workingArea.maxX,
			workingArea.maxY + _prefetchBottom);

	_cairoPainter.resetIncrementalMemory();
	_cairoPainter.setDeviceTransformation(_scale, util::point<CanvasPrecision>(0.0, 0.0));
	_cairoCleanUpPainter.setDeviceTransformation(_scale, util::point<CanvasPrecision>(0.0, 0.0));

	_mode = IncrementalDrawing;
}

void
CanvasPainter::drawTexture(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	if (_mode == Zooming) {

		glScaled(_scaleChange.x, _scaleChange.y, 1.0);
		_canvasTexture->render(roi/_scaleChange);

	} else {

		_canvasTexture->render(roi);
	}

	glPopMatrix();
}
