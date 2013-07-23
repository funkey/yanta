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
	_documentChanged(true),
	_documentPainter(gui::skia_pixel_t(255, 255, 255)),
	_documentCleanUpPainter(gui::skia_pixel_t(255, 255, 255)),
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

	_documentPainter.setDeviceTransformation(_scale, util::point<double>(0, 0));
	_documentCleanUpPainter.setDeviceTransformation(_scale, util::point<double>(0, 0));
	_overlayPainter.setDeviceTransformation(_scale, util::point<double>(0, 0));
}

void
BackendPainter::drag(const util::point<DocumentPrecision>& direction) {

	// the direction is given in (sub)pixel units, but we need integers
	util::point<int> d;
	d.x = (int)round(direction.x);
	d.y = (int)round(direction.y);

	_shift += d;

	_mode = Moving;
}

void
BackendPainter::zoom(double zoomChange, const util::point<DocumentPrecision>& anchor) {

	LOG_ALL(backendpainterlog) << "changing zoom by " << zoomChange << " keeping " << anchor << " where it is" << std::endl;

	// convert the anchor from screen coordinates to texture coordinates
	_zoomAnchor = anchor - _shift;

	if (_mode != Zooming)
		_scaleChange = util::point<double>(1.0, 1.0);

	_scaleChange *= zoomChange;
	_scale       *= zoomChange;
	_shift        = _shift + (1 - zoomChange)*_zoomAnchor;

	// don't change the document painter transformations -- we simulate the zoom 
	// by stretching the texture (and repaint when we leave the zoom mode)

	_mode = Zooming;
}

util::point<DocumentPrecision>
BackendPainter::screenToDocument(const util::point<double>& point) {

	util::point<DocumentPrecision> inv = point;

	inv -= _shift;
	inv /= _scale;

	return inv;
}

util::point<int>
BackendPainter::documentToTexture(const util::point<DocumentPrecision>& point) {

	util::point<double> inv = point;

	inv *= _scale;

	return inv;
}

void
BackendPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_documentPainter.hasDocument()) {

		LOG_DEBUG(backendpainterlog) << "no document to paint (yet)" << std::endl;
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

	if (_documentChanged) {

		LOG_DEBUG(backendpainterlog) << "the document changed entirely -- resetting incremental memories" << std::endl;

		_documentPainter.resetIncrementalMemory();
		_documentChanged = false;
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
			_documentTexture->markDirty(_left);
			_documentTexture->markDirty(_right);
			_documentTexture->markDirty(_top);
			_documentTexture->markDirty(_bottom);

			// update the working area ourselves
			updateDocument(pixelRoi);
			updateOverlay(pixelRoi);

		// shift changed in incremental drawing mode -- move prefetch texture 
		// and get back to incremental drawing
		} else if (pixelShift != _previousShift) {

			LOG_DEBUG(backendpainterlog) << "shift changed while we are in drawing mode" << std::endl;

			_documentTexture->shift(pixelShift - _previousShift);
			_overlayTexture->shift(pixelShift - _previousShift);
			prepareDrawing(pixelRoi);

		// transformation did not change
		} else {

			LOG_ALL(backendpainterlog) << "transformation did not change -- I just quickly update the document" << std::endl;

			if (pixelRoi != _previousPixelRoi) {

				LOG_ALL(backendpainterlog) << "The ROI changed" << std::endl;
				prepareDrawing(pixelRoi);
			}

			updateDocument(pixelRoi);
			updateOverlay(pixelRoi);
		}
	}

	if (_mode == Moving) {

		// shift changed in move mode -- just move prefetch texture
		if (pixelShift != _previousShift) {

			LOG_ALL(backendpainterlog) << "shift changed by " << (pixelShift - _previousShift) << std::endl;

			// show a different part of the document texture
			_documentTexture->shift(pixelShift - _previousShift);
			_overlayTexture->shift(pixelShift - _previousShift);
			_overlayTexture->cleanUp(_overlayPainter);
		}
	}

	// draw the document and its overlay
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

	if (_documentTexture && (_documentTexture->width() < textureWidth || _documentTexture->height() < textureHeight)) {

		LOG_DEBUG(backendpainterlog) << "texture is of different size, create a new one" << std::endl;

		_documentTexture.reset();
	}

	if (!_documentTexture) {

		_documentTexture = boost::make_shared<PrefetchTexture>(pixelRoi, _prefetchLeft, _prefetchRight, _prefetchTop, _prefetchBottom);

		return true;
	}

	return false;
}

