#include <cmath>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/make_shared.hpp>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "BackendPainter.h"
#include "TilingTexture.h"

logger::LogChannel backendpainterlog("backendpainterlog", "[BackendPainter] ");

util::ProgramOption optionDpi(
	util::_long_name        = "dpi",
	util::_description_text = "The dots per inch of the screen.",
	util::_default_value    = 96);

BackendPainter::BackendPainter() :
	_documentChanged(true),
	_documentPainter(gui::skia_pixel_t(255, 255, 255)),
	_documentCleanUpPainter(gui::skia_pixel_t(255, 255, 255)),
	_overlayAlpha(1.0),
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

	setDeviceTransformation();
	_documentPainter.setIncremental(true);
}

void
BackendPainter::contentAdded(const util::rect<DocumentPrecision>& region) {

	util::rect<int> pixelRegion = documentToTexture(region);

	if (_contentAddedRegion.isZero())

		_contentAddedRegion = pixelRegion;

	else {

		_contentAddedRegion.minX = std::min(_contentAddedRegion.minX, pixelRegion.minX);
		_contentAddedRegion.minY = std::min(_contentAddedRegion.minY, pixelRegion.minY);
		_contentAddedRegion.maxX = std::max(_contentAddedRegion.maxX, pixelRegion.maxX);
		_contentAddedRegion.maxY = std::max(_contentAddedRegion.maxY, pixelRegion.maxY);
	}
}

