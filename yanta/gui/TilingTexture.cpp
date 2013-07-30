#include <SkBitmap.h>
#include <SkCanvas.h>

#include <gui/Skia.h>
#include <gui/OpenGl.h>
#include "TilingTexture.h"

logger::LogChannel tilingtexturelog("tilingtexturelog", "[TilingTexture] ");

TilingTexture::TilingTexture(const util::point<int>& center) {

	LOG_ALL(tilingtexturelog) << "creating new tiling texture around " << center << std::endl;

	for (unsigned int x = 0; x < TilesX; x++)
		for (unsigned int y = 0; y < TilesY; y++)
			_tiles[x][y] = new gui::Texture(TileSize, TileSize, GL_RGBA);

	reset(center);
}

TilingTexture::~TilingTexture() {

	gui::OpenGl::Guard guard;

	for (unsigned int x = 0; x < TilesX; x++)
		for (unsigned int y = 0; y < TilesY; y++)
			delete _tiles[x][y];
}

void
TilingTexture::shift(const util::point<int>& shift) {

	LOG_ALL(tilingtexturelog) << "shifting texture content by " << shift << std::endl;

	_shift += shift;

	// We are shifting content out of the region covered by this texture.
	//
	// If we shifted far enough to the right, such that a whole column of tiles 
	// got shifted out of the region, we take this column and insert it at the 
	// left. We correct the top left tile position, shift the region represented 
	// by the tiles and mark the newly inserted tiles as dirty.

	while (_shift.x >= (int)TileSize) {
		_topLeftTile.x++;
		if (_topLeftTile.x >= (int)TilesX) _topLeftTile.x -= TilesX;
		_tilesRegion -= util::point<int>((int)TileSize, 0);
		_shift.x -= TileSize;
		int x = (TilesX - _topLeftTile.x) % TilesX;
		for (int y = 0; y < (int)TilesY; y++)
			markDirty(util::point<int>(x, y));
	}
	while (_shift.x <= -(int)TileSize) {
		_topLeftTile.x--;
		if (_topLeftTile.x <  0) _topLeftTile.x += TilesX;
		_tilesRegion += util::point<int>((int)TileSize, 0);
		_shift.x += TileSize;
		int x = (TilesX - 1) - _topLeftTile.x;
		for (int y = 0; y < (int)TilesY; y++)
			markDirty(util::point<int>(x, y));
	}
	while (_shift.y >= (int)TileSize) {
		_topLeftTile.y++;
		if (_topLeftTile.y >= (int)TilesY) _topLeftTile.y -= TilesY;
		_tilesRegion -= util::point<int>(0, (int)TileSize);
		_shift.y -= TileSize;
		int y = (TilesY - _topLeftTile.y) % TilesY;
		for (int x = 0; x < (int)TilesX; x++)
			markDirty(util::point<int>(x, y));
	}
	while (_shift.y <= -(int)TileSize) {
		_topLeftTile.y--;
		if (_topLeftTile.y <  0) _topLeftTile.y += TilesY;
		_tilesRegion += util::point<int>(0, (int)TileSize);
		_shift.y += TileSize;
		int y = (TilesY - 1) - _topLeftTile.y;
		for (int x = 0; x < (int)TilesX; x++)
			markDirty(util::point<int>(x, y));
	}

	LOG_ALL(tilingtexturelog) << "  texture region is now " << _tilesRegion << std::endl;
	LOG_ALL(tilingtexturelog) << "  top left tile is " << _topLeftTile << std::endl;
}

void
TilingTexture::reset(const util::point<int>& center) {

	_tilesRegion.minX = center.x - TilesX/2*TileSize;
	_tilesRegion.minY = center.y - TilesY/2*TileSize;
	_tilesRegion.maxX = _tilesRegion.minX + TilesX*TileSize;
	_tilesRegion.maxY = _tilesRegion.minY + TilesY*TileSize;

	_topLeftTile = util::point<int>(0, 0);

	_shift = util::point<int>(0, 0);

	// dismiss all pending clean-up requests
	_cleanUpRequests.clear();

	// mark everything as dirty
	markDirty(_tilesRegion);
}

void
TilingTexture::render(const util::rect<int>& roi) {

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// The texture's ROI is in general split into four parts that we have to 
	// draw individually. On the fly, make sure that the requested region is not 
	// larger than the one that is covered by the tiles.

	// the texCoords
	util::rect<double> texCoords;

	util::rect<int>  subregions[4];

	split(roi.intersection(_tilesRegion), subregions);

	// for each part...
	for (int i = 0; i < 4; i++) {

		if (subregions[i].area() <= 0)
			continue;

		LOG_ALL(tilingtexturelog) << "drawing texture part " << i << std::endl;

		util::rect<int> tiles = getTiles(subregions[i]);

		// ...for each tile in this part...
		for (int x = tiles.minX; x < tiles.maxX; x++) {
			for (int y = tiles.minY; y < tiles.maxY; y++) {

				util::point<int> tileInArray(x, y);
				util::rect<int>  tileRegion = getTileRegion(tileInArray);

				boost::mutex::scoped_lock lock(_tiles[x][y]->getMutex());
				_tiles[x][y]->bind();

				//LOG_ALL(tilingtexturelog) << "drawing tile " << tileInArray << " at " << tileRegion << std::endl;
				glColor3f(fmod(3.432432*x,1.0), fmod(3.43243413*y,1.0), fmod(0.10990232*x*y,1.0));

				// draw the whole tile
				glBegin(GL_QUADS);
				glTexCoord2d(0, 0); glVertex2d(tileRegion.minX, tileRegion.minY);
				glTexCoord2d(1, 0); glVertex2d(tileRegion.maxX, tileRegion.minY);
				glTexCoord2d(1, 1); glVertex2d(tileRegion.maxX, tileRegion.maxY);
				glTexCoord2d(0, 1); glVertex2d(tileRegion.minX, tileRegion.maxY);
				glEnd();
			}
		}
	}

	glDisable(GL_BLEND);
}

