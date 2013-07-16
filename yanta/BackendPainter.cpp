#include <cmath>
#include <boost/make_shared.hpp>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "BackendPainter.h"

logger::LogChannel backendpainterlog("backendpainterlog", "[BackendPainter] ");

util::ProgramOption optionDpi(
	util::_long_name        = "dpi",
	util::_description_text = "The dots per inch of the screen.",
	util::_default_value    = 96);

BackendPainter::BackendPainter() :
	_canvasChanged(true),
	_canvasPainter(gui::skia_pixel_t(255, 255, 255)),
	_canvasCleanUpPainter(gui::skia_pixel_t(255, 255, 255)),
	_prefetchLeft(1024),
	_prefetchRight(1024),
	_prefetchTop(1024),
	_prefetchBottom(1024),
	_shift(0, 0),
	_scale(optionDpi.as<double>()*0.0393701, optionDpi.as<double>()*0.0393701), // pixel per millimeter
	_scaleChange(1, 1),
	_zoomAnchor(0, 0),
	_previousShift(0, 0),
	_previousScale(0, 0),
	_previousPixelRoi(0, 0, 0, 0),
	_mode(IncrementalDrawing),
	_cursorPosition(0, 0) {

	_canvasPainter.setDeviceTransformation(_scale, util::point<double>(0, 0));
	_canvasCleanUpPainter.setDeviceTransformation(_scale, util::point<double>(0, 0));
	_overlayPainter.setDeviceTransformation(_scale, util::point<double>(0, 0));
}

void
BackendPainter::drag(const util::point<CanvasPrecision>& direction) {

	// the direction is given in (sub)pixel units, but we need integers
	util::point<int> d;
	d.x = (int)round(direction.x);
	d.y = (int)round(direction.y);

	_shift += d;

	_mode = Moving;
}

void
BackendPainter::zoom(double zoomChange, const util::point<CanvasPrecision>& anchor) {

	LOG_ALL(backendpainterlog) << "changing zoom by " << zoomChange << " keeping " << anchor << " where it is" << std::endl;

	// convert the anchor from screen coordinates to texture coordinates
	_zoomAnchor = anchor - _shift;

	if (_mode != Zooming)
		_scaleChange = util::point<double>(1.0, 1.0);

	_scaleChange *= zoomChange;
	_scale       *= zoomChange;
	_shift        = _shift + (1 - zoomChange)*_zoomAnchor;

	// don't change the canvas painter transformations -- we simulate the zoom 
	// by stretching the texture (and repaint when we leave the zoom mode)

	_mode = Zooming;
}

util::point<CanvasPrecision>
BackendPainter::screenToCanvas(const util::point<double>& point) {

	util::point<CanvasPrecision> inv = point;

	inv -= _shift;
	inv /= _scale;

	return inv;
}

util::point<int>
BackendPainter::canvasToTexture(const util::point<CanvasPrecision>& point) {

	util::point<double> inv = point;

	inv *= _scale;

	return inv;
}

