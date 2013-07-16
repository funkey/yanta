#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <gui/Skia.h>
#include <gui/Painter.h>
#include <gui/Texture.h>

#include "SkiaCanvasPainter.h"
#include "SkiaOverlayPainter.h"
#include "PrefetchTexture.h"
#include "Canvas.h"
#include "Overlay.h"

extern logger::LogChannel backendpainterlog;

class BackendPainter : public gui::Painter {

public:

	BackendPainter();

	void setCanvas(boost::shared_ptr<Canvas> canvas) {

		_canvas = canvas;
		_canvasPainter.setCanvas(canvas);
		_canvasCleanUpPainter.setCanvas(canvas);
		_canvasChanged = true;
	}

	void setOverlay(boost::shared_ptr<Overlay> overlay) {

		_overlay = overlay;
		_overlayPainter.setOverlay(overlay);
	}

	void setCursorPosition(const util::point<CanvasPrecision>& position) {

		LOG_ALL(backendpainterlog) << "cursor set to position " << position << std::endl;

		_cursorPosition = position;
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	/**
	 * Request a drag of the painter in pixel units.
	 */
	void drag(const util::point<CanvasPrecision>& direction);

	/**
	 * Request a zoom by the given scale, centered around anchor.
	 */
	void zoom(double zoomChange, const util::point<CanvasPrecision>& anchor);

	/**
	 * Prepare for drawing. Call this after drag() or zoom().
	 */
	void prepareDrawing(const util::rect<int>& roi = util::rect<int>(0, 0, 0, 0));

	/**
	 * Transform a point from screen pixel units to canvas units.
	 */
	util::point<CanvasPrecision> screenToCanvas(const util::point<double>& p);

	/**
	 * Transform a point from canvas units to texture pixel units.
	 */
	util::point<int> canvasToTexture(const util::point<CanvasPrecision>& p);

	/**
	 * Manually request a full redraw.
	 */
	void refresh();

	/**
	 * Mark an area as dirty for redraws.
	 */
	void markDirty(const util::rect<CanvasPrecision>& area);

	/**
	 * Clean all dirty areas of the painter. Returns true, if there were some, 
	 * false otherwise.
	 */
	bool cleanDirtyAreas(unsigned int maxNumRequests);

private:

	/**
	 * The possible states of the BackendPainter.
	 */
	enum BackendPainterMode {

		IncrementalDrawing,
		Moving,
		Zooming
	};

	void initiateFullRedraw(const util::rect<int>& roi);

	/**
	 * Prepare the texture and buffers of the respective sizes.
	 */
	bool prepareTextures(const util::rect<int>& pixelRoi);

	/**
	 * Update the texture canvas in the specified ROI.
	 */
	void updateCanvas(const Canvas& canvas, const util::rect<int>& roi);

	/**
	 * Update the overlay in the specified ROI.
	 */
	void updateOverlay(const util::rect<int>& roi);

	/**
	 * Draw the texture content that corresponds to roi into roi.
	 */
	void drawTextures(const util::rect<int>& roi);

	// the canvas to draw
	boost::shared_ptr<Canvas> _canvas;

	// the overlay to draw on top of it
	boost::shared_ptr<Overlay> _overlay;

	// indicates that the canvas was changed entirely
	bool _canvasChanged;

	// the skia painter for the canvas
	SkiaCanvasPainter _canvasPainter;

	// a skia painter for the background updates
	SkiaCanvasPainter _canvasCleanUpPainter;

	// a skia painter for the overlay
	SkiaOverlayPainter _overlayPainter;

	// the textures to draw to
	boost::shared_ptr<PrefetchTexture> _canvasTexture;
	boost::shared_ptr<PrefetchTexture> _overlayTexture;

	// the number of pixels to add to the visible region for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// Mapping from canvas untis to screen pixel units:
	//
	//   [x] = _scale*(x) + _shift,
	//
	// where (x) is in canvas untis and [x] is in pixels and [x] = (0, 0) is the 
	// upper left screen pixel.
	util::point<double> _shift;
	util::point<double> _scale;
	// the scale change during zooming mode
	util::point<double> _scaleChange;
	// the center of the zoom during zooming mode
	util::point<double> _zoomAnchor;

	// the transform of the last call to draw
	util::point<int>    _previousShift;
	util::point<double> _previousScale;

	util::rect<int> _previousPixelRoi;

	// the prefetch parts
	util::rect<int> _left;
	util::rect<int> _right;
	util::rect<int> _bottom;
	util::rect<int> _top;

	BackendPainterMode _mode;

	// the position of the cursor to draw in screen pixels
	util::point<double> _cursorPosition;
};

#endif // CANVAS_PAINTER_H__
