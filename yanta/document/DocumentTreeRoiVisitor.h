#ifndef YANTA_DOCUMENT_TREE_ROI_VISITOR_H__
#define YANTA_DOCUMENT_TREE_ROI_VISITOR_H__

#include "DocumentTreeVisitor.h"
#include "DocumentElement.h"
#include "DocumentElementContainer.h"

/**
 * Base class for document tree visitors, that only need to visit elements in a 
 * specified region of interest.
 */
class DocumentTreeRoiVisitor : public DocumentTreeVisitor {

public:

	DocumentTreeRoiVisitor() :
		_roi(0, 0, 0, 0) {}

	/**
	 * Set the region of interest for this visitor.
	 */
	void setRoi(const util::rect<DocumentPrecision>& roi) { _roi = roi; }

	/**
	 * Get the current roi of this visitor.
	 */
	const util::rect<DocumentPrecision>& getRoi() { return _roi; }

	/**
	 * Traverse method for DocumentElementContainers. Calls accept() on each 
	 * element of the container that are part of the roi.
	 */
	template <typename Types, typename VisitorType>
	void traverse(DocumentElementContainer<Types>& container, VisitorType& visitor) {

		RoiTraverser<VisitorType> traverser(visitor, _roi);
		container.for_each(traverser);
	}

	// fallback implementation
	using DocumentTreeVisitor::traverse;

private:

	/**
	 * Functor to pass the visitor to each element of a container.
	 */
	template <typename VisitorType>
	class RoiTraverser {

	public:

		RoiTraverser(VisitorType& visitor, const util::rect<DocumentPrecision>& roi) : _visitor(visitor), _roi(roi) {}

		template <typename ElementType>
		void operator()(ElementType& element) {

			if (element.getBoundingBox().intersects(_roi))
				element.accept(_visitor);
			else
				LOG_ALL(documenttreevisitorlog) << "element " << typeName(element) << " with roi " << element.getBoundingBox() << " does not intersect roi " << _roi << std::endl;
		}

	private:

		VisitorType& _visitor;
		const util::rect<DocumentPrecision>& _roi;
	};

	static logger::LogChannel documenttreevisitorlog;

	util::rect<DocumentPrecision> _roi;
};


#endif // YANTA_DOCUMENT_TREE_ROI_VISITOR_H__

