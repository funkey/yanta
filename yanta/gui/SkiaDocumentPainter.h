#ifndef YANTA_SKIA_CANVAS_PAINTER_H__
#define YANTA_SKIA_CANVAS_PAINTER_H__

#include <gui/Skia.h>
#include <util/rect.hpp>

#include <document/Document.h>
#include "SkiaDocumentVisitor.h"

class SkiaDocumentPainter : public SkiaDocumentVisitor {

public:

	/**
	 * Create a new document painter.
	 *
	 * @param clearColor The background color.
	 * @param drawPaper  If true, the paper (color, lines) will be drawn.
	 */
	SkiaDocumentPainter(
			const gui::skia_pixel_t& clearColor = gui::skia_pixel_t(255, 255, 255),
			bool drawPaper = true);

	/**
	 * Draw the document in the given ROI on the provided canvas. If 
	 * drawnUntilStroke is non-zero, only an incremental draw is performed, 
	 * starting from stroke drawnUntilStroke with point drawnUntilStrokePoint.
	 */
	void draw(
			SkCanvas& canvas,
			const util::rect<DocumentPrecision>& roi = util::rect<DocumentPrecision>(0, 0, 0, 0));

	/**
	 * Remember what was drawn already. Call this method prior an incremental 
	 * draw, to draw only new elements.
	 */
	void rememberDrawnElements() {

		_drawnUntilStrokePoint = _drawnUntilStrokePointTmp;
		_incremental = true;
	}

	/**
	 * Returns false if there are no more elements in this document then the 
	 * ones rememembered to be drawn already with a call to 
	 * rememberDrawnElements().
	 */
	bool needRedraw();

	/**
	 * Reset the memory about what has been drawn already. Call this method to 
	 * re-initialize incremental drawing.
	 */
	void resetIncrementalMemory() {

		_drawnUntilStrokePoint = 0;
		_incremental = false;
	}

	/**
	 * Overload of the traverse method for this document visitor. Does not 
	 * process the content of Selections (this is handled by the 
	 * SkiaOverlayPainter).
	 */
	template <typename VisitorType>
	void traverse(Selection&, VisitorType&) {}

	// other business as usual
	using SkiaDocumentVisitor::traverse;

	/**
	 * Top-level visitor callback. Initializes data structures needed for the 
	 * following drawing operations.
	 */
	void visit(Document& document);

	/**
	 * Visitor callback to draw pages.
	 */
	void visit(Page& page);

	/**
	 * Visitor callback to draw strokes.
	 */
	void visit(Stroke& stroke);

	// default callbacks
	using SkiaDocumentVisitor::visit;

private:

	// the background color
	gui::skia_pixel_t _clearColor;

	// shall the paper be drawn as well?
	bool _drawPaper;

	// the number of the stroke point until which all have been drawn already
	unsigned long _drawnUntilStrokePoint;

	// temporal memory of the number of the stroke point until which all have 
	// been drawn already
	unsigned long _drawnUntilStrokePointTmp;

	// did we remember what we drew?
	bool _incremental;
};

#endif // YANTA_SKIA_CANVAS_PAINTER_H__

