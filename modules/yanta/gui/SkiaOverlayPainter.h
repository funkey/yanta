#ifndef YANTA_SKIA_OVERLAY_PAINTER_H__
#define YANTA_SKIA_OVERLAY_PAINTER_H__

#include <SkCanvas.h>

#include <util/point.hpp>
#include <util/rect.hpp>

#include <document/DocumentTreeRoiVisitor.h>
#include <tools/Tools.h>
#include "SkiaDocumentPainter.h"

class SkiaOverlayPainter : public SkiaDocumentPainter {

public:

	SkiaOverlayPainter();

	/**
	 * Set the tools to draw in the overlay.
	 */
	void setTools(boost::shared_ptr<Tools> tools) { _tools = tools; }

	/**
	 * Draw the document in the given ROI on the provided canvas. If 
	 * drawnUntilStroke is non-zero, only an incremental draw is performed, 
	 * starting from stroke drawnUntilStroke with point drawnUntilStrokePoint.
	 */
	void draw(
			SkCanvas& canvas,
			const util::rect<DocumentPrecision>& roi = util::rect<DocumentPrecision>(0, 0, 0, 0));

	/**
	 * Overload of the traverse method for this document visitor. Calls accept() 
	 * only on selections in a document.
	 */
	template <typename VisitorType>
	void traverse(Document& document, VisitorType& visitor) {

		RoiTraverser<VisitorType> traverser(visitor, getRoi());
		std::for_each(document.get<Selection>().begin(), document.get<Selection>().end(), traverser);
	}

	// proceed with others as defined in SkiaDocumentVisitor (not 
	// SkiaOverlayPainter, since this one doesn't traverse into Selections)
	using SkiaDocumentVisitor::traverse;

	/**
	 * Visitor callback for Documents. Do nothing.
	 */
	void visit(Document&) {}

	/**
	 * Visitor callback for Selections.
	 */
	void visit(Selection& selection);

	// fallback implementation
	using SkiaDocumentPainter::visit;

private:

	boost::shared_ptr<Tools> _tools;

	SkPaint _selectionPaint;
};

#endif // YANTA_SKIA_OVERLAY_PAINTER_H__

