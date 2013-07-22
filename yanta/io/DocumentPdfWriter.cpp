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

		LOG_DEBUG(documentpdfwriterlog) << "rendering pdf page " << i << std::endl;

		// page size in mm
		util::point<double> pageSize = _document->getPage(i).getSize();

		// According to the Skia documentation, there are 72 points/inch, which 
		// is 2.834646 points/mm.
		SkISize pagePointSize = SkISize::Make(pageSize.x*2.834646, pageSize.y*2.834646);

		SkMatrix identity;
		identity.reset();
		SkPDFDevice* pdfDevice = new SkPDFDevice(pagePointSize, pagePointSize, identity);
		SkAutoUnref autounref(pdfDevice);

		SkCanvas canvas(pdfDevice);

		// Now Skia expects the coordinates in points. We have mm. Scale ours by 
		// 2.834646.
		canvas.scale(2.834646, 2.834646);

		//painter.drawPaper(canvas, util::rect<double>(0, 0, pageSize.x, pageSize.y));
		painter.drawPage(canvas, _document->getPage(i), util::rect<double>(0, 0, pageSize.x, pageSize.y));

		document.appendPage(pdfDevice);
	}

	LOG_DEBUG(documentpdfwriterlog) << "writing document" << std::endl;

	SkFILEWStream out(_filename.c_str());
	document.emitPDF(&out);
}
