#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <cairo.h>
#include <gui/Skia.h>
#include <gui/Painter.h>
#include <gui/Texture.h>

#include "SkiaCanvasPainter.h"
#include "PrefetchTexture.h"
#include "Strokes.h"

extern logger::LogChannel canvaspainterlog;

class CanvasPainter : public gui::Painter {

public:

	CanvasPainter();

	void setStrokes(boost::shared_ptr<Strokes> strokes) {

		_strokes = strokes;
		_cairoPainter.setStrokes(strokes);
		_cairoCleanUpPainter.setStrokes(strokes);
	}

	void setCursorPosition(const util::point<double>& position) {

		LOG_ALL(canvaspainterlog) << "cursor set to position " << position << std::endl;

		_cursorPosition = position;
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	/**
	 * Request a drag of the painter in pixel units.
	 */
	void drag(const util::point<double>& direction);

	/**
	 * Request a zoom by the given scale, centered around anchor.
	 */
	void zoom(double zoomChange, const util::point<double>& anchor);

	/**
	 * Prepare for drawing. Call this after drag() or zoom().
	 */
	void prepareDrawing(const util::rect<int>& roi = util::rect<int>(0, 0, 0, 0));

	/**
	 * Transform a point from screen pixel units to canvas units.
	 */
	util::point<double> screenToCanvas(const util::point<double>& p);

	/**
	 * Transform a point from canvas units to texture pixel units.
	 */
	util::point<int> canvasToTexture(const util::point<double>& p);

	/**
	 * Manually request a full redraw.
	 */
	void refresh();

	/**
	 * Mark an area as dirty for redraws.
	 */
	void markDirty(const util::rect<double>& area);

	/**
	 * Clean all dirty areas of the painter. Returns true, if there were some, 
	 * false otherwise.
	 */
	bool cleanDirtyAreas(unsigned int maxNumRequests);

private:

	/**
	 * The possible states of the CanvasPainter.
	 */
	enum CanvasPainterMode {

		IncrementalDrawing,
		Moving,
		Zooming
	};

	void initiateFullRedraw(const util::rect<int>& roi);

	/**
	 * Prepare the texture and buffers of the respective sizes.
	 */
	bool prepareTexture(const util::rect<int>& pixelRoi);

	/**
	 * Update the texture strokes in the specified ROI.
	 */
	void updateStrokes(const Strokes& strokes, const util::rect<int>& roi);

	/**
	 * Draw the texture content that corresponds to roi into roi.
	 */
	void drawTexture(const util::rect<int>& roi);

	// the strokes to draw
	boost::shared_ptr<Strokes> _strokes;

	// the cairo painter for the strokes
	SkiaCanvasPainter _cairoPainter;

	// a cairo painter for the background updates
	SkiaCanvasPainter _cairoCleanUpPainter;

	// the texture to draw to
	boost::shared_ptr<PrefetchTexture> _canvasTexture;

	// the number of pixels to add to the visible region for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// Mapping from canvas untis to pixel units:
	//
	//   [x] = _scale*(x) + _shift,
	//
	// where (x) is in canvas untis and [x] is in pixels. The shift is always 
	// integer, since it is in pixels and we don't want to shift with sub-pixel 
	// accuracy.
	util::point<int>    _shift;
	util::point<double> _scale;

	// the transform of the last call to draw
	util::point<int>    _previousShift;
	util::point<double> _previousScale;

	util::rect<int> _previousPixelRoi;

	CanvasPainterMode _mode;

	// the position of the cursor to draw in screen pixels
	util::point<double> _cursorPosition;
};

#endif // CANVAS_PAINTER_H__
