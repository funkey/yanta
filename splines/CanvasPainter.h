#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <cairo.h>
#include <gui/Cairo.h>
#include <gui/Painter.h>
#include <gui/Texture.h>

#include "Strokes.h"

class CanvasPainter : public gui::Painter {

public:

	CanvasPainter();

	void setStrokes(boost::shared_ptr<Strokes> strokes) {

		_strokes = strokes;
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	/**
	 * Manually request a full redraw.
	 */
	void refresh();

private:

	void drawStrokes(
			gui::cairo_pixel_t* data,
			const Strokes& strokes,
			const util::rect<double>& roi,
			bool incremental);

	void drawStroke(
			cairo_t* context,
			const Stroke& stroke,
			bool incremental);

	void initiateFullRedraw(const util::rect<double>& roi, const util::point<double>& resolution);

	void clearSurface();

	/**
	 * Get the width multiplier for a pressure value.
	 */
	double widthPressureCurve(double pressure);

	/**
	 * Get the alpha multiplier for a pressure value.
	 */
	double alphaPressureCurve(double pressure);

	bool sizeChanged(const util::rect<double>& roi, const util::rect<double>& previousRoi);

	// the strokes to draw
	boost::shared_ptr<Strokes> _strokes;

	/******************
	 * CANVAS TEXTURE *
	 ******************/

	/**
	 * Shift the area represented by the canvas texture. Invalidates prefetched 
	 * areas accordingly to the previous ROI.
	 */
	void shiftTexture(const util::point<double>& shift);

	/**
	 * Draw the texture content the correspond to roi into roi.
	 */
	void drawTexture(const util::rect<double>& roi);

	/**
	 * Update the texture strokes in the specified ROI.
	 */
	void updateStrokes(const Strokes& strokes, const util::rect<double>& roi, bool incremental);

	/**
	 * Mark an area within the texture as dirty.
	 */
	void markDirty(const util::rect<double>& area);

	/**
	 * Process all dirty areas and clean them.
	 */
	void cleanDirtyAreas();

	// the OpenGl texture to draw to
	gui::Texture* _canvasTexture;

	// the screen resolution
	unsigned int _screenWidth;
	unsigned int _screenHeight;

	// the number of pixels to add to the visible region for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// the area covered by the canvas texture in device units
	util::rect<double> _textureArea;

	// the split center of the texture (where all four corners meet) in device 
	// units
	util::point<double> _splitCenter;

	std::vector<util::rect<double> > _dirtyAreas;

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

	// the roi of the last call to draw
	util::rect<double>  _previousRoi;

	// the number of the stroke until which all have been drawn already for the 
	// current configuration
	unsigned int _drawnUntilStroke;
	unsigned int _drawnUntilStrokePoint;
};

#endif // CANVAS_PAINTER_H__

