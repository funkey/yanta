/**
 * yanta main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/timer/timer.hpp>

#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

#include <yanta/io/DocumentReader.h>
#include <yanta/gui/TilesCache.h>
#include <yanta/gui/SkiaDocumentPainter.h>
#include <yanta/util/ring_mapping.hpp>
#include <yanta/util/torus_mapping.hpp>

util::ProgramOption optionFilename(
		util::_long_name        = "file",
		util::_short_name       = "f",
		util::_description_text = "The file you want to open/save to",
		util::_default_value    = "",
		util::_is_positional    = true);

void testRing() {

	ring_mapping<int, 16> staticMapping;

	std::cout << "static mapping:" << std::endl;
	for (int i = 0; i < 16; i++) {

		std::cout << "    " << i << " => " << staticMapping.map(i) << std::endl;
		assert(i == staticMapping.map(i));
	}

	std::cout << "shift by 1" << std::endl;

	staticMapping.shift(1);

	std::cout << "static mapping:" << std::endl;
	for (int i = 1; i < 17; i++) {

		std::cout << "    " << i << " => " << staticMapping.map(i) << std::endl;
		if (i < 16)
			assert(i == staticMapping.map(i));
		else
			assert(0 == staticMapping.map(i));
	}

	std::cout << "shift by 1000" << std::endl;

	staticMapping.shift(1000);

	std::cout << "static mapping:" << std::endl;
	for (int i = 1001; i < 1017; i++)
		std::cout << "    " << i << " => " << staticMapping.map(i) << std::endl;

	int begins[2];
	int ends[2];

	staticMapping.split(1001, 1017, begins, ends);

	std::cout
			<< "    [" << begins[0] << ", " << ends[0] << ") "
			<< "    [" << begins[1] << ", " << ends[1] << ")" << std::endl;

	std::cout << "reset to 100" << std::endl;

	// set beginning of mapping to 100
	staticMapping.reset(100);

	for (int i = 100; i < 116; i++) {

		std::cout << "    " << i << " => " << staticMapping.map(i) << std::endl;
		assert(i - 100 == staticMapping.map(i));
	}

	std::cout << "shift by -1" << std::endl;

	staticMapping.shift(-1);

	for (int i = 99; i < 115; i++) {

		std::cout << "    " << i << " => " << staticMapping.map(i) << std::endl;
		if (i > 99)
			assert(i - 100 == staticMapping.map(i));
		else
			assert(15 == staticMapping.map(i));
	}
}

void testTorus() {

	torus_mapping<int, 16, 16> staticMapping;
	torus_mapping<int>         dynamicMapping(16, 16);

	staticMapping.shift(util::point<int>(100, 100));

	util::rect<int> subregions[4];

	staticMapping.split(util::rect<int>(100, 100, 110, 116), subregions);

	for (int i = 0; i < 4; i++)
		std::cout << "    " << subregions[i] << std::endl;
}

void testTilesCache(boost::shared_ptr<Document> document) {

	util::point<int> center(0, 0);
	TilesCache cache(center);

	boost::shared_ptr<SkiaDocumentPainter> painter = boost::make_shared<SkiaDocumentPainter>();
	painter->setDocument(document);

	boost::shared_ptr<SkiaDocumentPainter> bgpainter = boost::make_shared<SkiaDocumentPainter>();
	bgpainter->setDocument(document);

	cache.setBackgroundRasterizer(bgpainter);

	cache.getTile(util::point<int>(0, 0), *painter);

	cache.markDirty(util::point<int>(0, 0), TilesCache::NeedsUpdate);

	cache.shift(util::point<int>(1, 1));
	cache.getTile(util::point<int>(1, 1), *painter);

	sleep(5);

	cache.getTile(util::point<int>(0, 0), *painter);
	cache.getTile(util::point<int>(1, 1), *painter);
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

		std::string filename = optionFilename;

		// create process nodes
		pipeline::Process<DocumentReader> reader(filename);

		pipeline::Value<Document> document = reader->getOutput();

		testRing();

		testTorus();

		testTilesCache(document);

	} catch (Exception& e) {

		handleException(e, logger::out(logger::error));
	}
}

