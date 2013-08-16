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
class TorusTexture;

class BackendPainter : public gui::Painter {

public:

	BackendPainter();

	void setDocument(boost::shared_ptr<Document> document) {

		_documentPainter.setDocument(document);
		_overlayPainter.setDocument(document);
		_documentCleanUpPainter->setDocument(document);
		_documentChanged = true;
	}

	void setTools(boost::shared_ptr<Tools> tools) {

		_overlayPainter.setTools(tools);
	}

	void setCursorPosition(const util::point<DocumentPrecision>& position) {

		LOG_ALL(backendpainterlog) << "cursor set to position " << position << std::endl;

		_cursorPosition = position;
	}

	bool draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	/**
	 * Give the painter a hint about added content.
	 */
	void contentAdded(const util::rect<DocumentPrecision>& region);

	/**
	 * Request a drag of the painter in pixel units.
	 */
	void drag(const util::point<DocumentPrecision>& direction);

	/**
	 * Request a zoom by the given scale, centered around anchor.
	 */
	void zoom(double zoomChange, const util::point<DocumentPrecision>& anchor);

	/**
	 * Initiate a full redraw with the current zoom level. Call this after a 
	 * sequence of zoom() requests.
	 */
	void finishZoom();

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
	 * Mark an area in the overlay dirty for redraws.
	 */
	void markOverlayDirty(const util::rect<DocumentPrecision>& area);

private:

	/**
	 * The possible states of the BackendPainter.
	 */
	enum BackendPainterMode {

		// TODO: do we need that?
		IncrementalDrawing,

		// user is dragging the texture
		Dragging,

		// user is zooming
		Zooming
	};

	/**
	 * Set the current device transformation in all painters.
	 */
	void setDeviceTransformation();

	void initiateFullRedraw(const util::rect<int>& roi);

	/**
	 * Prepare the textures for the given region.
	 */
	bool prepareTextures(const util::rect<int>& pixelRoi);

	/**
	 * Draw the textures' content that corresponds to roi into roi.
	 */
	void drawTextures(const util::rect<int>& roi);

	// indicates that the document was changed entirely
	bool _documentChanged;

	// the skia painter for the document
	SkiaDocumentPainter _documentPainter;

	// a skia painter for the background updates
	boost::shared_ptr<SkiaDocumentPainter> _documentCleanUpPainter;

	// a skia painter for the overlay
	SkiaOverlayPainter _overlayPainter;

	// the textures to draw to
	boost::shared_ptr<TorusTexture> _documentTexture;
	boost::shared_ptr<TorusTexture> _overlayTexture;

	// the transparency of the overlay texture
	float _overlayAlpha;

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

	// the current mode of the backend painter
	BackendPainterMode _mode;

	// the position of the cursor to draw in screen pixels
	util::point<double> _cursorPosition;
};

#endif // CANVAS_PAINTER_H__

