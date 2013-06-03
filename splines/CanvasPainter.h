#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <gui/Painter.h>

#include "Strokes.h"

class CanvasPainter : public gui::Painter {

public:

	void setStrokes(boost::shared_ptr<Strokes> strokes) {

		_strokes = strokes;
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

private:

	boost::shared_ptr<Strokes> _strokes;
};

#endif // CANVAS_PAINTER_H__

