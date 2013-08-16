#include "TorusTexture.h"

logger::LogChannel torustexturelog("torustexturelog", "[TorusTexture] ");

TorusTexture::TorusTexture(const util::rect<int>& region) :
	_width (region.width() /TileSize + (region.width()  % TileSize == 0 ? 0 : 1)),
	_height(region.height()/TileSize + (region.height() % TileSize == 0 ? 0 : 1)),
	_outOfDates(boost::extents[_width][_height]),
	_mapping(_width, _height),
	_texture(0) {

	LOG_DEBUG(torustexturelog) << "creating new torus texture with " << _width << "x" << _height << " tiles to cover " << region << std::endl;

	// the tiles cache has to contain at least the tiles that we need
	assert(_width  <= TilesCache::Width);
	assert(_height <= TilesCache::Height);

	reset(region.center());

	gui::OpenGl::Guard guard;

	_texture = new gui::Texture(_width*TileSize, _height*TileSize, GL_RGBA);
}

TorusTexture::~TorusTexture() {

	gui::OpenGl::Guard guard;

	if (_texture)
		delete _texture;
}

void
TorusTexture::reset(const util::point<int>& center) {

	LOG_DEBUG(torustexturelog) << "reseting torus texture around " << center << std::endl;

	// get the tile containing the center
	util::point<int> centerTile = center/static_cast<int>(TileSize);

	// reset the tile mapping, such that all tiles around center map to 
	// [0,w)x[0,h)
	_mapping.reset(centerTile - util::point<int>(_width/2, _height/2));

	LOG_DEBUG(torustexturelog) << "new center tile is " << centerTile << std::endl;

	// set the accumulated shift such that center really is in the center of the 
	// texture
	_shift = center - centerTile*static_cast<int>(TileSize);

	LOG_DEBUG(torustexturelog) << "current shift is " << _shift << std::endl;

	// center the cache around our center tile as well
	_cache.reset(util::point<int>(centerTile.x, centerTile.y));

	// mark all tiles as need-update
	for (unsigned int x = 0; x < _width; x++)
		for (unsigned int y = 0; y < _height; y++)
			_outOfDates[x][y] = true;
}

