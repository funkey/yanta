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
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	/**
	 * Manually request a full redraw.
	 */
	void refresh();

private:

	util::rect<int> snapToPixelGrid(const util::rect<double>& roi, const util::point<double>& resolution);

	void drawStrokes(
			gui::cairo_pixel_t*     data,
			const Strokes&          strokes,
			const util::rect<int>&  dataArea,
			const util::point<int>& splitCenter,
			const util::rect<int>&  roi,
			bool                    incremental);

	void initiateFullRedraw(
			const util::rect<int>&     canvasPixelRoi,
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	bool sizeChanged(const util::rect<int>& canvasPixelRoi, const util::point<double>& resolution);

	// the strokes to draw
	boost::shared_ptr<Strokes> _strokes;

	// the cairo painter for the strokes
	CairoCanvasPainter _cairoPainter;

	/**
	 * Prepare the texture and buffers of the respective sizes.
	 */
	bool prepareTexture(const util::rect<int>& canvasPixelRoi);

	/**
	 * Draw the texture content that corresponds to roi into roi.
	 */
	void drawTexture(const util::rect<int>& roi);

	/**
	 * Update the texture strokes in the specified ROI.
	 */
	void updateStrokes(const Strokes& strokes, const util::rect<int>& roi);

	PrefetchTexture* _canvasTexture;

	// the number of pixels to add to the visible region for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// Mapping from integer texture coordinates to device units:
	//
	//   (x) = _deviceUnitsPerPixel*[x] + _deviceOffset,
	//
	// where (x) is device and [x] is texture.
	util::point<double> _deviceUnitsPerPixel;
	util::point<double> _deviceOffset;

	// Mapping from device untis to integer texture coordinates:
	//
	//   [x] = _pixelsPerDeviceUnit*[x] + _pixelOffset,
	//
	// where (x) is device and [x] is texture.
	util::point<double> _pixelsPerDeviceUnit;
	util::point<double> _pixelOffset;

	/*********
	 * CAIRO *
	 *********/

	// the cairo context
	cairo_t*            _context;

	// the cairo surface to draw to
	cairo_surface_t*    _surface;

	/********
	 * MISC *
	 ********/

	enum CanvasPainterState {

		IncrementalDrawing,
		Moving
	};

	CanvasPainterState _state;

	// the snapped pixel grid roi of the last call to draw
	util::rect<int>     _previousCanvasPixelRoi;
	util::point<double> _previousResolution;
};

#endif // CANVAS_PAINTER_H__

