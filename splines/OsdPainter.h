#ifndef SPLINES_OSD_PAINTER_H__
#define SPLINES_OSD_PAINTER_H__

#include <gui/Painter.h>
#include <gui/Texture.h>

#include "PenMode.h"

class OsdPainter : public gui::Painter {

public:

	OsdPainter(const PenMode& penMode = PenMode());

	void setPenMode(const PenMode& penMode) {

		_penMode = penMode;
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

private:

	PenMode _penMode;
};

#endif // SPLINES_OSD_PAINTER_H__

