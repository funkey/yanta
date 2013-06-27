#include <fstream>

#include <SkPDFDevice.h>
#include <SkPDFDocument.h>

#include "CanvasPdfWriter.h"
#include "SkiaCanvasPainter.h"
#include <util/Logger.h>

logger::LogChannel canvaspdfwriterlog("canvaspdfwriterlog", "[CanvasPdfWriter] ");

CanvasPdfWriter::CanvasPdfWriter(const std::string& filename) :
		_filename(filename) {

	registerInput(_canvas, "canvas");
}

void
CanvasPdfWriter::write() {

	updateInputs();

	SkPDFDocument document;

	SkiaCanvasPainter painter;
	painter.setCanvas(_canvas);

	for (unsigned int i = 0; i < _canvas->numPages(); i++) {

		LOG_DEBUG(canvaspdfwriterlog) << "rendering pdf page " << i << std::endl;

		// page size in mm
		util::point<double> pageSize = _canvas->getPage(i).getSize();

		// According to the Skia documentation, there are 72 points/inch, which 
		// is 2.834646 points/mm.
		SkISize pagePointSize = SkISize::Make(pageSize.x*2.834646, pageSize.y*2.834646);

		SkMatrix identity;
		identity.reset();
		SkPDFDevice* pdfDevice = new SkPDFDevice(pagePointSize, pagePointSize, identity);
		SkAutoUnref autounref(pdfDevice);

		SkCanvas skiaCanvas(pdfDevice);

		// Now Skia expects the coordinates in points. We have mm. Scale ours by 
		// 2.834646.
		skiaCanvas.scale(2.834646, 2.834646);

		//painter.drawPaper(skiaCanvas, util::rect<double>(0, 0, pageSize.x, pageSize.y));
		painter.drawPage(skiaCanvas, _canvas->getPage(i), util::rect<double>(0, 0, pageSize.x, pageSize.y));

		document.appendPage(pdfDevice);
	}

	LOG_DEBUG(canvaspdfwriterlog) << "writing document" << std::endl;

	SkFILEWStream out(_filename.c_str());
	document.emitPDF(&out);
}
