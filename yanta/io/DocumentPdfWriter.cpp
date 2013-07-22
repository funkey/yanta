#include <fstream>

#include <SkPDFDevice.h>
#include <SkPDFDocument.h>

#include <util/Logger.h>

#include <gui/SkiaDocumentPainter.h>

#include "DocumentPdfWriter.h"

logger::LogChannel documentpdfwriterlog("documentpdfwriterlog", "[DocumentPdfWriter] ");

DocumentPdfWriter::DocumentPdfWriter(const std::string& filename) :
		_filename(filename) {

	registerInput(_document, "document");
}

void
DocumentPdfWriter::write() {

	updateInputs();

	SkPDFDocument document;

	SkiaDocumentPainter painter;
	painter.setDocument(_document);

	for (unsigned int i = 0; i < _document->numPages(); i++) {

		Page& page = _document->getPage(i);

		LOG_DEBUG(documentpdfwriterlog) << "rendering pdf page " << i << std::endl;

		// page size in mm
		util::point<double> pageSize = page.getSize();

		// According to the Skia documentation, there are 72 points/inch, which 
		// is 2.834646 points/mm.
		SkISize pagePointSize = SkISize::Make(pageSize.x*2.834646, pageSize.y*2.834646);

		SkMatrix identity;
		identity.reset();
		SkPDFDevice* pdfDevice = new SkPDFDevice(pagePointSize, pagePointSize, identity);
		SkAutoUnref autounref(pdfDevice);

		SkCanvas canvas(pdfDevice);

		// make upper left of page (0, 0)
		canvas.translate(-page.getShift().x, -page.getShift().y);

		// Now Skia expects the coordinates in points. We have mm. Scale ours by 
		// 2.834646.
		canvas.scale(2.834646, 2.834646);

		painter.draw(canvas, page.getPageBoundingBox());

		document.appendPage(pdfDevice);
	}

	LOG_DEBUG(documentpdfwriterlog) << "writing document" << std::endl;

	SkFILEWStream out(_filename.c_str());
	document.emitPDF(&out);
}
