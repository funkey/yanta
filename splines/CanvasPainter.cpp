#include <cmath>
#include <boost/make_shared.hpp>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_cairoPainter(gui::skia_pixel_t(255, 255, 255)),
	_cairoCleanUpPainter(gui::skia_pixel_t(127, 255, 255)),
	_prefetchLeft(1024),
	_prefetchRight(1024),
	_prefetchTop(1024),
	_prefetchBottom(1024),
	_shift(0, 0),
	_scale(1.0, 1.0),
	_previousShift(0, 0),
	_previousScale(0, 0),
	_previousPixelRoi(0, 0, 0, 0),
	_state(Moving) {

	_cairoPainter.setDeviceTransformation(_scale, _shift);
	_cairoCleanUpPainter.setDeviceTransformation(_scale, _shift);

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
	_cairoCleanUpPainter.setDeviceTransformation(_scale, util::point<double>(0.0, 0.0));
}

util::point<double>
CanvasPainter::screenToCanvas(const util::point<double>& point) {

	util::point<double> inv = point;

	inv -= _shift;
	inv /= _scale;

	return inv;
}

util::point<int>
CanvasPainter::canvasToTexture(const util::point<double>& point) {

	util::point<double> inv = point;

	inv *= _scale;

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

	if (prepareTexture(pixelRoi))
		initiateFullRedraw(pixelRoi);

	// transformation did not change
	if (_shift == _previousShift && _scale == _previousScale) {

		LOG_ALL(canvaspainterlog) << "transformation did not change -- I just quickly update the strokes" << std::endl;

		if (_state == Moving) {
		
			LOG_ALL(canvaspainterlog) << "I was moving, previously" << std::endl;
			startIncrementalDrawing(pixelRoi);
		}

		if (pixelRoi != _previousPixelRoi) {

			LOG_ALL(canvaspainterlog) << "The ROI changed" << std::endl;
			startIncrementalDrawing(pixelRoi);
		}

		updateStrokes(*_strokes, pixelRoi);

	// scale changed -- texture needs to be redrawn completely
	} else if (_scale != _previousScale) {

		LOG_ALL(canvaspainterlog) << "scale changed" << std::endl;

		_state = Moving;

		initiateFullRedraw(pixelRoi);

	// shift changed -- move prefetch texture
	} else {

		LOG_ALL(canvaspainterlog) << "transformation moved by " << (_shift - _previousShift) << std::endl;

		_state = Moving;

		// show a different part of the canvas texture
		_canvasTexture->shift(_shift - _previousShift);
	}

	drawTexture(pixelRoi);

	_previousShift    = _shift;
	_previousScale    = _scale;
	_previousPixelRoi = pixelRoi;
}

bool
CanvasPainter::prepareTexture(const util::rect<int>& pixelRoi) {

	unsigned int textureWidth  = pixelRoi.width()  + _prefetchLeft + _prefetchRight;
	unsigned int textureHeight = pixelRoi.height() + _prefetchTop  + _prefetchBottom;

	LOG_ALL(canvaspainterlog) << "with pre-fetch areas, texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	if (_canvasTexture && (_canvasTexture->width() < textureWidth || _canvasTexture->height() < textureHeight)) {

		LOG_ALL(canvaspainterlog) << "texture is of different size, create a new one" << std::endl;

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

	LOG_ALL(canvaspainterlog) << "refresh requested" << std::endl;
	_cairoPainter.resetIncrementalMemory();
}

void
CanvasPainter::updateStrokes(const Strokes& strokes, const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (_cairoPainter.alreadyDrawn(strokes)) {

		LOG_ALL(canvaspainterlog) << "nothing changed, skipping redraw" << std::endl;
		return;
	}

	_canvasTexture->fill(roi, _cairoPainter);
	_cairoPainter.rememberDrawnStrokes();
}

void
CanvasPainter::initiateFullRedraw(const util::rect<int>& roi) {

	_canvasTexture->reset(roi);
	_cairoPainter.resetIncrementalMemory();
}

void
CanvasPainter::markDirty(const util::rect<double>& area) {

	// area is in canvas units -- transform it to pixel units
	util::point<int> ul = canvasToTexture(area.upperLeft());
	util::point<int> lr = canvasToTexture(area.lowerRight());

	// add a border of one pixels to compensate for rounding artefacts
	util::rect<int> pixelArea(ul.x - 1, ul.y - 1, lr.x + 1, lr.y +1);

	// are we currently looking at this area?
	if (_state == IncrementalDrawing && pixelArea.intersects(_previousPixelRoi)) {

		LOG_ALL(canvaspainterlog) << "redrawing " << pixelArea << std::endl;

		gui::OpenGl::Guard guard;

		// in this case, redraw immediately
		_canvasTexture->fill(pixelArea, _cairoCleanUpPainter);

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
CanvasPainter::startIncrementalDrawing(const util::rect<int>& roi) {

	LOG_DEBUG(canvaspainterlog) << "resetting the incremental memory" << std::endl;

	if (roi.width() == 0)
		_canvasTexture->setWorkingArea(_previousPixelRoi);
	else
		_canvasTexture->setWorkingArea(roi);
	_cairoPainter.resetIncrementalMemory();

	_state = IncrementalDrawing;
}

void
CanvasPainter::drawTexture(const util::rect<int>& roi) {

	glPushMatrix();
	glTranslated(_shift.x, _shift.y, 0.0);

	_canvasTexture->render(roi);

	if (_canvasTexture->isDirty(roi)) {

		glColor4f(1.0, 0.3, 1.0, 0.3);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glVertex2d(roi.minX, roi.minY);
		glVertex2d(roi.maxX, roi.minY);
		glVertex2d(roi.maxX, roi.maxY);
		glVertex2d(roi.minX, roi.maxY);
		glEnd();
	}

	glPopMatrix();
}
