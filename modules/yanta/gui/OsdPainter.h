#ifndef YANTA_OSD_PAINTER_H__
#define YANTA_OSD_PAINTER_H__

#include <gui/Painter.h>
#include <gui/Texture.h>

#include <tools/PenMode.h>

class OsdPainter : public gui::Painter {

public:

	OsdPainter(const PenMode& penMode = PenMode());

	void setPenMode(const PenMode& penMode) {

		_penMode = penMode;
	}

	bool draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

private:

	PenMode _penMode;
};

#endif // YANTA_OSD_PAINTER_H__

