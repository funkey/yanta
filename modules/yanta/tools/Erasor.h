#ifndef YANTA_TOOLS_ERASOR_H__
#define YANTA_TOOLS_ERASOR_H__

#include <document/Document.h>
#include <document/DocumentTreeRoiVisitor.h>

class Erasor : public DocumentTreeRoiVisitor {

public:

	enum Mode {

		/**
		 * Erases elements entirely that intersect the path of the erasor tool.
		 */
		ElementErasor,

		/**
		 * Erases everything within a certain distance to the erasor.
		 */
		SphereErasor
	};

	Erasor(Document& document, Mode mode = SphereErasor);

	/**
	 * Erase elements in the given document between start and end. What exactly 
	 * that means, i.e., which elements are selected for removal, depends on the 
	 * current erasor mode.
	 *
	 * @param document
	 *             The document to erase in.
	 * @param start
	 *             The start of the erasor line.
	 * @param end
	 *             The end of the erasor line.
	 *
	 * @return The bounding box of the erased elements.
	 */
	util::rect<DocumentPrecision> erase(
			const util::point<DocumentPrecision>& start,
			const util::point<DocumentPrecision>& end);

	/**
	 * Set the ersor mode.
	 */
	void setMode(Mode mode) { _mode = mode; }

	/**
	 * Set the radius of the erasor.
	 */
	void setRadius(DocumentPrecision radius) { _radius = radius; }

	/**
	 * Visitor callback for stokes.
	 */
	void visit(Page& page) { _currentPage = &page; }

	/**
	 * Visitor callback for stokes.
	 */
	void visit(Stroke& stroke);


	// default for other elements
	using DocumentTreeRoiVisitor::visit;

private:

	/**
	 * Erase points from a stroke by splitting. Reports the changed area.
	 */
	util::rect<PagePrecision> erase(
			Stroke* stroke,
			const util::point<PagePrecision>& position,
			PagePrecision radius);

	/**
	 * Erase the given stroke if it intersects the line. Reports changed area.
	 */
	util::rect<PagePrecision> erase(
			Stroke& stroke,
			const util::point<PagePrecision>& lineBegin,
			const util::point<PagePrecision>& lineEnd);

	/**
	 * Test, whether a line intersects a circle. Radius is given squared.
	 */
	bool intersectsErasorCircle(
			const util::point<PagePrecision> lineStart,
			const util::point<PagePrecision> lineEnd,
			const util::point<PagePrecision> center,
			PagePrecision                    radius2);

	/**
	 * Test, whether the lines p + t*r and q + u*s, with t and u in [0,1], 
	 * intersect.
	 */
	bool intersectLines(
			const util::point<PagePrecision>& p,
			const util::point<PagePrecision>& r,
			const util::point<PagePrecision>& q,
			const util::point<PagePrecision>& s);

	Document& _document;

	Page* _currentPage;

	StrokePoints& _strokePoints;

	Mode _mode;

	DocumentPrecision _radius;

	// start and end of the erasor line
	util::point<DocumentPrecision> _start;
	util::point<DocumentPrecision> _end;

	util::rect<DocumentPrecision> _changed;
};

#endif // YANTA_TOOLS_ERASOR_H__

