#ifndef YANTA_DOCUMENT_TREE_ROI_VISITOR_H__
#define YANTA_DOCUMENT_TREE_ROI_VISITOR_H__

#include "DocumentTreeTransformationVisitor.h"
#include "DocumentElement.h"
#include "DocumentElementContainer.h"

/**
 * Base class for document tree visitors, that only need to visit elements in a 
 * specified region of interest.
 */
class DocumentTreeRoiVisitor : public DocumentTreeTransformationVisitor {

public:

	DocumentTreeRoiVisitor() :
		_roi(0, 0, 0, 0) {}

	/**
	 * Set the region of interest for this visitor.
	 */
	void setRoi(const util::rect<DocumentPrecision>& roi) {

		_roi = roi;
		LOG_ALL(documenttreeroivisitorlog) << "roi set to " << _roi << std::endl;
	}

	/**
	 * Get the current roi of this visitor, corrected for the transformation 
	 * along the path to the current element.
	 */
	inline util::rect<DocumentPrecision> getRoi() { return getTransformation().getInverse().applyTo(_roi); }

	/**
	 * Traverse method for DocumentElementContainers. Calls accept() on each 
	 * element of the container that are part of the roi.
	 */
	template <typename Types, typename VisitorType>
	void traverse(DocumentElementContainer<Types>& container, VisitorType& visitor) {

		LOG_ALL(documenttreeroivisitorlog) << "changing roi " << _roi << " by transformation " << getTransformation() << std::endl;
		LOG_ALL(documenttreeroivisitorlog) << "new roi is " << (_roi - getTransformation().getShift())/getTransformation().getScale() << std::endl;
		LOG_ALL(documenttreeroivisitorlog) << "should be the same as " << getTransformation().getInverse().applyTo(_roi) << std::endl;
		LOG_ALL(documenttreeroivisitorlog) << "and " << getRoi() << std::endl;

		if (_roi.isZero()) {

			Traverser<VisitorType> traverser(visitor);
			container.for_each(traverser);

		} else {

			// visit all elements that intersect the roi
			RoiTraverser<VisitorType> traverser(visitor, getRoi());
			container.for_each(traverser);
		}
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

		RoiTraverser(VisitorType& visitor, const util::rect<DocumentPrecision>& roi) : _visitor(visitor), _roi(roi) {

			LOG_ALL(documenttreeroivisitorlog) << "created new traverser with roi " << roi << std::endl;
		}

		template <typename ElementType>
		void operator()(ElementType& element) {

			if (element.getBoundingBox().intersects(_roi)) {

				LOG_ALL(documenttreeroivisitorlog) << "element " << typeName(element) << " with roi " << element.getBoundingBox() << " does intersect roi " << _roi << std::endl;
				element.accept(_visitor);

			} else {

				LOG_ALL(documenttreeroivisitorlog) << "element " << typeName(element) << " with roi " << element.getBoundingBox() << " does not intersect roi " << _roi << std::endl;
			}
		}

	private:

		VisitorType& _visitor;
		util::rect<DocumentPrecision> _roi;
	};

	static logger::LogChannel documenttreeroivisitorlog;

	util::rect<DocumentPrecision> _roi;
};


#endif // YANTA_DOCUMENT_TREE_ROI_VISITOR_H__

