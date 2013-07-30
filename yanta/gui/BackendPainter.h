#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <gui/Skia.h>
#include <gui/Painter.h>
#include <gui/Texture.h>

#include <document/Document.h>
#include <document/DocumentSignals.h>
#include <tools/Tools.h>
#include "SkiaDocumentPainter.h"
#include "SkiaOverlayPainter.h"

extern logger::LogChannel backendpainterlog;

// forward declaration
class TilingTexture;

class BackendPainter : public gui::Painter {

public:

	BackendPainter();

	void setDocument(boost::shared_ptr<Document> document) {

		_documentPainter.setDocument(document);
		_overlayPainter.setDocument(document);
		_documentCleanUpPainter.setDocument(document);
		_documentChanged = true;
	}

	void setTools(boost::shared_ptr<Tools> tools) {

		_overlayPainter.setTools(tools);
	}

	void setCursorPosition(const util::point<DocumentPrecision>& position) {

		LOG_ALL(backendpainterlog) << "cursor set to position " << position << std::endl;

		_cursorPosition = position;
	}

	/**
	 * Give the painter a hint about added content.
	 */
	void contentAdded(const util::rect<DocumentPrecision>& region);

	bool draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	/**
	 * Request a drag of the painter in pixel units.
	 */
	void drag(const util::point<DocumentPrecision>& direction);

	/**
	 * Request a zoom by the given scale, centered around anchor.
	 */
	void zoom(double zoomChange, const util::point<DocumentPrecision>& anchor);

	/**
	 * Prepare pen-drawing. Call this after a change to drag() or zoom().
	 */
	void prepareDrawing();

	/**
	 * Transform a point from screen pixel units to document units.
	 */
	util::point<DocumentPrecision> screenToDocument(const util::point<double>& p);

	/**
	 * Transform a point from document units to texture pixel units.
	 */
	util::point<int> documentToTexture(const util::point<DocumentPrecision>& p);

	/**
	 * Transform a rect from document units to texture pixel units.
	 */
	util::rect<int> documentToTexture(const util::rect<DocumentPrecision>& r);

	/**
	 * Manually request a full redraw.
	 */
	void refresh();

	/**
	 * Mark an area as dirty for redraws.
	 */
	void markDirty(const util::rect<DocumentPrecision>& area);

	/**
	 * Clean all dirty areas of the painter. Returns true, if there were some, 
	 * false otherwise.
	 */
	bool cleanDirtyAreas(unsigned int maxNumRequests);

	/**
	 * Initiate a redraw of a moved selection.
	 */
	void moveSelection(const SelectionMoved& signal);

private:

	/**
	 * The possible states of the BackendPainter.
	 */
	enum BackendPainterMode {

		IncrementalDrawing,
		Moving,
		Zooming
	};

	/**
	 * Set the current device transformation in all painters.
	 */
	void setDeviceTransformation();

	void initiateFullRedraw(const util::rect<int>& roi);

	/**
	 * Prepare the texture and buffers of the respective sizes.
	 */
	bool prepareTextures(const util::rect<int>& pixelRoi);

	/**
	 * Update the document in the specified ROI.
	 */
	void updateDocument(const util::rect<int>& roi);

	/**
	 * Update the overlay in the specified ROI.
	 */
	bool updateOverlay(const util::rect<int>& roi);

	/**
	 * Draw the texture content that corresponds to roi into roi.
	 */
	void drawTextures(const util::rect<int>& roi);

	// indicates that the document was changed entirely
	bool _documentChanged;

	// hint where content was added
	util::rect<int> _contentAddedRegion;

	// the skia painter for the document
	SkiaDocumentPainter _documentPainter;

	// a skia painter for the background updates
	SkiaDocumentPainter _documentCleanUpPainter;

	// a skia painter for the overlay
	SkiaOverlayPainter _overlayPainter;

	// the textures to draw to
	boost::shared_ptr<TilingTexture> _documentTexture;
	boost::shared_ptr<TilingTexture> _overlayTexture;

	// the transparency of the overlay texture
	float _overlayAlpha;

	// the number of pixels to add to the visible region for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// Mapping from document untis to screen pixel units:
	//
	//   [x] = _scale*(x) + _shift,
	//
	// where (x) is in document untis and [x] is in pixels and [x] = (0, 0) is the 
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

