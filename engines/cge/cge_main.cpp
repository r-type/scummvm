/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This code is based on original Soltys source code
 * Copyright (c) 1994-1995 Janus B. Wisniewski and L.K. Avalon
 */

#include "common/scummsys.h"
#include "common/endian.h"
#include "common/memstream.h"
#include "common/savefile.h"
#include "common/serializer.h"
#include "common/str.h"
#include "graphics/palette.h"
#include "graphics/scaler.h"
#include "graphics/thumbnail.h"
#include "cge/general.h"
#include "cge/sound.h"
#include "cge/vga13h.h"
#include "cge/snail.h"
#include "cge/text.h"
#include "cge/game.h"
#include "cge/events.h"
#include "cge/talk.h"
#include "cge/vmenu.h"
#include "cge/cge_main.h"
#include "cge/cge.h"
#include "cge/walk.h"
#include "cge/sound.h"

namespace CGE {

uint16  _stklen = (kStackSize * 2);

Vga *_vga;
System *_sys;
Sprite *_pocLight;
EventManager *_eventManager;
Keyboard *_keyboard;
Mouse *_mouse;
Sprite *_pocket[kPocketNX];
Sprite *_sprite;
Sprite *_miniCave;
Sprite *_shadow;
HorizLine *_horzLine;
InfoLine *_infoLine;
Sprite *_cavLight;
InfoLine *_debugLine;

Snail *_snail;
Snail *_snail_;

Fx *_fx;
Sound *_sound;
ResourceManager *_resman;

// 0.75 - 17II95  - full sound support
// 0.76 - 18II95  - small MiniEMS in DEMO,
//		    unhide CavLight in SNLEVEL
//		    keyclick suppress in startup
//		    keyclick on key service in: SYSTEM, GET_TEXT
// 1.01 - 17VII95 - default savegame with sound ON
//		    coditionals EVA for 2-month evaluation version

const char *savegameStr = "SCUMMVM_CGE";

//--------------------------------------------------------------------------

const Dac g_stdPal[] =  {// R    G   B
	{   0,  60,  0},    // 198
	{   0, 104,  0},    // 199
	{  20, 172,  0},    // 200
	{  82,  82,  0},    // 201
	{   0, 132, 82},    // 202
	{ 132, 173, 82},    // 203
	{  82,   0,  0},    // 204
	{ 206,   0, 24},    // 205
	{ 255,  33, 33},    // 206
	{ 123,  41,  0},    // 207
	{   0,  41,  0},    // 208
	{   0,   0, 82},    // 209
	{ 132,   0,  0},    // 210
	{ 255,   0,  0},    // 211
	{ 255,  66, 66},    // 212
	{ 148,  66, 16},    // 213
	{   0,  82,  0},    // 214
	{   0,   0, 132},   // 215
	{ 173,   0,  0},    // 216
	{ 255,  49,  0},    // 217
	{ 255,  99, 99},    // 218
	{ 181, 107, 49},    // 219
	{   0, 132,  0},    // 220
	{   0,   0, 255},   // 221
	{ 173,  41,  0},    // 222
	{ 255,  82,  0},    // 223
	{ 255, 132, 132},   // 224
	{ 214, 148, 74},    // 225
	{  41, 214,  0},    // 226
	{   0,  82, 173},   // 227
	{ 255, 214,  0},    // 228
	{ 247, 132, 49},    // 229
	{ 255, 165, 165},   // 230
	{ 239, 198, 123},   // 231
	{ 173, 214,  0},    // 232
	{   0, 132, 214},   // 233
	{  57,  57, 57},    // 234
	{ 247, 189, 74},    // 235
	{ 255, 198, 198},   // 236
	{ 255, 239, 173},   // 237
	{ 214, 255, 173},   // 238
	{  82, 173, 255},   // 239
	{ 107, 107, 107},   // 240
	{ 247, 222, 99},    // 241
	{ 255,   0, 255},   // 242
	{ 255, 132, 255},   // 243
	{ 132, 132, 173},   // 244
	{ 148, 247, 255},   // 245
	{ 148, 148, 148},   // 246
	{  82,   0, 82},    // 247
	{ 112,  68, 112},   // 248
	{ 176,  88, 144},   // 249
	{ 214, 132, 173},   // 250
	{ 206, 247, 255},   // 251
	{ 198, 198, 198},   // 252
	{   0, 214, 255},   // 253
	{  96, 224, 96 },   // 254
	{ 255, 255, 255},   // 255
};

void CGEEngine::syncHeader(Common::Serializer &s) {
	debugC(1, kCGEDebugEngine, "CGEEngine::syncHeader(s)");

	int i;

	s.syncAsUint16LE(_now);
	s.syncAsUint16LE(_oldLev);
	s.syncAsUint16LE(_demoText);
	for (i = 0; i < 5; i++)
		s.syncAsUint16LE(_game);
	s.syncAsSint16LE(i);		// unused VGA::Mono variable
	s.syncAsUint16LE(_music);
	s.syncBytes(_volume, 2);
	for (i = 0; i < 4; i++)
		s.syncAsUint16LE(_flag[i]);

	if (s.isLoading()) {
		// Reset cave values
		initCaveValues();
	}

	for (i = 0; i < kCaveMax; i++) {
		s.syncAsSint16LE(_heroXY[i].x);
		s.syncAsUint16LE(_heroXY[i].y);
	}
	for (i = 0; i < 1 + kCaveMax; i++) {
		s.syncAsByte(_barriers[i]._horz);
		s.syncAsByte(_barriers[i]._vert);
	}
	for (i = 0; i < kPocketNX; i++)
		s.syncAsUint16LE(_pocref[i]);

	if (s.isSaving()) {
		// Write checksum
		int checksum = kSavegameCheckSum;
		s.syncAsUint16LE(checksum);
	} else {
		// Read checksum and validate it
		uint16 checksum;
		s.syncAsUint16LE(checksum);
		if (checksum != kSavegameCheckSum)
			error("%s", _text->getText(kBadSVG));
	}
}

bool CGEEngine::loadGame(int slotNumber, SavegameHeader *header, bool tiny) {
	debugC(1, kCGEDebugEngine, "CGEEngine::loadgame(%d, header, %s)", slotNumber, tiny ? "true" : "false");

	Common::MemoryReadStream *readStream;
	SavegameHeader saveHeader;

	if (slotNumber == -1) {
		// Loading the data for the initial game state
		kSavegame0File file = kSavegame0File(kSavegame0Name);
		int size = file.size();
		byte *dataBuffer = (byte *)malloc(size);
		file.read(dataBuffer, size);
		readStream = new Common::MemoryReadStream(dataBuffer, size, DisposeAfterUse::YES);

	} else {
		// Open up the savgame file
		Common::String slotName = generateSaveName(slotNumber);
		Common::InSaveFile *saveFile = g_system->getSavefileManager()->openForLoading(slotName);

		// Read the data into a data buffer
		int size = saveFile->size();
		byte *dataBuffer = (byte *)malloc(size);
		saveFile->read(dataBuffer, size);
		readStream = new Common::MemoryReadStream(dataBuffer, size, DisposeAfterUse::YES);
	}

	// Check to see if it's a ScummVM savegame or not
	char buffer[kSavegameStrSize + 1];
	readStream->read(buffer, kSavegameStrSize + 1);

	if (strncmp(buffer, savegameStr, kSavegameStrSize + 1) != 0) {
		// It's not, so rewind back to the start
		readStream->seek(0);

		if (header)
			// Header wanted where none exists, so return false
			return false;
	} else {
		// Found header
		if (!readSavegameHeader(readStream, saveHeader)) {
			delete readStream;
			return false;
		}

		if (header) {
			*header = saveHeader;
			delete readStream;
			return true;
		}

		// Delete the thumbnail
		saveHeader.thumbnail->free();
		delete saveHeader.thumbnail;
	}

	// Get in the savegame
	syncGame(readStream, NULL, tiny);

	delete readStream;
	return true;
}

/**
 * Returns true if a given savegame exists
 */
bool CGEEngine::savegameExists(int slotNumber) {
	Common::String slotName = generateSaveName(slotNumber);

	Common::InSaveFile *saveFile = g_system->getSavefileManager()->openForLoading(slotName);
	bool result = saveFile != NULL;
	delete saveFile;
	return result;
}

/**
 * Support method that generates a savegame name
 * @param slot		Slot number
 */
Common::String CGEEngine::generateSaveName(int slot) {
	return Common::String::format("%s.%03d", _targetName.c_str(), slot);
}

Common::Error CGEEngine::loadGameState(int slot) {
	// Clear current game activity
	caveDown();
	resetGame();

	// Load the game
	loadGame(slot, NULL);
	_snail->addCom(kSnLevel, -1, _oldLev, &_cavLight);
	_cavLight->gotoxy(kCaveX + ((_now - 1) % kCaveNx) * kCaveDx + kCaveSX,
	                  kCaveY + ((_now - 1) / kCaveNx) * kCaveDy + kCaveSY);
	caveUp();

	return Common::kNoError;
}

void CGEEngine::resetGame() {
	_vga->_spareQ->clear();
}

Common::Error CGEEngine::saveGameState(int slot, const Common::String &desc) {
	caveDown();
	_oldLev = _lev;
	saveSound();

	// Write out the user's progress
	saveGame(slot, desc);

	// Reload the scene
	caveUp();

	return Common::kNoError;
}

void CGEEngine::saveSound() {
	warning("STUB: CGEEngine::saveSound");
}

void CGEEngine::saveGame(int slotNumber, const Common::String &desc) {
	// Set up the serializer
	Common::String slotName = generateSaveName(slotNumber);
	Common::OutSaveFile *saveFile = g_system->getSavefileManager()->openForSaving(slotName);

	// Write out the ScummVM savegame header
	SavegameHeader header;
	header.saveName = desc;
	header.version = kSavegameVersion;
	writeSavegameHeader(saveFile, header);

	// Write out the data of the savegame
	syncGame(NULL, saveFile, false);

	// Finish writing out game data
	saveFile->finalize();
	delete saveFile;
}

void CGEEngine::writeSavegameHeader(Common::OutSaveFile *out, SavegameHeader &header) {
	// Write out a savegame header
	out->write(savegameStr, kSavegameStrSize + 1);

	out->writeByte(kSavegameVersion);

	// Write savegame name
	out->write(header.saveName.c_str(), header.saveName.size() + 1);

	// Get the active palette
	uint8 thumbPalette[256 * 3];
	g_system->getPaletteManager()->grabPalette(thumbPalette, 0, 256);

	// Create a thumbnail and save it
	Graphics::Surface *thumb = new Graphics::Surface();
	Graphics::Surface *s = _vga->_page[0];
	::createThumbnail(thumb, (const byte *)s->pixels, kScrWidth, kScrHeight, thumbPalette);
	Graphics::saveThumbnail(*out, *thumb);
	thumb->free();
	delete thumb;

	// Write out the save date/time
	TimeDate td;
	g_system->getTimeAndDate(td);
	out->writeSint16LE(td.tm_year + 1900);
	out->writeSint16LE(td.tm_mon + 1);
	out->writeSint16LE(td.tm_mday);
	out->writeSint16LE(td.tm_hour);
	out->writeSint16LE(td.tm_min);
}

void CGEEngine::syncGame(Common::SeekableReadStream *readStream, Common::WriteStream *writeStream, bool tiny) {
	Common::Serializer s(readStream, writeStream);

	if (s.isSaving()) {
		for (int i = 0; i < kPocketNX; i++) {
			register Sprite *pocSpr = _pocket[i];
			_pocref[i] = (pocSpr) ? pocSpr->_ref : -1;
		}

		// Skip Digital and Midi volumes, useless under ScummVM
		_volume[0] = 0;
		_volume[1] = 0;
	}

	// Synchronise header data
	syncHeader(s);

	if (s.isSaving()) {
		// Loop through saving the sprite data
		for (Sprite *spr = _vga->_spareQ->first(); spr; spr = spr->_next) {
			if (!s.err())
				spr->sync(s);
		}
	} else {
		// Loading game
		if (_soundOk == 1 && _mode == 0) {
			// Skip Digital and Midi volumes, useless under ScummVM
			sndSetVolume();
		}

		if (!tiny) { // load sprites & pocket
			while (readStream->pos() < readStream->size()) {
				Sprite S(this, NULL);
				S.sync(s);

				S._prev = S._next = NULL;
				Sprite *spr = (scumm_stricmp(S._file + 2, "MUCHA") == 0) ? new Fly(this, NULL)
					  : new Sprite(this, NULL);
				assert(spr != NULL);
				*spr = S;
				_vga->_spareQ->append(spr);
			}

			for (int i = 0; i < kPocketNX; i++) {
				register int r = _pocref[i];
				_pocket[i] = (r < 0) ? NULL : _vga->_spareQ->locate(r);
			}
		}
	}
}

bool CGEEngine::readSavegameHeader(Common::InSaveFile *in, SavegameHeader &header) {
	header.thumbnail = NULL;

	// Get the savegame version
	header.version = in->readByte();
	if (header.version > kSavegameVersion)
		return false;

	// Read in the string
	header.saveName.clear();
	char ch;
	while ((ch = (char)in->readByte()) != '\0')
		header.saveName += ch;

	// Get the thumbnail
	header.thumbnail = Graphics::loadThumbnail(*in);
	if (!header.thumbnail) {
		delete header.thumbnail;
		header.thumbnail = NULL;
		return false;
	}

	// Read in save date/time
	header.saveYear = in->readSint16LE();
	header.saveMonth = in->readSint16LE();
	header.saveDay = in->readSint16LE();
	header.saveHour = in->readSint16LE();
	header.saveMinutes = in->readSint16LE();

	return true;
}

void CGEEngine::heroCover(int cvr) {
	debugC(1, kCGEDebugEngine, "CGEEngine::heroCover(%d)", cvr);

	_snail->addCom(kSnCover, 1, cvr, NULL);
}

void CGEEngine::trouble(int seq, int text) {
	debugC(1, kCGEDebugEngine, "CGEEngine::trouble(%d, %d)", seq, text);

	_hero->park();
	_snail->addCom(kSnWait, -1, -1, _hero);
	_snail->addCom(kSnSeq, -1, seq, _hero);
	_snail->addCom(kSnSound, -1, 2, _hero);
	_snail->addCom(kSnWait, -1, -1, _hero);
	_snail->addCom(kSnSay,  1, text, _hero);
}

void CGEEngine::offUse() {
	debugC(1, kCGEDebugEngine, "CGEEngine::offUse()");

	trouble(kSeqOffUse, kOffUse + newRandom(_offUseCount));
}

void CGEEngine::tooFar() {
	debugC(1, kCGEDebugEngine, "CGEEngine::tooFar()");

	trouble(kSeqTooFar, kTooFar);
}

void CGEEngine::loadHeroXY() {
	debugC(1, kCGEDebugEngine, "CGEEngine::loadHeroXY()");

	EncryptedStream cf("CGE.HXY");
	uint16 x, y;

	memset(_heroXY, 0, sizeof(_heroXY));
	if (!cf.err()) {
		for (int i = 0; i < kCaveMax; ++i) {
			cf.read((byte *)&x, 2);
			cf.read((byte *)&y, 2);

			_heroXY[i].x = (int16)FROM_LE_16(x);
			_heroXY[i].y = (int16)FROM_LE_16(y);
		}
	}
}

void CGEEngine::loadMapping() {
	debugC(1, kCGEDebugEngine, "CGEEngine::loadMapping()");

	if (_now <= kCaveMax) {
		EncryptedStream cf("CGE.TAB");
		if (!cf.err()) {
			// Move to the data for the given room
			cf.seek((_now - 1) * kMapArrSize);
			
			// Read in the data
			for (int z = 0; z < kMapZCnt; ++z) {
				cf.read(&Cluster::_map[z][0], kMapXCnt);
			}
		}
	}
}

Square::Square(CGEEngine *vm) : Sprite(vm, NULL), _vm(vm) {
	_flags._kill = true;
	_flags._bDel = false;

	BitmapPtr *MB = new BitmapPtr[2];
	MB[0] = new Bitmap("BRICK");
	MB[1] = NULL;
	setShapeList(MB);
}

void Square::touch(uint16 mask, int x, int y) {
	Sprite::touch(mask, x, y);
	if (mask & kMouseLeftUp) {
		XZ(_x + x, _y + y).cell() = 0;
		_snail_->addCom(kSnKill, -1, 0, this);
	}
}

void CGEEngine::setMapBrick(int x, int z) {
	debugC(1, kCGEDebugEngine, "CGEEngine::setMapBrick(%d, %d)", x, z);

	Square *s = new Square(this);
	if (s) {
		char n[6];
		s->gotoxy(x * kMapGridX, kMapTop + z * kMapGridZ);
		sprintf(n, "%02d:%02d", x, z);
		Cluster::_map[z][x] = 1;
		s->setName(n);
		_vga->_showQ->insert(s, _vga->_showQ->first());
	}
}

void CGEEngine::keyClick() {
	debugC(1, kCGEDebugEngine, "CGEEngine::keyClick()");

	_snail_->addCom(kSnSound, -1, 5, NULL);
}

void CGEEngine::resetQSwitch() {
	debugC(1, kCGEDebugEngine, "CGEEngine::resetQSwitch()");

	_snail_->addCom(kSnSeq, 123,  0, NULL);
	keyClick();
}

void CGEEngine::quit() {
	debugC(1, kCGEDebugEngine, "CGEEngine::quit()");

	static Choice QuitMenu[] = {
		{ NULL, &CGEEngine::startCountDown },
		{ NULL, &CGEEngine::resetQSwitch   },
		{ NULL, &CGEEngine::dummy          }
	};

	if (_snail->idle() && !_hero->_flags._hide) {
		if (Vmenu::_addr) {
			_snail_->addCom(kSnKill, -1, 0, Vmenu::_addr);
			resetQSwitch();
		} else {
			QuitMenu[0]._text = _text->getText(kQuit);
			QuitMenu[1]._text = _text->getText(kNoQuit);
			(new Vmenu(this, QuitMenu, -1, -1))->setName(_text->getText(kQuitTitle));
			_snail_->addCom(kSnSeq, 123, 1, NULL);
			keyClick();
		}
	}
}

void CGEEngine::miniStep(int stp) {
	debugC(1, kCGEDebugEngine, "CGEEngine::miniStep(%d)", stp);

	if (stp < 0) {
		_miniCave->_flags._hide = true;
	} else {
		*_miniShp[0] = *_miniShpList[stp];
		_miniCave->_flags._hide = false;
	}
}

void CGEEngine::postMiniStep(int step) {
	debugC(6, kCGEDebugEngine, "CGEEngine::postMiniStep(%d)", step);

	if (_miniCave && step != _recentStep)
		_snail_->addCom2(kSnExec, -1, _recentStep = step, kMiniStep);
}

void CGEEngine::showBak(int ref) {
	debugC(1, kCGEDebugEngine, "CGEEngine::showBack(%d)", ref);

	Sprite *spr = _vga->_spareQ->locate(ref);
	if (!spr)
		return;

	Bitmap::_pal = _vga->_sysPal;
	spr->expand();
	Bitmap::_pal = NULL;
	spr->show(2);
	_vga->copyPage(1, 2);
	_sys->setPal();
	spr->contract();
}

void CGEEngine::caveUp() {
	debugC(1, kCGEDebugEngine, "CGEEngine::caveUp()");

	const int BakRef = 1000 * _now;
	if (_music)
		_midiPlayer.loadMidi(_now);

	showBak(BakRef);
	loadMapping();
	Sprite *spr = _vga->_spareQ->first();
	while (spr) {
		Sprite *n = spr->_next;
		if (spr->_cave == _now || spr->_cave == 0)
			if (spr->_ref != BakRef) {
				if (spr->_flags._back)
					spr->backShow();
				else
					expandSprite(spr);
			}
		spr = n;
	}

	_sound->stop();
	_fx->clear();
	_fx->preload(0);
	_fx->preload(BakRef);

	if (_hero) {
		_hero->gotoxy(_heroXY[_now - 1].x, _heroXY[_now - 1].y);
		// following 2 lines trims Hero's Z position!
		_hero->tick();
		_hero->_time = 1;
		_hero->_flags._hide = false;
	}

	if (!_dark)
		_vga->sunset();

	_vga->copyPage(0, 1);
	selectPocket(-1);
	if (_hero)
		_vga->_showQ->insert(_vga->_showQ->remove(_hero));

	if (_shadow) {
		_vga->_showQ->remove(_shadow);
		_shadow->makeXlat(glass(_vga->_sysPal, 204, 204, 204));
		_vga->_showQ->insert(_shadow, _hero);
		_shadow->_z = _hero->_z;
	}
	feedSnail(_vga->_showQ->locate(BakRef + 999), kTake);
	_vga->show();
	_vga->copyPage(1, 0);
	_vga->show();
	_vga->sunrise(_vga->_sysPal);
	_dark = false;
	if (!_startupMode)
		_mouse->on();
}

void CGEEngine::caveDown() {
	debugC(1, kCGEDebugEngine, "CGEEngine::caveDown()");

	if (_horzLine && !_horzLine->_flags._hide)
		switchMapping();

	for (Sprite *spr = _vga->_showQ->first(); spr;) {
		Sprite *n = spr->_next;
		if (spr->_ref >= 1000 /*&& spr->_cave*/) {
			if (spr->_ref % 1000 == 999)
				feedSnail(spr, kTake);
			_vga->_spareQ->append(_vga->_showQ->remove(spr));
		}
		spr = n;
	}
}

void CGEEngine::xCave() {
	debugC(6, kCGEDebugEngine, "CGEEngine::xCave()");

	caveDown();
	caveUp();
}

void CGEEngine::qGame() {
	debugC(1, kCGEDebugEngine, "CGEEngine::qGame()");

	caveDown();
	_oldLev = _lev;
	saveSound();

	// Write out the user's progress
	saveGame(0, Common::String("Automatic Savegame"));

	_vga->sunset();
	_finis = true;
}

void CGEEngine::switchCave(int cav) {
	debugC(1, kCGEDebugEngine, "CGEEngine::switchCave(%d)", cav);

	if (cav == _now)
		return;

	if (cav < 0) {
		_snail->addCom(kSnLabel, -1, 0, NULL);  // wait for repaint
		_snail->addCom2(kSnExec,  -1, 0, kQGame); // switch cave
	} else {
		_now = cav;
		_mouse->off();
		if (_hero) {
			_hero->park();
			_hero->step(0);
			_vga->_spareQ->_show = 0;
		}
		_cavLight->gotoxy(kCaveX + ((_now - 1) % kCaveNx) * kCaveDx + kCaveSX,
		                  kCaveY + ((_now - 1) / kCaveNx) * kCaveDy + kCaveSY);
		killText();
		if (!_startupMode)
			keyClick();
		_snail->addCom(kSnLabel, -1, 0, NULL);  // wait for repaint
		_snail->addCom2(kSnExec,   0, 0, kXCave); // switch cave
	}
}

System::System(CGEEngine *vm) : Sprite(vm, NULL), _vm(vm) {
	_funDel = kHeroFun0;
	setPal();
	tick();
}

void System::setPal() {
	Dac *p = _vga->_sysPal + 256 - ARRAYSIZE(g_stdPal);
	for (uint i = 0; i < ARRAYSIZE(g_stdPal); i++) {
		p[i]._r = g_stdPal[i]._r >> 2;
		p[i]._g = g_stdPal[i]._g >> 2;
		p[i]._b = g_stdPal[i]._b >> 2;
	}
}

void System::funTouch() {
	uint16 n = (_vm->_flag[0]) ? kHeroFun1 : kHeroFun0; // PAIN flag
	if (_talk == NULL || n > _funDel)
		_funDel = n;
}

void System::touch(uint16 mask, int x, int y) {
	funTouch();

	if (mask & kEventKeyb) {
		_vm->keyClick();
		killText();
		if (_vm->_startupMode == 1) {
			_snail->addCom(kSnClear, -1, 0, NULL);
			return;
		}
		switch (x) {
		case 'X':
			if (_keyboard->_key[kKeyAlt])
				_vm->quit();
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
			if (_keyboard->_key[kKeyAlt]) {
				_snail->addCom(kSnLevel, -1, x - '0', NULL);
				break;
			}
			break;
		}
	} else {
		if (_vm->_startupMode)
			return;
		int cav = 0;
		_infoLine->update(NULL);
		if (y >= kWorldHeight ) {
			if (x < kButtonX) {                           // select cave?
				if (y >= kCaveY && y < kCaveY + kCaveNy * kCaveDy &&
				    x >= kCaveX && x < kCaveX + kCaveNx * kCaveDx && !_vm->_game) {
					cav = ((y - kCaveY) / kCaveDy) * kCaveNx + (x - kCaveX) / kCaveDx + 1;
					if (cav > _vm->_maxCave)
						cav = 0;
				} else {
					cav = 0;
				}
			} else if (mask & kMouseLeftUp) {
				if (y >= kPocketY && y < kPocketY + kPocketNY * kPocketDY &&
				    x >= kPocketX && x < kPocketX + kPocketNX * kPocketDX) {
					int n = ((y - kPocketY) / kPocketDY) * kPocketNX + (x - kPocketX) / kPocketDX;
					_vm->selectPocket(n);
				}
			}
		}

		_vm->postMiniStep(cav - 1);

		if (mask & kMouseLeftUp) {
			if (cav && _snail->idle() && _hero->_tracePtr < 0)
				_vm->switchCave(cav);

			if (_horzLine && !_horzLine->_flags._hide) {
				if (y >= kMapTop && y < kMapTop + kMapHig) {
					Cluster tmpCluster = XZ(x, y);
					int16 x1 = tmpCluster._pt.x;
					int16 z1 = tmpCluster._pt.y;
					Cluster::_map[z1][x1] = 1;
					_vm->setMapBrick(x1, z1);
				}
			} else {
				if (!_talk && _snail->idle() && _hero
				        && y >= kMapTop && y < kMapTop + kMapHig && !_vm->_game) {
					_hero->findWay(XZ(x, y));
				}
			}
		}
	}
}

void System::tick() {
	if (!_vm->_startupMode)
		if (--_funDel == 0) {
			killText();
			if (_snail->idle()) {
				if (_vm->_flag[0]) // Pain flag
					_vm->heroCover(9);
				else { // CHECKME: Before, was: if (Startup::_core >= CORE_MID) {
					int n = newRandom(100);
					if (n > 96)
						_vm->heroCover(6 + (_hero->_x + _hero->_w / 2 < kScrWidth / 2));
					else if (n > 90)
						_vm->heroCover(5);
					else if (n > 60)
						_vm->heroCover(4);
					else
						_vm->heroCover(3);
				}
			}
			funTouch();
		}
	_time = kSystemRate;
}

void CGEEngine::switchColorMode() {
	debugC(1, kCGEDebugEngine, "CGEEngine::switchColorMode()");

	_snail_->addCom(kSnSeq, 121, _vga->_mono = !_vga->_mono, NULL);
	keyClick();
	_vga->setColors(_vga->_sysPal, 64);
}

void CGEEngine::switchMusic() {
	debugC(1, kCGEDebugEngine, "CGEEngine::switchMusic()");

	_snail_->addCom(kSnSeq, 122, (_music = !_music), NULL);
	keyClick();

	if (_music)
		_midiPlayer.loadMidi(_now);
	else
		_midiPlayer.killMidi();
}

void CGEEngine::startCountDown() {
	debugC(1, kCGEDebugEngine, "CGEEngine::startCountDown()");

	//SNPOST(SNSEQ, 123, 0, NULL);
	switchCave(-1);
}

void CGEEngine::switchMapping() {
	assert(_horzLine);
	debugC(1, kCGEDebugEngine, "CGEEngine::switchMapping()");

	if (_horzLine && _horzLine->_flags._hide) {
		for (int i = 0; i < kMapZCnt; i++) {
			for (int j = 0; j < kMapXCnt; j++) {
				if (Cluster::_map[i][j])
					setMapBrick(j, i);
			}
		}
	} else {
		for (Sprite *s = _vga->_showQ->first(); s; s = s->_next)
			if (s->_w == kMapGridX && s->_h == kMapGridZ)
				_snail_->addCom(kSnKill, -1, 0, s);
	}
	_horzLine->_flags._hide = !_horzLine->_flags._hide;
}

void CGEEngine::killSprite() {
	debugC(1, kCGEDebugEngine, "CGEEngine::killSprite()");

	_sprite->_flags._kill = true;
	_sprite->_flags._bDel = true;
	_snail_->addCom(kSnKill, -1, 0, _sprite);
	_sprite = NULL;
}

void CGEEngine::optionTouch(int opt, uint16 mask) {
	switch (opt) {
	case 1:
		if (mask & kMouseLeftUp)
			switchColorMode();
		break;
	case 2:
		if (mask & kMouseLeftUp)
			switchMusic();
		else if (mask & kMouseRightUp)
			warning("TODO: Use ScummVM sound dialog");
		break;
	case 3:
		if (mask & kMouseLeftUp)
			quit();
		break;
	}
}

#pragma argsused
void Sprite::touch(uint16 mask, int x, int y) {
	_sys->funTouch();

	if ((mask & kEventAttn) != 0)
		return;

	_infoLine->update(name());

	if (mask & (kMouseRightDown | kMouseLeftDown))
		_sprite = this;

	if (_ref / 10 == 12) {
		_vm->optionTouch(_ref % 10, mask);
		return;
	}

	if (_flags._syst)
		return;       // cannot access system sprites

	if (_vm->_game)
		if (mask & kMouseLeftUp) {
			mask &= ~kMouseLeftUp;
			mask |= kMouseRightUp;
		}

	if ((mask & kMouseRightUp) && _snail->idle()) {
		Sprite *ps = (_pocLight->_seqPtr) ? _pocket[_vm->_pocPtr] : NULL;
		if (ps) {
			if (_flags._kept || _hero->distance(this) < kDistMax) {
				if (works(ps)) {
					_vm->feedSnail(ps, kTake);
				} else
					_vm->offUse();
				_vm->selectPocket(-1);
			} else
				_vm->tooFar();
		} else {
			if (_flags._kept) {
				mask |= kMouseLeftUp;
			} else {
				if (_hero->distance(this) < kDistMax) {
					if (_flags._port) {
						if (_vm->findPocket(NULL) < 0) {
							_vm->pocFul();
						} else {
							_snail->addCom(kSnReach, -1, -1, this);
							_snail->addCom(kSnKeep, -1, -1, this);
							_flags._port = false;
						}
					} else {
						if (_takePtr != kNoPtr) {
							if (snList(kTake)[_takePtr]._com == kSnNext)
								_vm->offUse();
							else
								_vm->feedSnail(this, kTake);
						} else {
							_vm->offUse();
						}
					}
				} else {
					_vm->tooFar();
				}
			}
		}
	}

	if ((mask & kMouseLeftUp) && _snail->idle()) {
		if (_flags._kept) {
			for (int n = 0; n < kPocketNX; n++) {
				if (_pocket[n] == this) {
					_vm->selectPocket(n);
					break;
				}
			}
		} else {
			_snail->addCom(kSnWalk, -1, -1, this); // Hero->FindWay(this);
		}
	}
}

void CGEEngine::loadSprite(const char *fname, int ref, int cav, int col = 0, int row = 0, int pos = 0) {
	static const char *Comd[] = { "Name", "Type", "Phase", "East",
	                              "Left", "Right", "Top", "Bottom",
	                              "Seq", "Near", "Take",
	                              "Portable", "Transparent",
	                              NULL
	                            };
	static const char *Type[] = { "DEAD", "AUTO", "WALK", "NEWTON", "LISSAJOUS",
	                              "FLY", NULL
	                            };

	int shpcnt = 0;
	int type = 0; // DEAD
	bool east = false;
	bool port = false;
	bool tran = false;
	int i, lcnt = 0;

	char tmpStr[kLineMax + 1];
	Common::String line;
	mergeExt(tmpStr, fname, kSprExt);

	if (_resman->exist(tmpStr)) {      // sprite description file exist
		EncryptedStream sprf(tmpStr);
		if (sprf.err())
			error("Bad SPR [%s]", tmpStr);

		uint16 len;
		for (line = sprf.readLine(); !sprf.eos(); line = sprf.readLine()) {
			len = line.size();			
			lcnt++;
			strcpy(tmpStr, line.c_str());
			if (len == 0 || *tmpStr == '.')
				continue;

			if ((i = takeEnum(Comd, strtok(tmpStr, " =\t"))) < 0)
				error("Bad line %d [%s]", lcnt, fname);


			switch (i) {
			case  0 : // Name - will be taken in Expand routine
				break;
			case  1 : // Type
				if ((type = takeEnum(Type, strtok(NULL, " \t,;/"))) < 0)
					error("Bad line %d [%s]", lcnt, fname);
				break;
			case  2 : // Phase
				shpcnt++;
				break;
			case  3 : // East
				east = (atoi(strtok(NULL, " \t,;/")) != 0);
				break;
			case 11 : // Portable
				port = (atoi(strtok(NULL, " \t,;/")) != 0);
				break;
			case 12 : // Transparent
				tran = (atoi(strtok(NULL, " \t,;/")) != 0);
				break;
			}
		}
		if (! shpcnt)
			error("No shapes [%s]", fname);
	} else {
		// no sprite description: mono-shaped sprite with only .BMP file
		++shpcnt;
	}

	// make sprite of choosen type
	switch (type) {
	case 1:
		// AUTO
		_sprite = new Sprite(this, NULL);
		if (_sprite) {
			_sprite->gotoxy(col, row);
		}
		break;
	case 2:
		{ // WALK
		Walk *w = new Walk(this, NULL);
		if (w && ref == 1) {
			w->gotoxy(col, row);
			if (_hero)
				error("2nd HERO [%s]", fname);
			_hero = w;
		}
		_sprite = w;
		break;
		}
	case 3:  // NEWTON
	case 4:  // LISSAJOUS
		error("Bad type [%s]", fname);
		break;
	case 5:
		{ // FLY
		Fly *f = new Fly(this, NULL);
		_sprite = f;
		break;
		}
	default:
		// DEAD
		_sprite = new Sprite(this, NULL);
		if (_sprite)
			_sprite->gotoxy(col, row);
		break;
	}

	if (_sprite) {
		_sprite->_ref = ref;
		_sprite->_cave = cav;
		_sprite->_z = pos;
		_sprite->_flags._east = east;
		_sprite->_flags._port = port;
		_sprite->_flags._tran = tran;
		_sprite->_flags._kill = true;
		_sprite->_flags._bDel = true;

		// Extract the filename, without the extension
		strcpy(_sprite->_file, fname);
		char *p = strchr(_sprite->_file, '.');
		if (p)
			*p = '\0';

		_sprite->_shpCnt = shpcnt;
		_vga->_spareQ->append(_sprite);
	}
}

void CGEEngine::loadScript(const char *fname) {
	EncryptedStream scrf(fname);

	if (scrf.err())
		return;

	bool ok = true;
	int lcnt = 0;

	char tmpStr[kLineMax+1];
	Common::String line;

	for (line = scrf.readLine(); !scrf.eos(); line = scrf.readLine()) {
		char *p;

		lcnt++;
		strcpy(tmpStr, line.c_str());
		if ((line.size() == 0) || (*tmpStr == '.'))
			continue;

		ok = false;   // not OK if break

		// sprite ident number
		if ((p = strtok(tmpStr, " \t\n")) == NULL)
			break;
		int SpI = atoi(p);

		// sprite file name
		char *SpN;
		if ((SpN = strtok(NULL, " ,;/\t\n")) == NULL)
			break;

		// sprite cave
		if ((p = strtok(NULL, " ,;/\t\n")) == NULL)
			break;
		int SpA = atoi(p);

		// sprite column
		if ((p = strtok(NULL, " ,;/\t\n")) == NULL)
			break;
		int SpX = atoi(p);

		// sprite row
		if ((p = strtok(NULL, " ,;/\t\n")) == NULL)
			break;
		int SpY = atoi(p);

		// sprite Z pos
		if ((p = strtok(NULL, " ,;/\t\n")) == NULL)
			break;
		int SpZ = atoi(p);

		// sprite life
		if ((p = strtok(NULL, " ,;/\t\n")) == NULL)
			break;
		bool BkG = atoi(p) == 0;

		ok = true;    // no break: OK

		_sprite = NULL;
		loadSprite(SpN, SpI, SpA, SpX, SpY, SpZ);
		if (_sprite && BkG)
			_sprite->_flags._back = true;
	}

	if (!ok)
		error("Bad INI line %d [%s]", lcnt, fname);
}

void CGEEngine::mainLoop() {
	_vga->show();
	_snail_->runCom();
	_snail->runCom();

	// Handle a delay between game frames
	handleFrame();

	// Handle any pending events
	_eventManager->poll();
}

void CGEEngine::handleFrame() {
	// Game frame delay
	uint32 millis = g_system->getMillis();
	while (!_eventManager->_quitFlag && (millis < (_lastFrame + kGameFrameDelay))) {
		// Handle any pending events
		_eventManager->poll();

		if (millis >= (_lastTick + kGameTickDelay)) {
			// Dispatch the tick to any active objects
			tick();
			_lastTick = millis;
		}

		// Slight delay
		g_system->delayMillis(5);
		millis = g_system->getMillis();
	}
	_lastFrame = millis;

	if (millis >= (_lastTick + kGameTickDelay)) {
		// Dispatch the tick to any active objects
		tick();
		_lastTick = millis;
	}
}

void CGEEngine::tick() {
	for (Sprite *spr = _vga->_showQ->first(); spr; spr = spr->_next) {
		if (spr->_time) {
			if (!spr->_flags._hide) {
				if (--spr->_time == 0)
					spr->tick();
			}
		}
	}
}

void CGEEngine::loadUser() {
	// set scene
	if (_mode == 0) {
		// user .SVG file found - load it from slot 0
		loadGame(0, NULL);
	} else if (_mode == 1) {
		// Load either initial game state savegame or launcher specified savegame
		loadGame(_startGameSlot, NULL);
	} else {
		error("Creating setup savegames not supported");
	}
	loadScript("CGE.IN0");
}

void CGEEngine::runGame() {
	if (_eventManager->_quitFlag)
		return;

	loadHeroXY();

	_cavLight->_flags._tran = true;
	_vga->_showQ->append(_cavLight);
	_cavLight->_flags._hide = true;

	const Seq pocSeq[] = {
		{ 0, 0, 0, 0, 20 },
		{ 1, 2, 0, 0,  4 },
		{ 2, 3, 0, 0,  4 },
		{ 3, 4, 0, 0, 16 },
		{ 2, 5, 0, 0,  4 },
		{ 1, 6, 0, 0,  4 },
		{ 0, 1, 0, 0, 16 },
	};
	Seq *seq = (Seq *)malloc(7 * sizeof(Seq));
	Common::copy(pocSeq, pocSeq + 7, seq);
	_pocLight->setSeq(seq);

	_pocLight->_flags._tran = true;
	_pocLight->_time = 1;
	_pocLight->_z = 120;
	_vga->_showQ->append(_pocLight);
	selectPocket(-1);

	_vga->_showQ->append(_mouse);

//    ___________
	loadUser();
//    ~~~~~~~~~~~

	if ((_sprite = _vga->_spareQ->locate(121)) != NULL)
		_snail_->addCom(kSnSeq, -1, _vga->_mono, _sprite);
	if ((_sprite = _vga->_spareQ->locate(122)) != NULL)
		_sprite->step(_music);
	_snail_->addCom(kSnSeq, -1, _music, _sprite);
	if (!_music)
		_midiPlayer.killMidi();

	if (_resman->exist("MINI.SPR")) {
		_miniShp = new BitmapPtr[2];
		_miniShp[0] = _miniShp[1] = NULL;

		loadSprite("MINI", -1, 0, kMiniX, kMiniY);
		expandSprite(_miniCave = _sprite);  // NULL is ok
		if (_miniCave) {
			_miniCave->_flags._kill = false;
			_miniCave->_flags._hide = true;
			_miniShp[0] = new Bitmap(*_miniCave->shp());
			_miniShpList = _miniCave->setShapeList(_miniShp);
			postMiniStep(-1);
		}
	}

	if (_hero) {
		expandSprite(_hero);
		_hero->gotoxy(_heroXY[_now - 1].x, _heroXY[_now - 1].y);
		if (_resman->exist("00SHADOW.SPR")) {
			loadSprite("00SHADOW", -1, 0, _hero->_x + 14, _hero->_y + 51);
			delete _shadow;
			if ((_shadow = _sprite) != NULL) {
				_shadow->_ref = 2;
				_shadow->_flags._tran = true;
				_shadow->_flags._kill = false;
				_hero->_flags._shad = true;
				_vga->_showQ->insert(_vga->_spareQ->remove(_shadow), _hero);
			}
		}
	}

	_infoLine->gotoxy(kInfoX, kInfoY);
	_infoLine->_flags._tran = true;
	_infoLine->update(NULL);
	_vga->_showQ->insert(_infoLine);

	_debugLine->_z = 126;
	_vga->_showQ->insert(_debugLine);

	if (_horzLine) {
		_horzLine->_y = kMapTop - (kMapTop > 0);
		_horzLine->_z = 126;
		_vga->_showQ->insert(_horzLine);
	}

	_mouse->_busy = _vga->_spareQ->locate(kBusyRef);
	if (_mouse->_busy)
		expandSprite(_mouse->_busy);

	_startupMode = 0;

	_snail->addCom(kSnLevel, -1, _oldLev, &_cavLight);
	_cavLight->gotoxy(kCaveX + ((_now - 1) % kCaveNx) * kCaveDx + kCaveSX,
	                  kCaveY + ((_now - 1) / kCaveNx) * kCaveDy + kCaveSY);
	caveUp();

	_keyboard->setClient(_sys);
	// main loop
	while (!_finis && !_eventManager->_quitFlag) {
		if (_flag[3])
			_snail->addCom2(kSnExec,  -1, 0, kQGame);
		mainLoop();
	}

	// If finishing game due to closing ScummVM window, explicitly save the game
	if (!_finis && canSaveGameStateCurrently())
		qGame();

	_keyboard->setClient(NULL);
	_snail->addCom(kSnClear, -1, 0, NULL);
	_snail_->addCom(kSnClear, -1, 0, NULL);
	_mouse->off();
	_vga->_showQ->clear();
	_vga->_spareQ->clear();
	_hero = NULL;
	_shadow = NULL;
}

void CGEEngine::movie(const char *ext) {
	assert(ext);

	if (_eventManager->_quitFlag)
		return;

	char fn[12];
	sprintf(fn, "CGE.%s", (*ext == '.') ? ext +1 : ext);

	if (_resman->exist(fn)) {
		loadScript(fn);
		expandSprite(_vga->_spareQ->locate(999));
		feedSnail(_vga->_showQ->locate(999), kTake);
		_vga->_showQ->append(_mouse);
		_keyboard->setClient(_sys);
		while (!_snail->idle() && !_eventManager->_quitFlag)
			mainLoop();

		_keyboard->setClient(NULL);
		_snail->addCom(kSnClear, -1, 0, NULL);
		_snail_->addCom(kSnClear, -1, 0, NULL);
		_vga->_showQ->clear();
		_vga->_spareQ->clear();
	}
}

bool CGEEngine::showTitle(const char *name) {
	if (_eventManager->_quitFlag)
		return false;

	Bitmap::_pal = _vga->_sysPal;
	BitmapPtr *LB = new BitmapPtr[2];
	LB[0] = new Bitmap(name);
	LB[1] = NULL;
	Bitmap::_pal = NULL;

	Sprite D(this, LB);
	D._flags._kill = true;
	D._flags._bDel = true;
	D.center();
	D.show(2);

	if (_mode == 2) {
		inf(kSavegame0Name);
		_talk->show(2);
	}

	_vga->sunset();
	_vga->copyPage(1, 2);
	_vga->copyPage(0, 1);
	selectPocket(-1);
	_vga->sunrise(_vga->_sysPal);

	if (_mode < 2 && !_soundOk) {
		_vga->copyPage(1, 2);
		_vga->copyPage(0, 1);
		_vga->_showQ->append(_mouse);
		_mouse->on();
		for (; !_snail->idle() || Vmenu::_addr;) {
			mainLoop();
			if (_eventManager->_quitFlag)
				return false;
		}

		_mouse->off();
		_vga->_showQ->clear();
		_vga->copyPage(0, 2);
		_soundOk = 2;
		if (_music)
			_midiPlayer.loadMidi(0);
	}

	if (_mode < 2) {
		// At this point the game originally set the protection variables
		// used by the copy protection check
		movie(kPaylistExt); // paylist
		_vga->copyPage(1, 2);
		_vga->copyPage(0, 1);
		_vga->_showQ->append(_mouse);
		// In the original game, the user had to enter his name
		// As it was only used to name savegames, it has been removed
		_vga->_showQ->clear();
		_vga->copyPage(0, 2);

		if (_mode == 0) {
// The auto-load of savegame #0 is currently disabled
#if 0
			if (savegameExists(0)) {
				// Load the savegame
				loadGame(0, NULL, true); // only system vars
				_vga->setColors(_vga->_sysPal, 64);
				_vga->update();
				if (_flag[3]) { //flag FINIS
					_mode++;
					_flag[3] = false;
				}
			} else
#endif
				_mode++;
		}
	}

	if (_mode < 2)
		movie(kWinkExt);

	_vga->copyPage(0, 2);

	return true;
}

void CGEEngine::cge_main() {
	memset(_barriers, 0xFF, sizeof(_barriers));

	if (!_mouse->_exist)
		error("%s", _text->getText(kTextNoMouse));

	if (!_resman->exist(kSavegame0Name))
		_mode = 2;

	_debugLine->_flags._hide = true;
	if (_horzLine)
		_horzLine->_flags._hide = true;

	if (_music && _soundOk)
		_midiPlayer.loadMidi(0);

	if (_startGameSlot != -1) {
		// Starting up a savegame from the launcher
		_mode++;
		runGame();

		_startupMode = 2;
		if (_flag[3]) // Flag FINIS
			movie(kEndgExt);
	} else {
		if (_mode < 2)
			movie(kLgoExt);

		if (showTitle("WELCOME")) {
			if (_mode == 1)
				movie(kIntroExt);
			runGame();
			_startupMode = 2;
			if (_flag[3]) // Flag FINIS
				movie(kEndgExt);
		} else
			_vga->sunset();
	}
}

} // End of namespace CGE
