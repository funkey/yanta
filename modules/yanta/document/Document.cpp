#include <util/Logger.h>
#include "Document.h"

logger::LogChannel documentlog("documentlog", "[Document] ");

Document::Document() {}

Document::Document(Document& other) :
		pipeline::Data() {

	copyFrom(other);
}

Document&
Document::operator=(Document& other) {

	copyFrom(other);

	return *this;
}

void
Document::createPage(
		const util::point<DocumentPrecision>& position,
		const util::point<PagePrecision>&   size) {

	LOG_DEBUG(documentlog) << "created a new page" << std::endl;

	add<Page>(Page(this, position, size));
}

util::rect<DocumentPrecision>
Document::erase(const util::point<DocumentPrecision>& begin, const util::point<DocumentPrecision>& end) {

	_currentPage = getPageIndex(begin);

	return get<Page>(_currentPage).erase(begin, end);
}

util::rect<DocumentPrecision>
Document::erase(const util::point<DocumentPrecision>& position, DocumentPrecision radius) {

	_currentPage = getPageIndex(position);

	return get<Page>(_currentPage).erase(position, radius);
}

void
Document::copyFrom(Document& other) {

	_strokePoints = other._strokePoints;
	_currentPage  = other._currentPage;

	// We can't just copy pages, since they have a reference to the document they 
	// belong to. Therefore, we properly initialize our pages and copy the 
	// relevant parts, only.
	clear<Page>();

	for (unsigned int i = 0; i < other.numPages(); i++) {

		createPage(other.getPage(i).getShift(), other.getPage(i).getSize());
		get<Page>(i) = other.getPage(i);
	}
}
