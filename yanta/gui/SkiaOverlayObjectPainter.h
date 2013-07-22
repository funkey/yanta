#ifndef YANTA_SKIA_OVERLAY_OBJECT_PAINTER_H__
#define YANTA_SKIA_OVERLAY_OBJECT_PAINTER_H__

#include <SkCanvas.h>
#include "OverlayObjectVisitor.h"

class SkiaOverlayObjectPainter : public OverlayObjectVisitor {

public:

	SkiaOverlayObjectPainter(SkCanvas& canvas) :
		_canvas(canvas) {}

	void processLasso(const Lasso& lasso);

	void processSelection(const Selection& selection);

private:

	SkCanvas& _canvas;
};

#endif // YANTA_SKIA_OVERLAY_OBJECT_PAINTER_H__