void
BackendPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_canvas) {

		LOG_DEBUG(backendpainterlog) << "no canvas to paint (yet)" << std::endl;
		return;
	}

	LOG_ALL(backendpainterlog) << "redrawing in " << roi << " with resolution " << resolution << std::endl;

	// the smallest integer pixel roi fitting the desired roi
	util::rect<int> pixelRoi;
	pixelRoi.minX = (int)floor(roi.minX);
	pixelRoi.minY = (int)floor(roi.minY);
	pixelRoi.maxX = (int)ceil(roi.maxX);
	pixelRoi.maxY = (int)ceil(roi.maxY);

	// the current shift in pixel units
	util::point<int> pixelShift(_shift);

	// convert the pixel roi from screen coordinates to texture coordinates
	pixelRoi -= pixelShift;

	LOG_ALL(backendpainterlog) << "pixel roi is " << pixelRoi << std::endl;

	gui::OpenGl::Guard guard;

	if (prepareTextures(pixelRoi))
		initiateFullRedraw(pixelRoi);

	if (_canvasChanged) {

		LOG_DEBUG(backendpainterlog) << "the canvas changed entirely -- resetting incremental memories" << std::endl;

		_canvasPainter.resetIncrementalMemory();
		_canvasChanged = false;
	}

	if (_mode == IncrementalDrawing) {

		// scale changed while we are in drawing mode -- texture needs to be 
		// redrawn completely
		if (_scale != _previousScale) {

			LOG_DEBUG(backendpainterlog) << "scale changed while we are in drawing mode" << std::endl;

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
			updateOverlay(pixelRoi);

		// shift changed in incremental drawing mode -- move prefetch texture 
		// and get back to incremental drawing
		} else if (pixelShift != _previousShift) {

			LOG_DEBUG(backendpainterlog) << "shift changed while we are in drawing mode" << std::endl;

			_canvasTexture->shift(pixelShift - _previousShift);
			_overlayTexture->shift(pixelShift - _previousShift);
			prepareDrawing(pixelRoi);

		// transformation did not change
		} else {

			LOG_ALL(backendpainterlog) << "transformation did not change -- I just quickly update the canvas" << std::endl;

			if (pixelRoi != _previousPixelRoi) {

				LOG_ALL(backendpainterlog) << "The ROI changed" << std::endl;
				prepareDrawing(pixelRoi);
			}

			updateCanvas(*_canvas, pixelRoi);
			updateOverlay(pixelRoi);
		}
	}

	if (_mode == Moving) {

		// shift changed in move mode -- just move prefetch texture
		if (pixelShift != _previousShift) {

			LOG_ALL(backendpainterlog) << "shift changed by " << (pixelShift - _previousShift) << std::endl;

			// show a different part of the canvas texture
			_canvasTexture->shift(pixelShift - _previousShift);
			_overlayTexture->shift(pixelShift - _previousShift);
			_overlayTexture->cleanUp(_overlayPainter);
		}
	}

	// draw the canvas and its overlay
	drawTextures(pixelRoi);

	// draw the cursor
	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	glVertex2d(_cursorPosition.x - 2, _cursorPosition.y - 2);
	glVertex2d(_cursorPosition.x - 2, _cursorPosition.y + 2);
	glVertex2d(_cursorPosition.x + 2, _cursorPosition.y + 2);
	glVertex2d(_cursorPosition.x + 2, _cursorPosition.y - 2);
	glEnd();

	// remember configuration for next draw()
	_previousShift    = pixelShift;
	_previousPixelRoi = pixelRoi;
	// remember current scale only if we're not simulating the scaling in 
	// zooming mode (this allows us to detect a scale change when we're back in 
	// one of the other modes)
	if (_mode != Zooming)
		_previousScale    = _scale;
}

bool
BackendPainter::prepareTextures(const util::rect<int>& pixelRoi) {

	unsigned int textureWidth  = pixelRoi.width()  + _prefetchLeft + _prefetchRight;
	unsigned int textureHeight = pixelRoi.height() + _prefetchTop  + _prefetchBottom;

	LOG_ALL(backendpainterlog) << "with pre-fetch areas, texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	if (_overlayTexture && (_overlayTexture->width() < (unsigned int)pixelRoi.width() || _overlayTexture->height() < (unsigned int)pixelRoi.height())) {

		_overlayTexture.reset();
	}

	if (!_overlayTexture) {

		_overlayTexture = boost::make_shared<PrefetchTexture>(pixelRoi, 0, 0, 0, 0);
	}

	if (_canvasTexture && (_canvasTexture->width() < textureWidth || _canvasTexture->height() < textureHeight)) {

		LOG_DEBUG(backendpainterlog) << "texture is of different size, create a new one" << std::endl;

		_canvasTexture.reset();
	}

	if (!_canvasTexture) {

		_canvasTexture = boost::make_shared<PrefetchTexture>(pixelRoi, _prefetchLeft, _prefetchRight, _prefetchTop, _prefetchBottom);

		return true;
	}

	return false;
}

