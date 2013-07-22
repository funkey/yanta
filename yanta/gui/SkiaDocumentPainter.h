#ifndef YANTA_SKIA_CANVAS_PAINTER_H__
#define YANTA_SKIA_CANVAS_PAINTER_H__

#include <SkCanvas.h>

#include <gui/Skia.h>
#include <util/rect.hpp>

#include <document/Document.h>
#include <document/DocumentTreeRoiVisitor.h>

class SkiaDocumentPainter : public DocumentTreeRoiVisitor {

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
	 * Set the document that shall be draw by subsequent draw() calls.
	 */
	void setDocument(boost::shared_ptr<Document> document) { _document = document; }

	/**
	 * Set the canvas to draw to.
	 */
	void setCanvas(SkCanvas& canvas) { _canvas = &canvas; }

	/**
	 * Set the transformation to map from document units to pixel units.
	 */
	void setDeviceTransformation(
			const util::point<double>& pixelsPerDeviceUnit,
			const util::point<int>&    pixelOffset) {

		_pixelsPerDeviceUnit = pixelsPerDeviceUnit;
		_pixelOffset         = pixelOffset;
	}

	/**
	 * Draw the whole document on the provided canvas.
	 */
	void draw(SkCanvas& canvas);

	/**
	 * Draw the document in the given ROI on the provided canvas. If 
	 * drawnUntilStroke is non-zero, only an incremental draw is performed, 
	 * starting from stroke drawnUntilStroke with point drawnUntilStrokePoint.
	 */
	void draw(
			SkCanvas& canvas,
			const util::rect<DocumentPrecision>& roi);

	/**
	 * Remember what was drawn already. Call this method prior an incremental 
	 * draw, to draw only new elements.
	 */
	void rememberDrawnDocument() {

		_drawnUntilStrokePoint = _drawnUntilStrokePointTmp;
		_incremental = true;
	}

	/**
	 * Return true if all the strokes of this document have been drawn by this 
	 * painter already.
	 */
	bool alreadyDrawn(const Document& document);

	/**
	 * Reset the memory about what has been drawn already. Call this method to 
	 * re-initialize incremental drawing.
	 */
	void resetIncrementalMemory() {

		_drawnUntilStrokePoint = 0;
		_incremental = false;
	}

	/**
	 * Visitor callback to enter a document element. Applies the transformation 
	 * that is stored in this element.
	 */
	void enter(DocumentElement& element);

	/**
	 * Visitor callback to leave a document element. Restores the transformation 
	 * that was changed for this element.
	 */
	void leave(DocumentElement& element);

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
	using DocumentTreeVisitor::visit;

private:

	/**
	 * Draw the paper boundary and lines.
	 */
	void drawPaper(Page& page);

	/**
	 * Draw a stroke.
	 */
	void drawStroke(Stroke& stroke);

	// the skia canvas to draw to
	SkCanvas* _canvas;

	// the background color
	gui::skia_pixel_t _clearColor;

	// shall the paper be drawn as well?
	bool _drawPaper;

	// the document to draw
	boost::shared_ptr<Document> _document;

	// the device transformation (document to skia canvas)
	util::point<double> _pixelsPerDeviceUnit;
	util::point<int>    _pixelOffset;

	// the number of the stroke point until which all have been drawn already
	unsigned long _drawnUntilStrokePoint;

	// temporal memory of the number of the stroke point until which all have 
	// been drawn already
	unsigned long _drawnUntilStrokePointTmp;

	// did we remember what we drew?
	bool _incremental;
};

#endif // YANTA_SKIA_CANVAS_PAINTER_H__

