/**
 * yanta main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/timer/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <gui/ContainerView.h>
#include <gui/Window.h>
#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

#include <yanta/Backend.h>
#include <yanta/BackendView.h>
#include <yanta/Osd.h>
#include <yanta/CanvasReader.h>
#include <yanta/CanvasWriter.h>
#include <yanta/CanvasPdfWriter.h>

util::ProgramOption optionFilename(
		util::_long_name        = "file",
		util::_short_name       = "f",
		util::_description_text = "The file you want to open/save to",
		util::_default_value    = "",
		util::_is_positional    = true);

util::ProgramOption optionShowMouseCursor(
		util::_long_name        = "showMouseCursor",
		util::_description_text = "Show the mouse cursor.");

util::ProgramOption optionExportPdf(
		util::_long_name        = "exportPdf",
		util::_description_text = "Export the yanta document as a pdf file.");

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

	boost::timer::cpu_timer timer;

	const boost::timer::nanosecond_type NanosBusyWait = 1000000LL;  // 1/1000th of a second
	const boost::timer::nanosecond_type NanosIdleWait = 10000000LL; // 1/100th of a second

	try {

		while (!window->closed()) {

			bool processedEvents = window->processEvents();

			boost::timer::cpu_times const elapsed(timer.elapsed());

			boost::timer::nanosecond_type waitAtLeast = (processedEvents ? NanosBusyWait : NanosIdleWait);

			if (elapsed.wall <= waitAtLeast)
				usleep((waitAtLeast - elapsed.wall)/1000);

			timer.stop();
			timer.start();
		}

	} catch (boost::exception& e) {

		handleException(e);
	}
}

std::string
dateString() {

	using namespace boost::posix_time;

	return to_simple_string(second_clock::local_time());
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
			std::string filename = (strlen(optionFilename.as<std::string>().c_str()) == 0 ? dateString() + ".yan" : optionFilename);

			// create process nodes
			pipeline::Process<gui::ContainerView<gui::OverlayPlacing> > overlayView;
			pipeline::Process<BackendView>                              backendView;
			pipeline::Process<Osd>                                      osd;
			pipeline::Process<Backend>                                  backend;
			pipeline::Process<CanvasReader>                             reader(filename);
			pipeline::Process<CanvasWriter>                             writer(filename);

			// connect process nodes
			window->setInput(overlayView->getOutput());
			overlayView->addInput(osd->getOutput("osd painter"));
			overlayView->addInput(backendView->getOutput());
			backendView->setInput("canvas", backend->getOutput("canvas"));
			backendView->setInput("overlay", backend->getOutput("overlay"));
			backend->setInput("initial canvas", reader->getOutput());
			backend->setInput("pen mode", osd->getOutput("pen mode"));
			backend->setInput("osd request", osd->getOutput("osd request"));
			writer->setInput(backend->getOutput());

			// enter window main loop
			processEvents(window);

			// TODO: this seems to workaround the Intel bufferdeallocate bug, 
			// investigate that!
			window->close();

			backend->cleanup();

			// save strokes
			writer->write();

			if (optionExportPdf) {

				pipeline::Process<CanvasPdfWriter> pdfWriter(optionExportPdf.as<std::string>());
				pdfWriter->setInput(backend->getOutput());
				pdfWriter->write();
			}

			// destruct pipeline as long as window still exists (workaround for 
			// OpenGl-bug)
			window->getInput().unset();
		}

	} catch (Exception& e) {

		handleException(e);
	}
}
