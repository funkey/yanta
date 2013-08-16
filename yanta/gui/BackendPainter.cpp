#include <cmath>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/make_shared.hpp>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "BackendPainter.h"
#include "TorusTexture.h"

logger::LogChannel backendpainterlog("backendpainterlog", "[BackendPainter] ");

util::ProgramOption optionDpi(
	util::_long_name        = "dpi",
	util::_description_text = "The dots per inch of the screen.",
	util::_default_value    = 96);

BackendPainter::BackendPainter() :
	_documentChanged(true),
	_documentPainter(gui::skia_pixel_t(255, 255, 255)),
	_documentCleanUpPainter(boost::make_shared<SkiaDocumentPainter>(gui::skia_pixel_t(255, 255, 255))),
	_overlayAlpha(1.0),
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

	// TODO: is this really needed? I think we can assume a valid OpenGl 
	// context, here.
	gui::OpenGl::Guard guard;

	if (prepareTextures(pixelRoi) || _documentChanged) {

		LOG_DEBUG(backendpainterlog) << "rebuild texture or document changed entirely -- initiate full redraw" << std::endl;

		initiateFullRedraw(pixelRoi);
		_documentChanged = false;
	}

	bool wantsRedraw = false;

	if (_mode == IncrementalDrawing) {

		// scale changed while we are in drawing mode -- texture needs to be 
		// redrawn completely
		if (_scale != _previousScale) {

			LOG_DEBUG(backendpainterlog) << "scale changed while we are in drawing mode" << std::endl;

			initiateFullRedraw(pixelRoi);

		// shift changed in incremental drawing mode -- move prefetch texture 
		// and get back to incremental drawing
		} else if (pixelShift != _previousShift) {

			LOG_DEBUG(backendpainterlog) << "shift changed while we are in drawing mode" << std::endl;

			_documentTexture->shift(pixelShift - _previousShift);
			_overlayTexture->shift(pixelShift - _previousShift);

		// transformation did not change
		} else {

			LOG_ALL(backendpainterlog) << "transformation did not change" << std::endl;
		}
	}

	if (_mode == Dragging) {

		// shift changed in move mode -- just move prefetch texture
		if (pixelShift != _previousShift) {

			LOG_ALL(backendpainterlog) << "shift changed by " << (pixelShift - _previousShift) << std::endl;

			// show a different part of the document texture
			_documentTexture->shift(pixelShift - _previousShift);
			_overlayTexture->shift(pixelShift - _previousShift);
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
BackendPainter::contentAdded(const util::rect<DocumentPrecision>& region) {

	// get the pixels that are affected
	util::rect<int> pixelRegion = documentToTexture(region);

	// mark the corresponding part of the texture as needs-update
	_documentTexture->markDirty(pixelRegion, TorusTexture::NeedsUpdate);

	_mode = IncrementalDrawing;
}

void
BackendPainter::drag(const util::point<DocumentPrecision>& direction) {

	// the direction is given in (sub)pixel units, but we need integers
	util::point<int> d;
	d.x = (int)round(direction.x);
	d.y = (int)round(direction.y);

	_shift += d;

	_mode = Dragging;
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

void
BackendPainter::finishZoom() {

	setDeviceTransformation();
	initiateFullRedraw(_previousPixelRoi);

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

	initiateFullRedraw(_previousPixelRoi);
}

void
BackendPainter::markDirty(const util::rect<DocumentPrecision>& area) {

	// area is in document units -- transform it to pixel units
	util::point<int> ul = documentToTexture(area.upperLeft());
	util::point<int> lr = documentToTexture(area.lowerRight());

	// add a border of one pixels to compensate for rounding artefacts
	util::rect<int> pixelArea(ul.x - 1, ul.y - 1, lr.x + 1, lr.y +1);

	_documentTexture->markDirty(pixelArea, TorusTexture::NeedsRedraw);
}

void
BackendPainter::markOverlayDirty(const util::rect<DocumentPrecision>& area) {

	// area is in document units -- transform it to pixel units
	util::point<int> ul = documentToTexture(area.upperLeft());
	util::point<int> lr = documentToTexture(area.lowerRight());

	// add a border of one pixels to compensate for rounding artefacts
	util::rect<int> pixelArea(ul.x - 1, ul.y - 1, lr.x + 1, lr.y +1);

	_overlayTexture->markDirty(pixelArea, TorusTexture::NeedsRedraw);
}

void
BackendPainter::setDeviceTransformation() {

	_documentPainter.setDeviceTransformation(_scale, util::point<int>(0, 0));
	_documentCleanUpPainter->setDeviceTransformation(_scale, util::point<int>(0, 0));
	_overlayPainter.setDeviceTransformation(_scale, util::point<int>(0, 0));
}

void
BackendPainter::initiateFullRedraw(const util::rect<int>& roi) {

	LOG_DEBUG(backendpainterlog) << "initiate full redraw for roi " << roi << std::endl;

	_documentTexture->reset(roi.center());
	_documentPainter.resetIncrementalMemory();
	_overlayTexture->reset(roi.center());
}

bool
BackendPainter::prepareTextures(const util::rect<int>& pixelRoi) {

	if (!_documentTexture) {

		_documentTexture = boost::make_shared<TorusTexture>(pixelRoi);
		_documentTexture->setBackgroundRasterizer(_documentCleanUpPainter);

		_overlayTexture = boost::make_shared<TorusTexture>(pixelRoi);

		return true;
	}

	return false;
}

void
BackendPainter::drawTextures(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	if (_mode == Zooming) {

		glScaled(_scaleChange.x, _scaleChange.y, 1.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		_documentTexture->render(roi/_scaleChange, _documentPainter);
		glColor4f(1.0f, 1.0f, 0.5f, _overlayAlpha);
		_overlayTexture->render(roi/_scaleChange, _overlayPainter);

	} else {

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		_documentTexture->render(roi, _documentPainter);
		glColor4f(1.0f, 1.0f, 0.8f, _overlayAlpha);
		_overlayTexture->render(roi, _overlayPainter);
	}

	glPopMatrix();

	_documentPainter.rememberDrawnElements();
}
