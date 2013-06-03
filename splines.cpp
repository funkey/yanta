/**
 * splines main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/thread.hpp>

#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

#include <splines/Canvas.h>

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

	LOG_USER(logger::out) << "[window thread] releasing shared pointer to window" << std::endl;

	LOG_USER(logger::out) << "[window thread] quitting" << std::endl;
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

		pipeline::Process<gui::Window>   window("splines");
		pipeline::Process<gui::ZoomView> zoomView;
		pipeline::Process<Canvas>        canvas;

		window->setInput(zoomView->getOutput());
		zoomView->setInput(canvas->getOutput("painter"));

		LOG_USER(logger::out) << "[main] starting..." << std::endl;

		processEvents(window);

		LOG_USER(logger::out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e);
	}
}