bool
BackendPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_documentPainter.hasDocument()) {

		LOG_DEBUG(backendpainterlog) << "no document to paint (yet)" << std::endl;
		return false;
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

	bool wantsRedraw = false;

	if (_mode == IncrementalDrawing) {

		// scale changed while we are in drawing mode -- texture needs to be 
		// redrawn completely
		if (_scale != _previousScale) {

			LOG_DEBUG(backendpainterlog) << "scale changed while we are in drawing mode" << std::endl;

			initiateFullRedraw(pixelRoi);

			// update the working area ourselves
			updateDocument(pixelRoi);
			//wantsRedraw = updateOverlay(pixelRoi);

		// shift changed in incremental drawing mode -- move prefetch texture 
		// and get back to incremental drawing
		} else if (pixelShift != _previousShift) {

			LOG_DEBUG(backendpainterlog) << "shift changed while we are in drawing mode" << std::endl;

			_documentTexture->shift(pixelShift - _previousShift);
			//_overlayTexture->shift(pixelShift - _previousShift);

		// transformation did not change
		} else {

			LOG_ALL(backendpainterlog) << "transformation did not change -- I just quickly update the document" << std::endl;

			updateDocument(pixelRoi);
			//wantsRedraw = updateOverlay(pixelRoi);
		}
	}

	if (_mode == Moving) {

		// shift changed in move mode -- just move prefetch texture
		if (pixelShift != _previousShift) {

			LOG_ALL(backendpainterlog) << "shift changed by " << (pixelShift - _previousShift) << std::endl;

			// show a different part of the document texture
			_documentTexture->shift(pixelShift - _previousShift);
			//_overlayTexture->shift(pixelShift - _previousShift);
			//_overlayTexture->cleanUp(_overlayPainter);
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
		_previousScale = _scale;

	return wantsRedraw;
}

void
BackendPainter::drag(const util::point<DocumentPrecision>& direction) {

	// the direction is given in (sub)pixel units, but we need integers
	util::point<int> d;
	d.x = (int)round(direction.x);
	d.y = (int)round(direction.y);

	_shift += d;

	_mode = Moving;

	setDeviceTransformation();
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

	setDeviceTransformation();
}

void
BackendPainter::prepareDrawing() {

	setDeviceTransformation();
	_mode = IncrementalDrawing;
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

util::rect<int>
BackendPainter::documentToTexture(const util::rect<DocumentPrecision>& rect) {

	util::rect<double> inv = rect;

	inv *= _scale;

	return inv;
}

void
BackendPainter::refresh() {

	LOG_DEBUG(backendpainterlog) << "refresh requested" << std::endl;
	_documentPainter.resetIncrementalMemory();
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
		_documentPainter.setIncremental(false);
		_documentTexture->update(_previousPixelRoi.intersection(pixelArea), _documentPainter);
		_documentPainter.setIncremental(true);

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

	boost::shared_ptr<TilingTexture> texture = _documentTexture;

	if (!texture)
		return false;

	if (texture->cleanUp(_documentCleanUpPainter, maxNumRequests))
		return true;

	return false;
}

void
BackendPainter::setDeviceTransformation() {

	_documentPainter.setDeviceTransformation(_scale, util::point<int>(0, 0));
	_documentCleanUpPainter.setDeviceTransformation(_scale, util::point<int>(0, 0));
	_overlayPainter.setDeviceTransformation(_scale, util::point<int>(0, 0));
}

void
BackendPainter::initiateFullRedraw(const util::rect<int>& roi) {

	LOG_DEBUG(backendpainterlog) << "initiate full redraw for roi " << roi << std::endl;

	_documentTexture->reset(roi.center());
	_documentPainter.resetIncrementalMemory();
	//_overlayTexture->reset(roi);
}

bool
BackendPainter::prepareTextures(const util::rect<int>& pixelRoi) {

	unsigned int textureWidth  = pixelRoi.width()  + _prefetchLeft + _prefetchRight;
	unsigned int textureHeight = pixelRoi.height() + _prefetchTop  + _prefetchBottom;

	LOG_ALL(backendpainterlog) << "with pre-fetch areas, texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	//if (_overlayTexture && (_overlayTexture->width() < (unsigned int)pixelRoi.width() || _overlayTexture->height() < (unsigned int)pixelRoi.height())) {

		//_overlayTexture.reset();
	//}

	//if (!_overlayTexture) {

		//_overlayTexture = boost::make_shared<TilingTexture>(pixelRoi, 0, 0, 0, 0);
	//}

	if (!_documentTexture) {

		_documentTexture = boost::make_shared<TilingTexture>(pixelRoi.center());

		return true;
	}

	return false;
}

void
BackendPainter::updateDocument(const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (!_documentPainter.needRedraw()) {

		LOG_ALL(backendpainterlog) << "nothing changed, skipping redraw" << std::endl;
		return;
	}

	if (!_contentAddedRegion.isZero()) {

		LOG_ALL(backendpainterlog) << "adding content in " << _contentAddedRegion << std::endl;

		_documentTexture->update(_contentAddedRegion, _documentPainter);
		_contentAddedRegion = util::rect<DocumentPrecision>(0, 0, 0, 0);

	} else {

		LOG_ALL(backendpainterlog) << "updating in " << roi << std::endl;

		_documentTexture->update(roi, _documentPainter);
	}

	_documentPainter.rememberDrawnElements();
}

bool
BackendPainter::updateOverlay(const util::rect<int>& /*roi*/) {

	// TODO: clean only dirty parts
	//_overlayTexture->fill(roi, _overlayPainter);

	//if (_overlayPainter.getDocument().size<Selection>() > 0) {

		//LOG_ALL(backendpainterlog) << "there are selection in the overlay -- requesting a redraw" << std::endl;

		//// let the selection pulsate
		//long millis = boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds();
		//_overlayAlpha = 0.5 + 0.4*2*std::abs(0.5 - std::fmod(millis*0.001, 1.0));

		//return true;
	//}

	return false;
}

void
BackendPainter::moveSelection(const SelectionMoved& /*signal*/) {

	//_overlayTexture->markDirty(signal.area);
}

void
BackendPainter::drawTextures(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	if (_mode == Zooming) {

		glScaled(_scaleChange.x, _scaleChange.y, 1.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		_documentTexture->render(roi/_scaleChange);
		glColor4f(1.0f, 1.0f, 0.5f, _overlayAlpha);
		//_overlayTexture->render(roi/_scaleChange);

	} else {

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		_documentTexture->render(roi);
		glColor4f(1.0f, 1.0f, 0.8f, _overlayAlpha);
		//_overlayTexture->render(roi);
	}

	glPopMatrix();
}
