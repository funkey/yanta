#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "OsdPainter.h"
#include "Osd.h"

logger::LogChannel osdpainterlog("osdpainterlog", "[OsdPainter] ");

OsdPainter::OsdPainter(const PenMode& penMode) :
	_penMode(penMode) {}

void
OsdPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	LOG_ALL(osdpainterlog) << "redrawing in " << roi << " with resolution " << resolution << std::endl;

	// the smallest integer pixel roi fitting the desired roi
	util::rect<int> pixelRoi;
	pixelRoi.minX = (int)floor(roi.minX);
	pixelRoi.minY = (int)floor(roi.minY);
	pixelRoi.maxX = (int)ceil(roi.maxX);
	pixelRoi.maxY = (int)ceil(roi.maxY);

	double width = _penMode.getStyle().width();
	unsigned char red   = _penMode.getStyle().getRed();
	unsigned char green = _penMode.getStyle().getGreen();
	unsigned char blue  = _penMode.getStyle().getBlue();

	gui::OpenGl::Guard guard;

	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0, 0.0, 0.0, 0.1 + (float)red/255.0*0.9);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(0, 100);
	glVertex2f(100, 100);
	glVertex2f(100, 0);
	glEnd();
	glColor4f(0.0, 1.0, 0.0, 0.1 + (float)green/255.0*0.9);
	glBegin(GL_QUADS);
	glVertex2f(0, 100);
	glVertex2f(0, 200);
	glVertex2f(100, 200);
	glVertex2f(100, 100);
	glEnd();
	glColor4f(0.0, 0.0, 1.0, 0.1 + (float)blue/255.0*0.9);
	glBegin(GL_QUADS);
	glVertex2f(0, 200);
	glVertex2f(0, 300);
	glVertex2f(100, 300);
	glVertex2f(100, 200);
	glEnd();

	glColor4f(0.0, 0.0, 0.0, (std::abs(Osd::Small - width) > 0.0001 ? 0 : 0.9) + 0.1);
	double x = 50;
	double y = 350;
	glBegin(GL_QUADS);
	glVertex2f(x - 1, y - 1);
	glVertex2f(x - 1, y + 1);
	glVertex2f(x + 1, y + 1);
	glVertex2f(x + 1, y - 1);
	glEnd();

	glColor4f(0.0, 0.0, 0.0, (std::abs(Osd::Normal - width) > 0.0001 ? 0 : 0.9) + 0.1);
	x = 50;
	y = 450;
	glBegin(GL_QUADS);
	glVertex2f(x - 2, y - 2);
	glVertex2f(x - 2, y + 2);
	glVertex2f(x + 2, y + 2);
	glVertex2f(x + 2, y - 2);
	glEnd();

	glColor4f(0.0, 0.0, 0.0, (std::abs(Osd::Big - width) > 0.0001 ? 0 : 0.9) + 0.1);
	x = 50;
	y = 550;
	glBegin(GL_QUADS);
	glVertex2f(x - 4, y - 4);
	glVertex2f(x - 4, y + 4);
	glVertex2f(x + 4, y + 4);
	glVertex2f(x + 4, y - 4);
	glEnd();

	glColor4f(0.0, 0.0, 0.0, (std::abs(Osd::Large - width) > 0.0001 ? 0 : 0.9) + 0.1);
	x = 50;
	y = 650;
	glBegin(GL_QUADS);
	glVertex2f(x - 8, y - 8);
	glVertex2f(x - 8, y + 8);
	glVertex2f(x + 8, y + 8);
	glVertex2f(x + 8, y - 8);
	glEnd();

	glColor4f(0, 0, 0, _penMode.getMode() == PenMode::Lasso ? 1.0 : 0.1);
	x = 50;
	y = 750;
	glBegin(GL_QUADS);
	glVertex2f(x - 10, y - 13);
	glVertex2f(x - 10, y - 7);
	glVertex2f(x + 10, y - 7);
	glVertex2f(x + 10, y - 13);
	glEnd();
	glBegin(GL_QUADS);
	glVertex2f(x - 13, y - 10);
	glVertex2f(x - 13, y + 10);
	glVertex2f(x - 7, y + 10);
	glVertex2f(x - 7, y - 10);
	glEnd();

	glColor3f(0.0, 0.0, 0.0);
	x = 50;
	y = 850;
	glBegin(GL_QUADS);
	glVertex2f(x - 10, y - 3);
	glVertex2f(x - 10, y + 3);
	glVertex2f(x + 10, y + 3);
	glVertex2f(x + 10, y - 3);
	glEnd();
	glBegin(GL_QUADS);
	glVertex2f(x - 3, y - 10);
	glVertex2f(x - 3, y + 10);
	glVertex2f(x + 3, y + 10);
	glVertex2f(x + 3, y - 10);
	glEnd();
}

