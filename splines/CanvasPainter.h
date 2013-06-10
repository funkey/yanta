#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <cairo.h>
#include <gui/Cairo.h>
#include <gui/Painter.h>
#include <gui/Texture.h>

#include "CairoCanvasPainter.h"
#include "PrefetchTexture.h"
#include "Strokes.h"

class CanvasPainter : public gui::Painter {

public:

	CanvasPainter();

	void setStrokes(boost::shared_ptr<Strokes> strokes) {

		_strokes = strokes;
		_cairoPainter.setStrokes(strokes);
		_cairoCleanUpPainter.setStrokes(strokes);
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
	 * Transform a point from pixel units to canvas units.
	 */
	util::point<double> pixelToCanvas(const util::point<double>& p);

	/**
	 * Manually request a full redraw.
	 */
	void refresh();

	/**
	 * Clean all dirty areas of the painter. Returns true, if there were some, 
	 * false otherwise.
	 */
	bool cleanDirtyAreas();

private:

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

	/**
	 * The possible states of the CanvasPainter.
	 */
	enum CanvasPainterState {

		IncrementalDrawing,
		Moving
	};

	// the strokes to draw
	boost::shared_ptr<Strokes> _strokes;

	// the cairo painter for the strokes
	CairoCanvasPainter _cairoPainter;

	// a cairo painter for the background updates
	CairoCanvasPainter _cairoCleanUpPainter;

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

	CanvasPainterState _state;
};

#endif // CANVAS_PAINTER_H__

