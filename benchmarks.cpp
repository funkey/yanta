/**
 * yanta main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/timer/timer.hpp>

#include <gui/Buffer.h>
#include <gui/Texture.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

#include <gui/SkiaDocumentPainter.h>
#include <io/DocumentReader.h>

util::ProgramOption optionFilename(
		util::_long_name        = "file",
		util::_short_name       = "f",
		util::_description_text = "The file you want to open/save to",
		util::_default_value    = "",
		util::_is_positional    = true);

#define MEASURE(n, command, timer, name) \
	timer.start(); \
	for (int i = 0; i < n; i++) { command; } \
	std::cout \
		<< std::setprecision(4) << std::scientific \
		<< "    " << name << ":\t" \
		<< (double)timer.elapsed().wall/n/1.0e6 << " ms\t" \
		<< (double)timer.elapsed().system/n/1.0e6 << " ms\t" \
		<< (double)timer.elapsed().user/n/1.0e6 << " ms\t" \
		<< std::endl;

void createGlBuffer(unsigned int width, unsigned int height, gui::Buffer** buffer) {

	GLenum format = gui::detail::pixel_format_traits<gui::skia_pixel_t>::gl_format;
	GLenum type   = gui::detail::pixel_format_traits<gui::skia_pixel_t>::gl_type;

	if (*buffer)
		delete *buffer;

	*buffer = new gui::Buffer(width, height, format, type);
}

void mapGlBuffer(gui::Buffer* buffer, gui::skia_pixel_t** data) {

	if (*data)
		buffer->unmap();

	*data = buffer->map<gui::skia_pixel_t>();
}

void wrapBitmap(SkBitmap& bitmap, unsigned int width, unsigned int height, gui::skia_pixel_t* data) {

	// wrap the buffer in a skia bitmap
	bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
	bitmap.setPixels(data);
}

void testSkia(gui::skia_pixel_t* data, unsigned int width, unsigned int height, boost::shared_ptr<Document> document, boost::timer::cpu_timer& timer) {

	SkBitmap bitmap;

	MEASURE(1000000, wrapBitmap(bitmap, width, height, data), timer, "wrap skia bitmap");

	SkCanvas canvas(bitmap);

	MEASURE(1000000, SkCanvas foo(bitmap), timer, "wrap skia canvas");

	MEASURE(1000, canvas.drawColor(SkColorSetRGB(0, 0, 0)), timer, "clear skia canvas");

	SkiaDocumentPainter painter;
	painter.setDocument(document);

	MEASURE(10, painter.draw(canvas), timer, "draw document:");
}

void loadTexture(gui::Texture& texture, gui::skia_pixel_t* data) {

	texture.loadData(data);
}

void loadTexture(gui::Texture& texture, const util::rect<unsigned int>& roi, gui::skia_pixel_t* data) {

	texture.loadData(data, roi);
}

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		logger::LogManager::init();

		// init signal handler
		util::SignalHandler::init();

		std::cout << "all times in ms; wall, sys, user" << std::endl << std::endl;

		/******************
		 * SETUP PIPELINE *
		 ******************/

		{
			std::string filename = optionFilename;

			// create process nodes
			pipeline::Process<DocumentReader> reader(filename);

			pipeline::Value<Document> document = reader->getOutput();

			gui::OpenGl::Guard guard;

			const unsigned int width  = 1024;
			const unsigned int height = 1024;

			boost::timer::cpu_timer timer;

			gui::Buffer* glBuffer = 0;

			MEASURE(1000000, createGlBuffer(width, height, &glBuffer), timer, "create gl buffer");

			gui::skia_pixel_t* data = 0;

			MEASURE(1000000, mapGlBuffer(glBuffer, &data), timer, "map gl buffer   ");

			std::cout << std::endl << "testing skia on gl buffer" << std::endl << std::endl;

			testSkia(data, width, height, document, timer);

			glBuffer->unmap();

			data = new gui::skia_pixel_t[width*height];

			std::cout << std::endl << "testing skia on default buffer" << std::endl << std::endl;

			testSkia(data, width, height, document, timer);

			gui::Texture texture(width, height, GL_RGBA);

			std::cout << std::endl << "testing texture reloading from main memory" << std::endl << std::endl;

			MEASURE(1000, loadTexture(texture, data), timer, "load texture");

			MEASURE(1000, loadTexture(texture, util::rect<unsigned int>(100, 100, 356, 356), data), timer, "load texture (roi)");

			delete[] data;
		}

	} catch (Exception& e) {

		handleException(e, logger::out(logger::error));
	}
}
