/**
 * yanta main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/thread.hpp>

#include <gui/ContainerView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

#include <yanta/Backend.h>
#include <yanta/CanvasView.h>
#include <yanta/Osd.h>
#include <yanta/CanvasReader.h>
#include <yanta/CanvasWriter.h>
#include <yanta/CanvasPdfWriter.h>

util::ProgramOption optionFilename(
		util::_long_name        = "file",
		util::_short_name       = "f",
		util::_description_text = "The file you want to open/save to",
		util::_default_value    = "strokes.dat",
		util::_is_positional    = true);

util::ProgramOption optionShowMouseCursor(
		util::_long_name        = "showMouseCursor",
		util::_description_text = "Show the mouse cursor.");

void handleException(boost::exception& e) {

	LOG_ERROR(logger::out) << "[window thread] caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(logger::out) << *boost::get_error_info<error_message>(e);

	LOG_ERROR(logger::out) << std::endl;

	LOG_ERROR(logger::out) << "[window thread] stack trace:" << std::endl;

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(logger::out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(logger::out) << std::endl;

	exit(-1);
}

void processEvents(pipeline::Process<gui::Window> window) {

	try {

		while (!window->closed()) {

			window->processEvents();
			usleep(100);
		}

	} catch (boost::exception& e) {

		handleException(e);
	}
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

		/******************
		 * SETUP PIPELINE *
		 ******************/

		// make sure, the window gets destructed as the last process (workaround 
		// for OpenGl-bug)
		gui::WindowMode mode;
		mode.hideCursor = !optionShowMouseCursor;
		mode.fullscreen = true;
		pipeline::Process<gui::Window>   window("yanta", mode);

		{
			// create process nodes
			pipeline::Process<gui::ContainerView<gui::OverlayPlacing> > overlayView;
			pipeline::Process<gui::ZoomView>                            zoomView;
			pipeline::Process<CanvasView>                               canvasView;
			pipeline::Process<Osd>                                      osd;
			pipeline::Process<Backend>                                  backend;
			pipeline::Process<CanvasReader>                             reader(optionFilename.as<std::string>());
			pipeline::Process<CanvasWriter>                             writer(optionFilename.as<std::string>());
			pipeline::Process<CanvasPdfWriter>                          pdfWriter(optionFilename.as<std::string>() + ".pdf");

			// connect process nodes
			window->setInput(zoomView->getOutput());
			zoomView->setInput(overlayView->getOutput());
			overlayView->addInput(osd->getOutput("osd painter"));
			overlayView->addInput(canvasView->getOutput());
			canvasView->setInput(backend->getOutput("canvas"));
			backend->setInput("initial canvas", reader->getOutput());
			backend->setInput("pen mode", osd->getOutput("pen mode"));
			backend->setInput("osd request", osd->getOutput("osd request"));
			writer->setInput(backend->getOutput());
			pdfWriter->setInput(backend->getOutput());

			// enter window main loop
			processEvents(window);

			// TODO: this seems to workaround the Intel bufferdeallocate bug, 
			// investigate that!
			window->close();

			// save strokes
			writer->write();
			pdfWriter->write();

			// destruct pipeline as long as window still exists (workaround for 
			// OpenGl-bug)
			window->getInput().unset();
		}

	} catch (Exception& e) {

		handleException(e);
	}
}
