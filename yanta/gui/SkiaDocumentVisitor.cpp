#include "SkiaDocumentVisitor.h"

logger::LogChannel skiadocumentvisitorlog("skiadocumentvisitorlog", "[SkiaDocumentVisitor] ");

SkiaDocumentVisitor::SkiaDocumentVisitor() :
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0) {}

void
SkiaDocumentVisitor::prepare() {

	if (!getRoi().isZero()) {

		// clip outside our responsibility
		_canvas->clipRect(SkRect::MakeLTRB(getRoi().minX, getRoi().minY, getRoi().maxX, getRoi().maxY));

		// transform roi downwards
		setRoi((getRoi() - _pixelOffset)/_pixelsPerDeviceUnit);
	}

	// apply the device transformation
	_canvas->save();
	_canvas->translate(_pixelOffset.x, _pixelOffset.y);
	_canvas->scale(_pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);
}

void
SkiaDocumentVisitor::finish() {

	// restore original transformation
	_canvas->restore();
}

void
SkiaDocumentVisitor::enter(DocumentElement& element) {

	const util::point<DocumentPrecision>& shift = element.getShift();
	const util::point<DocumentPrecision>& scale = element.getScale();

	_canvas->save();

	if (!getRoi().isZero()) {

		// transform roi downwards
		util::rect<double> transformedRoi = getRoi();
		transformedRoi -= shift;
		transformedRoi /= scale;
		setRoi(transformedRoi);
	}

	// apply the element transformation
	_canvas->translate(shift.x, shift.y);
	_canvas->scale(scale.x, scale.y);
}

void
SkiaDocumentVisitor::leave(DocumentElement& element) {

	const util::point<DocumentPrecision>& shift = element.getShift();
	const util::point<DocumentPrecision>& scale = element.getScale();

	_canvas->restore();

	if (!getRoi().isZero()) {

		// transform roi upwards
		util::rect<DocumentPrecision> roi = getRoi();
		roi *= scale;
		roi += shift;
		setRoi(roi);
	}
}