void
BackendPainter::refresh() {

	LOG_DEBUG(backendpainterlog) << "refresh requested" << std::endl;
	_documentPainter.resetIncrementalMemory();
}

void
BackendPainter::updateDocument(const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (!_documentPainter.needRedraw()) {

		LOG_ALL(backendpainterlog) << "nothing changed, skipping redraw" << std::endl;
		return;
	}

	_documentTexture->fill(roi, _documentPainter);
	_documentPainter.rememberDrawnElements();
}

void
BackendPainter::updateOverlay(const util::rect<int>& roi) {

	// TODO: clean only dirty parts
	_overlayTexture->fill(roi, _overlayPainter);
}

void
BackendPainter::initiateFullRedraw(const util::rect<int>& roi) {

	LOG_DEBUG(backendpainterlog) << "initiate full redraw for roi " << roi << std::endl;

	_documentTexture->reset(roi);
	_documentPainter.resetIncrementalMemory();
	_overlayTexture->reset(roi);
}

void
BackendPainter::markDirty(const util::rect<DocumentPrecision>& area) {

	// area is in document units -- transform it to pixel units
	util::point<int> ul = documentToTexture(area.upperLeft());
	util::point<int> lr = documentToTexture(area.lowerRight());

	// add a border of one pixels to compensate for rounding artefacts
	util::rect<int> pixelArea(ul.x - 1, ul.y - 1, lr.x + 1, lr.y +1);

	// are we currently looking at this area?
	if (_mode == IncrementalDrawing && pixelArea.intersects(_previousPixelRoi)) {

		LOG_DEBUG(backendpainterlog) << "redrawing dirty working area " << (_previousPixelRoi.intersection(pixelArea)) << std::endl;

		gui::OpenGl::Guard guard;

		// in this case, redraw immediately the part that got dirty
		_documentTexture->fill(_previousPixelRoi.intersection(pixelArea), _documentCleanUpPainter);

		// send everything beyond the working area to the worker threads
		_documentTexture->markDirty(_left.intersection(pixelArea));
		_documentTexture->markDirty(_right.intersection(pixelArea));
		_documentTexture->markDirty(_top.intersection(pixelArea));
		_documentTexture->markDirty(_bottom.intersection(pixelArea));

	} else {

		// otherwise, let the background threads do the work
		_documentTexture->markDirty(pixelArea);
	}
}

bool
BackendPainter::cleanDirtyAreas(unsigned int maxNumRequests) {

	boost::shared_ptr<PrefetchTexture> texture = _documentTexture;

	if (!texture || !texture->hasDirtyAreas())
		return false;

	texture->cleanUp(_documentCleanUpPainter, maxNumRequests);

	return true;
}

void
BackendPainter::moveSelection(const SelectionMoved& signal) {

	_overlayTexture->markDirty(signal.area);
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

	_documentTexture->setWorkingArea(workingArea);
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

	_documentPainter.resetIncrementalMemory();
	_documentPainter.setDeviceTransformation(_scale, util::point<DocumentPrecision>(0.0, 0.0));
	_documentCleanUpPainter.setDeviceTransformation(_scale, util::point<DocumentPrecision>(0.0, 0.0));
	_overlayPainter.setDeviceTransformation(_scale, util::point<DocumentPrecision>(0.0, 0.0));

	_mode = IncrementalDrawing;
}

void
BackendPainter::drawTextures(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	if (_mode == Zooming) {

		glScaled(_scaleChange.x, _scaleChange.y, 1.0);
		_documentTexture->render(roi/_scaleChange);
		_overlayTexture->render(roi/_scaleChange);

	} else {

		_documentTexture->render(roi);
		_overlayTexture->render(roi);
	}

	glPopMatrix();
}