void
TorusTexture::shift(const util::point<int>& shift) {

	_shift += shift;

	LOG_ALL(torustexturelog) << "shifting texture content by " << shift << ", accumulated shift is " << _shift << std::endl;

	// We are shifting content out of the region covered by this texture.
	//
	// If we shifted far enough to the right, such that a whole column of tiles 
	// got shifted out of the region, we take this column and insert it at the 
	// left. We mark the newly inserted tiles as dirty.

	while (_shift.x >= (int)TileSize) {

		_mapping.shift(util::point<int>(-1, 0));
		_cache.shift(util::point<int>(1, 0));
		_shift.x -= TileSize;

		// the new tiles are in the left column
		util::rect<int> tilesRegion = _mapping.get_region();
		int x = tilesRegion.minX;
		for (int y = tilesRegion.minY; y < tilesRegion.maxY; y++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}
	while (_shift.x <= -(int)TileSize) {

		_mapping.shift(util::point<int>(1, 0));
		_cache.shift(util::point<int>(-1, 0));
		_shift.x += TileSize;

		// the new tiles are in the right column
		util::rect<int> tilesRegion = _mapping.get_region();
		int x = tilesRegion.maxX - 1;
		for (int y = tilesRegion.minY; y < tilesRegion.maxY; y++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}
	while (_shift.y >= (int)TileSize) {

		_mapping.shift(util::point<int>(0, -1));
		_cache.shift(util::point<int>(0, 1));
		_shift.y -= TileSize;

		// the new tiles are in the top column
		util::rect<int> tilesRegion = _mapping.get_region();
		int y = tilesRegion.minY;
		for (int x = tilesRegion.minX; x < tilesRegion.maxX; x++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}
	while (_shift.y <= -(int)TileSize) {

		_mapping.shift(util::point<int>(0, 1));
		_cache.shift(util::point<int>(0, -1));
		_shift.y += TileSize;

		// the new tiles are in the bottom column
		util::rect<int> tilesRegion = _mapping.get_region();
		int y = tilesRegion.maxY - 1;
		for (int x = tilesRegion.minX; x < tilesRegion.maxX; x++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}

	LOG_ALL(torustexturelog) << "  cache region is now " << _mapping.get_region() << std::endl;
}

void
TorusTexture::markDirty(const util::rect<int>& region, DirtyFlag dirtyFlag) {

	// get the tiles in the region
	util::rect<int> tiles = getTiles(region);

	// mark them dirty
	for (int x = tiles.minX; x < tiles.maxX; x++)
		for (int y = tiles.minY; y < tiles.maxY; y++)
			markDirty(util::point<int>(x, y), dirtyFlag);
}

void
TorusTexture::render(const util::rect<int>& region, SkiaDocumentPainter& painter) {

	LOG_ALL(torustexturelog) << "called render for " << region << std::endl;

	// get the tiles in the region
	util::rect<int> tiles = getTiles(region);

	LOG_ALL(torustexturelog) << "tiles would be " << tiles << std::endl;

	// intersect it with the tiles that are used in the texture to make sure we 
	// are not drawing tiles that don't exist
	tiles = _mapping.get_region().intersection(tiles);

	LOG_ALL(torustexturelog) << "intersected with my tiles " << _mapping.get_region() << ", this gives " << tiles << std::endl;

	if (tiles.area() <= 0) {

		LOG_ALL(torustexturelog) << "I don't have tiles for this region" << std::endl;
		return;
	}

	// reload out-of-date tiles
	// TODO: make this more efficient (by using a queue of reload requests)
	for (int x = tiles.minX; x < tiles.maxX; x++)
		for (int y = tiles.minY; y < tiles.maxY; y++) {

			util::point<int> physicalTile = _mapping.map(util::point<int>(x, y));

			LOG_ALL(torustexturelog) << "testing tile " << util::point<int>(x, y) << ", physical " << physicalTile << std::endl;

			if (_outOfDates[physicalTile.x][physicalTile.y]) {

				util::point<int> tile(x, y);

				reloadTile(tile, physicalTile, painter);
			}
		}

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// The texture is in general split into four parts that we have to draw 
	// individually.

	util::rect<int> subtiles[4];
	_mapping.split(tiles, subtiles);

	_texture->bind();

	// for each part...
	for (int i = 0; i < 4; i++) {

		LOG_ALL(torustexturelog) << "attempting to draw part " << i << " with tiles " << subtiles[i] << std::endl;

		if (subtiles[i].area() <= 0)
			continue;

		LOG_ALL(torustexturelog) << "drawing texture part " << i << std::endl;

		// the region covered by this part in pixels
		util::rect<int> subregion = subtiles[i]*static_cast<int>(TileSize);

		util::point<int> physicalUpperLeft  = _mapping.map(subtiles[i].upperLeft());
		util::point<int> physicalLowerRight = physicalUpperLeft + util::point<int>(subtiles[i].width(), subtiles[i].height());

		// the texCoords as tiles in the texture
		util::rect<double> texCoords(physicalUpperLeft.x, physicalUpperLeft.y, physicalLowerRight.x, physicalLowerRight.y);
		// normalized to [0,1)x[0,1)
		texCoords /= util::point<double>(_width, _height);

		// draw the texture part
		glBegin(GL_QUADS);
		glTexCoord2d(texCoords.minX, texCoords.minY); glVertex2d(subregion.minX, subregion.minY);
		glTexCoord2d(texCoords.maxX, texCoords.minY); glVertex2d(subregion.maxX, subregion.minY);
		glTexCoord2d(texCoords.maxX, texCoords.maxY); glVertex2d(subregion.maxX, subregion.maxY);
		glTexCoord2d(texCoords.minX, texCoords.maxY); glVertex2d(subregion.minX, subregion.maxY);
		glEnd();
	}

	_texture->unbind();

	glDisable(GL_BLEND);
}

void
TorusTexture::setBackgroundPainter(boost::shared_ptr<SkiaDocumentPainter> painter) {

	_cache.setBackgroundPainter(painter);
}

util::rect<int>
TorusTexture::getTiles(const util::rect<int>& region) {

	util::rect<int> tiles;

	tiles.minX = getTileCoordinate(region.minX);
	tiles.minY = getTileCoordinate(region.minY);
	tiles.maxX = getTileCoordinate(region.maxX - 1) + 1;
	tiles.maxY = getTileCoordinate(region.maxY - 1) + 1;

	return tiles;
}

int
TorusTexture::getTileCoordinate(int pixel) {

	return pixel/static_cast<int>(TileSize) - (pixel < 0 ? 1 : 0);
}

void
TorusTexture::markDirty(const util::point<int>& tile, DirtyFlag dirtyFlag) {

	LOG_ALL(torustexturelog) << "marking dirty tile " << tile << std::endl;

	// mark the tile out-of-date in the texture
	if (_mapping.get_region().contains(tile)) {

		LOG_ALL(torustexturelog) << "    marking out-of-date tile " << tile << std::endl;

		util::point<int> physicalTile = _mapping.map(tile);
		_outOfDates[physicalTile.x][physicalTile.y] = true;

		LOG_ALL(torustexturelog) << "    physical tile is " << physicalTile << std::endl;
	}

	// mark the tile dirty in the cache
	if (dirtyFlag == NeedsRedraw)
		_cache.markDirty(tile, TilesCache::NeedsRedraw);
	else if (dirtyFlag == NeedsUpdate)
		_cache.markDirty(tile, TilesCache::NeedsUpdate);
}

void
TorusTexture::reloadTile(const util::point<int>& tile, const util::point<int>& physicalTile, SkiaDocumentPainter& painter) {

	LOG_ALL(torustexturelog) << "reloading tile " << tile << std::endl;
	LOG_ALL(torustexturelog) << "    physical tile is " << physicalTile << std::endl;

	// get the tile's data (and update it on-the-fly, if needed)
	gui::skia_pixel_t* data = _cache.getTile(tile, painter);

	// get the target area within the texture
	util::rect<int> textureRegion(physicalTile.x, physicalTile.y, physicalTile.x + 1, physicalTile.y + 1);
	textureRegion *= static_cast<int>(TileSize);

	// partially reload the texture
	_texture->loadData(data, textureRegion);

	// mark tile as up-to-date
	_outOfDates[physicalTile.x][physicalTile.y] = false;
}
