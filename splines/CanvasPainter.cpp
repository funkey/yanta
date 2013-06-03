#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

void
CanvasPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	LOG_DEBUG(canvaspainterlog) << "redrawing canvas" << std::endl;

	if (!_strokes) {

		LOG_DEBUG(canvaspainterlog) << "no strokes to paint (yet)" << std::endl;
		return;
	}

	//updateStrokeTextures(roi, resolution);

	gui::OpenGl::Guard guard;

	for (int i = 0; i < _strokes->size(); i++) {

		for (int j = 1; j < (*_strokes)[i].size(); j++) {

			const util::point<double> prev = (*_strokes)[i][j - 1];
			const util::point<double> cur  = (*_strokes)[i][j];

			LOG_ALL(canvaspainterlog) << "drawing stroke piece from " << prev << " to  " << cur << std::endl;

			glBegin(GL_LINES);
			glVertex2f(prev.x, prev.y);
			glVertex2f(cur.x, cur.y);
			glEnd();
		}
	}
}