void
BackendPainter::refresh() {

	LOG_DEBUG(backendpainterlog) << "refresh requested" << std::endl;
	_canvasPainter.resetIncrementalMemory();
}

void
BackendPainter::updateCanvas(const Canvas& canvas, const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (_canvasPainter.alreadyDrawn(canvas)) {

		LOG_ALL(backendpainterlog) << "nothing changed, skipping redraw" << std::endl;
		return;
	}

	_canvasTexture->fill(roi, _canvasPainter);
	_canvasPainter.rememberDrawnCanvas();
}

void
BackendPainter::updateOverlay(const util::rect<int>& roi) {

	_overlayTexture->fill(roi, _overlayPainter);
}

void
BackendPainter::initiateFullRedraw(const util::rect<int>& roi) {

	LOG_DEBUG(backendpainterlog) << "initiate full redraw for roi " << roi << std::endl;

	_canvasTexture->reset(roi);
	_canvasPainter.resetIncrementalMemory();
	_overlayTexture->reset(roi);
}

void
BackendPainter::markDirty(const util::rect<CanvasPrecision>& area) {

	// area is in canvas units -- transform it to pixel units
	util::point<int> ul = canvasToTexture(area.upperLeft());
	util::point<int> lr = canvasToTexture(area.lowerRight());

	// add a border of one pixels to compensate for rounding artefacts
	util::rect<int> pixelArea(ul.x - 1, ul.y - 1, lr.x + 1, lr.y +1);

	// are we currently looking at this area?
	if (_mode == IncrementalDrawing && pixelArea.intersects(_previousPixelRoi)) {

		LOG_DEBUG(backendpainterlog) << "redrawing dirty working area " << (_previousPixelRoi.intersection(pixelArea)) << std::endl;

		gui::OpenGl::Guard guard;

		// in this case, redraw immediately the part that got dirty
		_canvasTexture->fill(_previousPixelRoi.intersection(pixelArea), _canvasCleanUpPainter);

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
BackendPainter::cleanDirtyAreas(unsigned int maxNumRequests) {

	boost::shared_ptr<PrefetchTexture> texture = _canvasTexture;

	if (!texture || !texture->hasDirtyAreas())
		return false;

	texture->cleanUp(_canvasCleanUpPainter, maxNumRequests);

	return true;
}

void
BackendPainter::prepareDrawing(const util::rect<int>& roi) {

	LOG_DEBUG(backendpainterlog) << "resetting the incremental memory" << std::endl;

	util::rect<int> workingArea;

	if (roi.isZero()) {

		if (_mode == IncrementalDrawing)
			return;

		workingArea = _previousPixelRoi;

	} else {

		workingArea = roi;
	}

	_canvasTexture->setWorkingArea(workingArea);
	_overlayTexture->setWorkingArea(workingArea);

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

	_canvasPainter.resetIncrementalMemory();
	_canvasPainter.setDeviceTransformation(_scale, util::point<CanvasPrecision>(0.0, 0.0));
	_canvasCleanUpPainter.setDeviceTransformation(_scale, util::point<CanvasPrecision>(0.0, 0.0));
	_overlayPainter.setDeviceTransformation(_scale, util::point<CanvasPrecision>(0.0, 0.0));

	_mode = IncrementalDrawing;
}

void
BackendPainter::drawTextures(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	if (_mode == Zooming) {

		glScaled(_scaleChange.x, _scaleChange.y, 1.0);
		_canvasTexture->render(roi/_scaleChange);
		_overlayTexture->render(roi/_scaleChange);

	} else {

		_canvasTexture->render(roi);
		_overlayTexture->render(roi);
	}

	glPopMatrix();
}