void
TilingTexture::markDirty(const util::rect<int>& region) {

	LOG_ALL(tilingtexturelog) << "marking region " << region << " as dirty" << std::endl;

	util::rect<int>  subregions[4];

	split(region.intersection(_tilesRegion), subregions);

	// for each part...
	for (int i = 0; i < 4; i++) {

		if (subregions[i].area() <= 0)
			continue;

		util::rect<int> tiles = getTiles(subregions[i]);

		// ...for each tile in this part...
		for (int x = tiles.minX; x < tiles.maxX; x++) {
			for (int y = tiles.minY; y < tiles.maxY; y++) {

				util::point<int> tileInArray(x, y);

				CleanUpRequest request(tileInArray, getTileRegion(tileInArray));
				_cleanUpRequests.push_back(request);
			}
		}
	}
}

void
TilingTexture::markDirty(const util::point<int>& tile) {

	LOG_ALL(tilingtexturelog) << "marking dirty tile " << tile << std::endl;

	CleanUpRequest request(tile, getTileRegion(tile));
	_cleanUpRequests.push_back(request);
}

void
TilingTexture::split(
		const util::rect<int>& region,
		util::rect<int>*       subregions) {

	LOG_ALL(tilingtexturelog) << "splitting region " << region << std::endl;

	// split point in pixels
	util::point<int> splitPoint = _tilesRegion.upperLeft() + _topLeftTile*util::point<int>(TileSize, TileSize);

	LOG_ALL(tilingtexturelog) << "split point is at " << splitPoint << std::endl;

	// upper left
	subregions[0].minX = std::max(splitPoint.x, region.minX);
	subregions[0].minY = std::max(splitPoint.y, region.minY);
	subregions[0].maxX = region.maxX;
	subregions[0].maxY = region.maxY;

	// upper right
	subregions[1].minX = region.minX;
	subregions[1].minY = std::max(splitPoint.y, region.minY);
	subregions[1].maxX = std::min(splitPoint.x, region.maxX);
	subregions[1].maxY = region.maxY;

	// lower left
	subregions[2].minX = std::max(splitPoint.x, region.minX);
	subregions[2].minY = region.minY;
	subregions[2].maxX = region.maxX;
	subregions[2].maxY = std::min(splitPoint.y, region.maxY);

	// lower right
	subregions[3].minX = region.minX;
	subregions[3].minY = region.minY;
	subregions[3].maxX = std::min(splitPoint.x, region.maxX);
	subregions[3].maxY = std::min(splitPoint.y, region.maxY);

	for (int i = 0; i < 4; i++) {

		if (subregions[i].width() <= 0 || subregions[i].height() <= 0) {

			subregions[i] = util::rect<int>(0, 0, 0, 0);
			continue;
		}
	}
}

util::rect<int>
TilingTexture::getTiles(const util::rect<int>& region) {

	LOG_ALL(tilingtexturelog) << "getting tiles for region " << region << std::endl;

	if (!region.intersects(_tilesRegion)) {

		LOG_ALL(tilingtexturelog) << "  region does not intersect any tile -- returning empty rect" << std::endl;
		return util::rect<int>(0, 0, 0, 0);
	}

	util::rect<int> tiles = region - _tilesRegion.upperLeft();

	LOG_ALL(tilingtexturelog) << "  relative to tiles region: " << tiles << std::endl;

	// get the tiles in content space
	tiles.minX = tiles.minX/TileSize;
	tiles.minY = tiles.minY/TileSize;
	tiles.maxX = (tiles.maxX - 1)/TileSize + 1;
	tiles.maxY = (tiles.maxY - 1)/TileSize + 1;

	LOG_ALL(tilingtexturelog) << "  in tiles: " << tiles << std::endl;

	// convert to tile array indices
	tiles -= _topLeftTile;
	if (tiles.minX < 0) tiles += util::point<int>((int)TilesX, 0);
	if (tiles.minY < 0) tiles += util::point<int>(0, (int)TilesY);

	LOG_ALL(tilingtexturelog) << "  as array indices " << tiles << std::endl;

	return tiles;
}

util::rect<int>
TilingTexture::getTileRegion(const util::point<int>& tile) {

	// get tile in content space
	util::point<int> tileInContent = tile +  _topLeftTile;
	if (tileInContent.x >= (int)TilesX) tileInContent.x -= TilesX;
	if (tileInContent.y >= (int)TilesY) tileInContent.y -= TilesY;

	return util::rect<int>(0, 0, TileSize, TileSize) + _tilesRegion.upperLeft() + tileInContent*util::point<int>(TileSize, TileSize);
}

bool
TilingTexture::getNextCleanUpRequest(CleanUpRequest& request) {

	if (_cleanUpRequests.size() == 0)
		return false;

	request = _cleanUpRequests.front();
	_cleanUpRequests.pop_front();

	return true;
}

bool
TilingTexture::isDirty(const util::rect<int>& roi) {

	// is it in _cleanUpRequests?
	for (std::deque<CleanUpRequest>::const_iterator i = _cleanUpRequests.begin(); i != _cleanUpRequests.end(); i++)
		if (i->tileRegion.intersects(roi)) {

			LOG_ALL(tilingtexturelog) << "roi " << roi << " is dirty because of request in " << i->tileRegion<< std::endl;
			return true;
		}

	return false;
}
