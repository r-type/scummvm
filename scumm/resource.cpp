/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2005 The ScummVM project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "common/str.h"
#include "scumm/dialogs.h"
#include "scumm/imuse.h"
#include "scumm/imuse_digi/dimuse.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"
#include "scumm/verbs.h"
#include "sound/mididrv.h" // Need MD_ enum values

namespace Scumm {

static uint16 newTag2Old(uint32 oldTag);
static const char *resTypeFromId(int id);


/* Open a room */
void ScummEngine::openRoom(int room) {
	int room_offs, roomlimit;
	bool result;
	char buf[128];
	char buf2[128] = "";
	byte encByte = 0;

	debugC(DEBUG_GENERAL, "openRoom(%d)", room);
	assert(room >= 0);

	/* Don't load the same room again */
	if (_lastLoadedRoom == room)
		return;
	_lastLoadedRoom = room;

	/* Room -1 means close file */
	if (room == -1) {
		deleteRoomOffsets();
		_fileHandle.close();
		return;
	}

	/* Either xxx.lfl or monkey.xxx file name */
	while (1) {
		if (_features & GF_SMALL_NAMES)
			roomlimit = 98;
		else
			roomlimit = 900;
		if (_features & GF_EXTERNAL_CHARSET && room >= roomlimit)
			room_offs = 0;
		else
			room_offs = room ? res.roomoffs[rtRoom][room] : 0;

		if (room_offs == -1)
			break;

		if (room_offs != 0 && room != 0) {
			_fileOffset = res.roomoffs[rtRoom][room];
			return;
		}
		if (!(_features & GF_SMALL_HEADER)) {

			if (_heversion >= 70) { // Windows titles
				if (_heversion >= 98) {
					sprintf(buf, "%s.%s", _gameName.c_str(), room == 0 ? "he0" : "(a)");
					sprintf(buf2, "%s.(b)", _gameName.c_str());
				} else
					sprintf(buf, "%s.he%.1d", _gameName.c_str(), room == 0 ? 0 : 1);
			} else if (_version >= 7) {
				if (room > 0 && (_version == 8))
					VAR(VAR_CURRENTDISK) = res.roomno[rtRoom][room];
				sprintf(buf, "%s.la%d", _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);

				sprintf(buf2, "%s.%.3d", _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
			} else if (_features & GF_HUMONGOUS)
				sprintf(buf, "%s.he%.1d", _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
			else {
				sprintf(buf, "%s.%.3d",  _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
				if (_gameId == GID_SAMNMAX)
					sprintf(buf2, "%s.sm%.1d",  _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
			}

			encByte = (_features & GF_USE_KEY) ? 0x69 : 0;
		} else if (!(_features & GF_SMALL_NAMES)) {
			if (room == 0 || room >= 900) {
				sprintf(buf, "%.3d.lfl", room);
				encByte = 0;
				if (openResourceFile(buf, encByte)) {
					return;
				}
				askForDisk(buf, room == 0 ? 0 : res.roomno[rtRoom][room]);

			} else {
				sprintf(buf, "disk%.2d.lec", room == 0 ? 0 : res.roomno[rtRoom][room]);
				encByte = 0x69;
			}
		} else {
			sprintf(buf, "%.2d.lfl", room);
			// Maniac Mansion demo has .man instead of .lfl
			if (_gameId == GID_MANIAC)
				sprintf(buf2, "%.2d.man", room);
			encByte = (_features & GF_USE_KEY) ? 0xFF : 0;
		}

		// If we have substitute
		if (_heMacFileNameIndex > 0) {
			char tmpBuf[128];

			generateMacFileName(buf, tmpBuf, 128, 0, _heMacFileNameIndex);
			strcpy(buf, tmpBuf);
			generateMacFileName(buf2, tmpBuf, 128, 0, _heMacFileNameIndex);
			strcpy(buf2, tmpBuf);
		}

		result = openResourceFile(buf, encByte);
		if ((result == false) && (buf2[0])) {
			result = openResourceFile(buf2, encByte);
			// We have .man files so set demo mode
			if (_gameId == GID_MANIAC)
				_demoMode = true;
		}
			
		if (result) {
			if (room == 0)
				return;
			if (_features & GF_EXTERNAL_CHARSET && room >= roomlimit)
				return;
			readRoomsOffsets();
			_fileOffset = res.roomoffs[rtRoom][room];

			if (_fileOffset != 8)
				return;

			error("Room %d not in %s", room, buf);
			return;
		}
		askForDisk(buf, room == 0 ? 0 : res.roomno[rtRoom][room]);
	}

	do {
		sprintf(buf, "%.3d.lfl", room);
		encByte = 0;
		if (openResourceFile(buf, encByte))
			break;
		askForDisk(buf, room == 0 ? 0 : res.roomno[rtRoom][room]);
	} while (1);

	deleteRoomOffsets();
	_fileOffset = 0;		// start of file
}

void ScummEngine::closeRoom() {
	if (_lastLoadedRoom != -1) {
		_lastLoadedRoom = -1;
		deleteRoomOffsets();
		_fileHandle.close();
	}
}

/** Delete the currently loaded room offsets. */
void ScummEngine::deleteRoomOffsets() {
	if (!(_features & GF_SMALL_HEADER) && !_dynamicRoomOffsets)
		return;

	for (int i = 0; i < _numRooms; i++) {
		if (res.roomoffs[rtRoom][i] != 0xFFFFFFFF)
			res.roomoffs[rtRoom][i] = 0;
	}
}

/** Read room offsets */
void ScummEngine::readRoomsOffsets() {
	int num, room, i;
	byte *ptr;

	debug(9, "readRoomOffsets()");

	deleteRoomOffsets();
	if (_features & GF_SMALL_NAMES)
		return;

	if (_heversion >= 70) { // Windows titles
		num = READ_LE_UINT16(_heV7RoomOffsets);
		ptr = _heV7RoomOffsets + 2;
		for (i = 0; i < num; i++) {
			res.roomoffs[rtRoom][i] = READ_LE_UINT32(ptr);	
			ptr += 4;
		}
		return;
	}
	
	if (!(_features & GF_SMALL_HEADER)) {
		if (!_dynamicRoomOffsets)
			return;

		_fileHandle.seek(16, SEEK_SET);
	} else {
		_fileHandle.seek(12, SEEK_SET);	// Directly searching for the room offset block would be more generic...
	}

	num = _fileHandle.readByte();
	while (num--) {
		room = _fileHandle.readByte();
		if (res.roomoffs[rtRoom][room] != 0xFFFFFFFF) {
			res.roomoffs[rtRoom][room] = _fileHandle.readUint32LE();
		} else {
			_fileHandle.readUint32LE();
		}
	}
}

bool ScummEngine::openFile(ScummFile &file, const char *filename) {
	bool result = false;

	if (!_containerFile.isEmpty()) {
		file.close();
		file.open(_containerFile.c_str());
		assert(file.isOpen());
		
		result = file.openSubFile(filename);
	}
	
	if (!result) {
		file.close();
		result = file.open(filename);
	}
	
	return result;
}

bool ScummEngine::openResourceFile(const char *filename, byte encByte) {
	debugC(DEBUG_GENERAL, "openResourceFile(%s)", filename);
	
	if (openFile(_fileHandle, filename)) {
		_fileHandle.setEnc(encByte);
		return true;
	}
	return false;
}

void ScummEngine::askForDisk(const char *filename, int disknum) {
	char buf[128];

	if (_version == 8) {
		char result;

		_imuseDigital->stopAllSounds();

#ifdef MACOSX
		sprintf(buf, "Cannot find file: '%s'\nPlease insert disc %d.\nPress OK to retry, Quit to exit", filename, disknum);
#else
		sprintf(buf, "Cannot find file: '%s'\nInsert disc %d into drive %s\nPress OK to retry, Quit to exit", filename, disknum, _gameDataPath.c_str());
#endif

		result = displayMessage("Quit", buf);
		if (!result) {
			error("Cannot find file: '%s'", filename);
		}
	} else { 
		sprintf(buf, "Cannot find file: '%s'", filename);
		InfoDialog dialog(this, (char*)buf);
		runDialog(dialog);
		error("Cannot find file: '%s'", filename);
	}
}

void ScummEngine::readIndexFile() {
	uint32 blocktype, itemsize;
	int numblock = 0;
	int num, i;
	bool stop = false;

	debugC(DEBUG_GENERAL, "readIndexFile()");

	closeRoom();
	openRoom(0);

	if (_version <= 5) {
		/* Figure out the sizes of various resources */
		while (!_fileHandle.eof()) {
			blocktype = fileReadDword();
			itemsize = _fileHandle.readUint32BE();
			if (_fileHandle.ioFailed())
				break;
			switch (blocktype) {
			case MKID('DOBJ'):
				_numGlobalObjects = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;
			case MKID('DROO'):
				_numRooms = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;

			case MKID('DSCR'):
				_numScripts = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;

			case MKID('DCOS'):
				_numCostumes = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;

			case MKID('DSOU'):
				_numSounds = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;
			}
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
		}
		_fileHandle.clearIOFailed();
		_fileHandle.seek(0, SEEK_SET);
	}

	while (!stop) {
		blocktype = fileReadDword();

		if (_fileHandle.ioFailed())
			break;
		itemsize = _fileHandle.readUint32BE();

		numblock++;

		switch (blocktype) {
		case MKID('DCHR'):
		case MKID('DIRF'):
			readResTypeList(rtCharset, MKID('CHAR'), "charset");
			break;
		
		case MKID('DOBJ'):
			debug(9, "found DOBJ block, reading object table");
			if (_version == 8)
				num = _fileHandle.readUint32LE();
			else
				num = _fileHandle.readUint16LE();
			assert(num == _numGlobalObjects);

			if (_version == 8) {	/* FIXME: Not sure.. */
				char buffer[40];
				for (i = 0; i < num; i++) {
					_fileHandle.read(buffer, 40);
					if (buffer[0]) {
						// Add to object name-to-id map
						_objectIDMap[buffer] = i;
					}
					_objectStateTable[i] = _fileHandle.readByte();
					_objectRoomTable[i] = _fileHandle.readByte();
					_classData[i] = _fileHandle.readUint32LE();
				}
				memset(_objectOwnerTable, 0xFF, num);
			} else if (_version == 7) {
				_fileHandle.read(_objectStateTable, num);
				_fileHandle.read(_objectRoomTable, num);
				memset(_objectOwnerTable, 0xFF, num);
			} else if (_heversion >= 70) { // HE Windows titles
				_fileHandle.read(_objectStateTable, num);
				_fileHandle.read(_objectOwnerTable, num);
				_fileHandle.read(_objectRoomTable, num);
			} else {
				_fileHandle.read(_objectOwnerTable, num);
				for (i = 0; i < num; i++) {
					_objectStateTable[i] = _objectOwnerTable[i] >> OF_STATE_SHL;
					_objectOwnerTable[i] &= OF_OWNER_MASK;
				}
			}
			
			if (_version != 8) {
				_fileHandle.read(_classData, num * sizeof(uint32));

				// Swap flag endian where applicable
#if defined(SCUMM_BIG_ENDIAN)
				for (i = 0; i != num; i++)
					_classData[i] = FROM_LE_32(_classData[i]);
#endif
			}
			break;

		case MKID('RNAM'):
			// Names of rooms
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
			debug(9, "found RNAM block, skipping");
			break;
		
		case MKID('DLFL'):
			i = _fileHandle.readUint16LE();
			_fileHandle.seek(-2, SEEK_CUR);
			_heV7RoomOffsets = (byte *)calloc(2 + (i * 4), 1);
			_fileHandle.read(_heV7RoomOffsets, (2 + (i * 4)) );
			break;

		case MKID('DIRM'):
			readResTypeList(rtImage, MKID('AWIZ'), "images");
			break;
			
		case MKID('DIRT'):
			readResTypeList(rtTalkie, MKID('TLKE'), "talkie");
			break;

		case MKID('SVER'):
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
			warning("SVER index block not yet handled, skipping");
			break;

		case MKID('DISK'):
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
			debug(2, "DISK index block not yet handled, skipping");
			break;

		case MKID('INIB'):
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
			debug(2, "INIB index block not yet handled, skipping");
			break;

		case MKID('DIRI'):
			readResTypeList(rtRoomImage, MKID('RMIM'), "room image");
			break;

		case MKID('ANAM'):		// Used by: The Dig, FT
			debug(9, "found ANAM block, reading audio names");
			_numAudioNames = _fileHandle.readUint16LE();
			_audioNames = (char*)malloc(_numAudioNames * 9);
			_fileHandle.read(_audioNames, _numAudioNames * 9);
			break;

		case MKID('DIRR'):
		case MKID('DROO'):
			readResTypeList(rtRoom, MKID('ROOM'), "room");
			break;

		case MKID('DRSC'):
			readResTypeList(rtRoomScripts, MKID('RMSC'), "room script");
			break;

		case MKID('DSCR'):
		case MKID('DIRS'):
			readResTypeList(rtScript, MKID('SCRP'), "script");
			break;

		case MKID('DCOS'):
		case MKID('DIRC'):
			readResTypeList(rtCostume, MKID('COST'), "costume");
			break;

		case MKID('MAXS'):
			readMAXS(itemsize);
			break;

		case MKID('DIRN'):
		case MKID('DSOU'):
			readResTypeList(rtSound, MKID('SOUN'), "sound");
			break;

		case MKID('AARY'):
			readArrayFromIndexFile();
			break;

		default:
			error("Bad ID %04X('%s') found in index file directory!", blocktype,
					tag2str(blocktype));
			return;
		}
	}

//  if (numblock!=9)
//    error("Not enough blocks read from directory");

	closeRoom();
}

void ScummEngine::readArrayFromIndexFile() {
	error("readArrayFromIndexFile() not supported in pre-V6 games");
}

void ScummEngine::readResTypeList(int id, uint32 tag, const char *name) {
	int num;
	int i;

	debug(9, "readResTypeList(%s,%s,%s)", resTypeFromId(id), tag2str(TO_BE_32(tag)), name);

	if (_version == 8)
		num = _fileHandle.readUint32LE();
	else if (!(_features & GF_OLD_BUNDLE))
		num = _fileHandle.readUint16LE();
	else
		num = _fileHandle.readByte();

	if (_features & GF_OLD_BUNDLE) {
		if (num >= 0xFF) {
			error("Too many %ss (%d) in directory", name, num);
		}
	} else {
		if (num != res.num[id]) {
			error("Invalid number of %ss (%d) in directory", name, num);
		}
	}

	if (_features & GF_OLD_BUNDLE) {
		if (id == rtRoom) {
			for (i = 0; i < num; i++)
				res.roomno[id][i] = i;
			_fileHandle.seek(num, SEEK_CUR);
		} else {
			for (i = 0; i < num; i++)
				res.roomno[id][i] = _fileHandle.readByte();
		}
		for (i = 0; i < num; i++) {
			res.roomoffs[id][i] = _fileHandle.readUint16LE();
			if (res.roomoffs[id][i] == 0xFFFF)
				res.roomoffs[id][i] = 0xFFFFFFFF;
		}

	} else if (_features & GF_SMALL_HEADER) {
		for (i = 0; i < num; i++) {
			res.roomno[id][i] = _fileHandle.readByte();
			res.roomoffs[id][i] = _fileHandle.readUint32LE();
		}
	} else {
		for (i = 0; i < num; i++) {
			res.roomno[id][i] = _fileHandle.readByte();
		}
		for (i = 0; i < num; i++) {
			res.roomoffs[id][i] = _fileHandle.readUint32LE();

			if (id == rtRoom && _heversion >= 70)
				_heV7RoomIntOffsets[i] = res.roomoffs[id][i];
		}

		if (_heversion >= 70) {
			for (i = 0; i < num; i++) {
				res.globsize[id][i] = _fileHandle.readUint32LE();
			}
		}
	}
}

void ScummEngine::allocResTypeData(int id, uint32 tag, int num, const char *name, int mode) {
	debug(9, "allocResTypeData(%s/%s,%s,%d,%d)", resTypeFromId(id), name, tag2str(TO_BE_32(tag)), num, mode);
	assert(id >= 0 && id < (int)(ARRAYSIZE(res.mode)));

	if (num >= 8000)
		error("Too many %ss (%d) in directory", name, num);

	res.mode[id] = mode;
	res.num[id] = num;
	res.tags[id] = tag;
	res.name[id] = name;
	res.address[id] = (byte **)calloc(num, sizeof(void *));
	res.flags[id] = (byte *)calloc(num, sizeof(byte));

	if (mode) {
		res.roomno[id] = (byte *)calloc(num, sizeof(byte));
		res.roomoffs[id] = (uint32 *)calloc(num, sizeof(uint32));
	}

	if (_heversion >= 70) {
		res.globsize[id] = (uint32 *)calloc(num, sizeof(uint32));

		if (id == rtRoom)
			_heV7RoomIntOffsets = (uint32 *)calloc(num, sizeof(uint32));
	}
}

void ScummEngine::loadCharset(int no) {
	int i;
	byte *ptr;

	debugC(DEBUG_GENERAL, "loadCharset(%d)", no);

	/* FIXME - hack around crash in Indy4 (occurs if you try to load after dieing) */
	if (_gameId == GID_INDY4 && no == 0)
		no = 1;

	/* for Humongous catalogs */
	if (_heversion >= 70 && _numCharsets == 1) {
		warning("not loading charset as it doesn't seem to exist?");
		return;
	}

	assert(no < (int)sizeof(_charsetData) / 16);
	checkRange(_numCharsets - 1, 1, no, "Loading illegal charset %d");

//  ensureResourceLoaded(rtCharset, no);
	ptr = getResourceAddress(rtCharset, no);
	if (_features & GF_SMALL_HEADER)
		ptr -= 12;

	for (i = 0; i < 15; i++) {
		_charsetData[no][i + 1] = ptr[i + 14];
	}
}

void ScummEngine::nukeCharset(int i) {
	checkRange(_numCharsets - 1, 1, i, "Nuking illegal charset %d");
	nukeResource(rtCharset, i);
}

void ScummEngine::ensureResourceLoaded(int type, int i) {
	void *addr = NULL;

	debugC(DEBUG_RESOURCE, "ensureResourceLoaded(%s,%d)", resTypeFromId(type), i);

	if ((type == rtRoom) && i > 0x7F && _version < 7) {
		i = _resourceMapper[i & 0x7F];
	}

	// FIXME - TODO: This check used to be "i==0". However, that causes
	// problems when using this function to ensure charset 0 is loaded.
	// This is done for many games, e.g. Zak256 or Indy3 (EGA and VGA).
	// For now we restrict the check to anything which is not a charset.
	// Question: Why was this check like that in the first place?
	// Answer: costumes with an index of zero in the newer games at least.
	// TODO: determine why the heck anything would try to load a costume
	// with id 0. Is that "normal", or is it caused by yet another bug in
	// our code base? After all we also have to add special cases for many
	// of our script opcodes that check for the (invalid) actor 0... so 
	// maybe both issues are related...
	if (type != rtCharset && i == 0)
		return;

	if (i <= res.num[type])
		addr = res.address[type][i];

	if (addr)
		return;

	loadResource(type, i);

	if (_version == 5 && type == rtRoom && i == _roomResource)
			VAR(VAR_ROOM_FLAG) = 1;
}

int ScummEngine::loadResource(int type, int idx) {
	int roomNr;
	uint32 fileOffs;
	uint32 size, tag;

	debugC(DEBUG_RESOURCE, "loadResource(%s,%d)", resTypeFromId(type),idx);

	if (type == rtCharset && (_features & GF_SMALL_HEADER)) {
		loadCharset(idx);
		return (1);
	}

	roomNr = getResourceRoomNr(type, idx);

	if (idx >= res.num[type])
		error("%s %d undefined %d %d", res.name[type], idx, res.num[type], roomNr);

	if (roomNr == 0)
		roomNr = _roomResource;

	if (type == rtRoom) {
		if (_version == 8)
			fileOffs = 8;
		else if (_heversion >= 70)
			fileOffs = _heV7RoomIntOffsets[idx];
		else
			fileOffs = 0;
	} else {
		fileOffs = res.roomoffs[type][idx];
		if (fileOffs == 0xFFFFFFFF)
			return 0;
	}

	openRoom(roomNr);

	_fileHandle.seek(fileOffs + _fileOffset, SEEK_SET);

	if (_features & GF_OLD_BUNDLE) {
		if ((_version == 3) && !(_features & GF_AMIGA) && (type == rtSound)) {
			return readSoundResourceSmallHeader(type, idx);
		} else {
			size = _fileHandle.readUint16LE();
			_fileHandle.seek(-2, SEEK_CUR);
		}
	} else if (_features & GF_SMALL_HEADER) {
		if (!(_features & GF_SMALL_NAMES))
			_fileHandle.seek(8, SEEK_CUR);
		size = _fileHandle.readUint32LE();
		tag = _fileHandle.readUint16LE();
		_fileHandle.seek(-6, SEEK_CUR);
		if ((type == rtSound) && !(_features & GF_AMIGA) && !(_features & GF_FMTOWNS)) {
			return readSoundResourceSmallHeader(type, idx);
		}
	} else {
		if (type == rtSound) {
			return readSoundResource(type, idx);
		}

		tag = fileReadDword();

		if (tag != res.tags[type] && _heversion < 70) {
			error("%s %d not in room %d at %d+%d in file %s",
					res.name[type], idx, roomNr,
					_fileOffset, fileOffs, _fileHandle.name());
		}

		size = _fileHandle.readUint32BE();
		_fileHandle.seek(-8, SEEK_CUR);
	}
	_fileHandle.read(createResource(type, idx, size), size);

	// dump the resource if requested
	if (_dumpScripts && type == rtScript) {
		dumpResource("script-", idx, getResourceAddress(rtScript, idx));
	}

	if (!_fileHandle.ioFailed()) {
		return 1;
	}

	nukeResource(type, idx);

	error("Cannot read resource");
}

int ScummEngine::getResourceRoomNr(int type, int idx) {
	if (type == rtRoom && _heversion < 70)
		return idx;
	return res.roomno[type][idx];
}

int ScummEngine::getResourceSize(int type, int idx) {
	byte *ptr = getResourceAddress(type, idx);
	MemBlkHeader *hdr = (MemBlkHeader *)(ptr - sizeof(MemBlkHeader));
	
	return hdr->size;
}

byte *ScummEngine::getResourceAddress(int type, int idx) {
	byte *ptr;

	CHECK_HEAP
	if (!validateResource("getResourceAddress", type, idx))
		return NULL;

	if (!res.address[type]) {
		debugC(DEBUG_RESOURCE, "getResourceAddress(%s,%d), res.address[type] == NULL", resTypeFromId(type), idx);
		return NULL;
	}

	if (res.mode[type] && !res.address[type][idx]) {
		ensureResourceLoaded(type, idx);
	}

	if (!(ptr = (byte *)res.address[type][idx])) {
		debugC(DEBUG_RESOURCE, "getResourceAddress(%s,%d) == NULL", resTypeFromId(type), idx);
		return NULL;
	}

	setResourceCounter(type, idx, 1);

	debugC(DEBUG_RESOURCE, "getResourceAddress(%s,%d) == %p", resTypeFromId(type), idx, ptr + sizeof(MemBlkHeader));
	return ptr + sizeof(MemBlkHeader);
}

byte *ScummEngine::getStringAddress(int i) {
	byte *addr = getResourceAddress(rtString, i);
	if (addr == NULL)
		return NULL;

	if (_heversion >= 72)
		return (addr + 0x14);	// ArrayHeader->data

	if (_features & GF_NEW_OPCODES)
		return (addr + 0x6);	// ArrayHeader->data
	return addr;
}

byte *ScummEngine::getStringAddressVar(int i) {
	return getStringAddress(_scummVars[i]);
}

void ScummEngine::setResourceCounter(int type, int idx, byte flag) {
	res.flags[type][idx] &= ~RF_USAGE;
	res.flags[type][idx] |= flag;
}

/* 2 bytes safety area to make "precaching" of bytes in the gdi drawer easier */
#define SAFETY_AREA 2

byte *ScummEngine::createResource(int type, int idx, uint32 size) {
	byte *ptr;

	CHECK_HEAP
	debugC(DEBUG_RESOURCE, "createResource(%s,%d,%d)", resTypeFromId(type), idx, size);

	if (!validateResource("allocating", type, idx))
		return NULL;
	nukeResource(type, idx);

	expireResources(size);

	CHECK_HEAP
	ptr = (byte *)calloc(size + sizeof(MemBlkHeader) + SAFETY_AREA, 1);
	if (ptr == NULL) {
		error("Out of memory while allocating %d", size);
	}

	_allocatedSize += size;

	res.address[type][idx] = ptr;
	((MemBlkHeader *)ptr)->size = size;
	setResourceCounter(type, idx, 1);
	return ptr + sizeof(MemBlkHeader);	/* skip header */
}

bool ScummEngine::validateResource(const char *str, int type, int idx) const {
	if (type < rtFirst || type > rtLast || (uint) idx >= (uint) res.num[type]) {
		warning("%s Illegal Glob type %s (%d) num %d", str, resTypeFromId(type), type, idx);
		return false;
	}
	return true;
}

void ScummEngine::nukeResource(int type, int idx) {
	byte *ptr;

	CHECK_HEAP
	if (!res.address[type])
		return;

	assert(idx >= 0 && idx < res.num[type]);

	if ((ptr = res.address[type][idx]) != NULL) {
		debugC(DEBUG_RESOURCE, "nukeResource(%s,%d)", resTypeFromId(type), idx);
		res.address[type][idx] = 0;
		res.flags[type][idx] = 0;
		_allocatedSize -= ((MemBlkHeader *)ptr)->size;
		free(ptr);
	}
}

const byte *ScummEngine::findResourceData(uint32 tag, const byte *ptr) {
	if (_features & GF_OLD_BUNDLE)
		error("findResourceData must not be used in GF_OLD_BUNDLE games");
	else if (_features & GF_SMALL_HEADER)
		ptr = findResourceSmall(tag, ptr);
	else
		ptr = findResource(tag, ptr);

	if (ptr == NULL)
		return NULL;
	return ptr + _resourceHeaderSize;
}

int ScummEngine::getResourceDataSize(const byte *ptr) const {
	if (ptr == NULL)
		return 0;

	if (_features & GF_OLD_BUNDLE)
		return READ_LE_UINT16(ptr) - 4;
	else if (_features & GF_SMALL_HEADER)
		return READ_LE_UINT32(ptr) - 6;
	else
		return READ_BE_UINT32(ptr - 4) - 8;
}

void ScummEngine::lock(int type, int i) {
	if (!validateResource("Locking", type, i))
		return;
	res.flags[type][i] |= RF_LOCK;
}

void ScummEngine::unlock(int type, int i) {
	if (!validateResource("Unlocking", type, i))
		return;
	res.flags[type][i] &= ~RF_LOCK;
}

bool ScummEngine::isResourceInUse(int type, int i) const {
	if (!validateResource("isResourceInUse", type, i))
		return false;
	switch (type) {
	case rtRoom:
		return _roomResource == (byte)i;
	case rtRoomScripts:
		return _roomResource == (byte)i;
	case rtScript:
		return isScriptInUse(i);
	case rtCostume:
		return isCostumeInUse(i);
	case rtSound:
		return _sound->isSoundInUse(i);
	default:
		return false;
	}
}

void ScummEngine::increaseResourceCounter() {
	int i, j;
	byte counter;

	for (i = rtFirst; i <= rtLast; i++) {
		for (j = res.num[i]; --j >= 0;) {
			counter = res.flags[i][j] & RF_USAGE;
			if (counter && counter < RF_USAGE_MAX) {
				setResourceCounter(i, j, counter + 1);
			}
		}
	}
}

void ScummEngine::expireResources(uint32 size) {
	int i, j;
	byte flag;
	byte best_counter;
	int best_type, best_res = 0;
	uint32 oldAllocatedSize;

	if (_expire_counter != 0xFF) {
		_expire_counter = 0xFF;
		increaseResourceCounter();
	}

	if (size + _allocatedSize < _maxHeapThreshold)
		return;

	oldAllocatedSize = _allocatedSize;

	do {
		best_type = 0;
		best_counter = 2;

		for (i = rtFirst; i <= rtLast; i++)
			if (res.mode[i]) {
				for (j = res.num[i]; --j >= 0;) {
					flag = res.flags[i][j];
					if (!(flag & RF_LOCK) && flag >= best_counter && res.address[i][j] && !isResourceInUse(i, j)) {
						best_counter = flag;
						best_type = i;
						best_res = j;
					}
				}
			}

		if (!best_type)
			break;
		nukeResource(best_type, best_res);
	} while (size + _allocatedSize > _minHeapThreshold);

	increaseResourceCounter();

	debugC(DEBUG_RESOURCE, "Expired resources, mem %d -> %d", oldAllocatedSize, _allocatedSize);
}

void ScummEngine::freeResources() {
	int i, j;
	for (i = rtFirst; i <= rtLast; i++) {
		for (j = res.num[i]; --j >= 0;) {
			if (isResourceLoaded(i, j))
				nukeResource(i, j);
		}
		free(res.address[i]);
		free(res.flags[i]);
		free(res.roomno[i]);
		free(res.roomoffs[i]);

		if (_heversion >= 70)
			free(res.globsize[i]);
	}
	if (_heversion >= 70) {
		free(_heV7RoomIntOffsets);
		free(_heV7RoomOffsets);
	}
}

void ScummEngine::loadPtrToResource(int type, int resindex, const byte *source) {
	byte *alloced;
	int i, len;

	nukeResource(type, resindex);

	len = resStrLen(source) + 1;

	if (len <= 0)
		return;

	alloced = createResource(type, resindex, len);

	if (!source) {
		alloced[0] = fetchScriptByte();
		for (i = 1; i < len; i++)
			alloced[i] = *_scriptPointer++;
	} else {
		for (i = 0; i < len; i++)
			alloced[i] = source[i];
	}
}

bool ScummEngine::isResourceLoaded(int type, int idx) const {
	if (!validateResource("isResourceLoaded", type, idx))
		return false;
	return res.address[type][idx] != NULL;
}

void ScummEngine::resourceStats() {
	int i, j;
	uint32 lockedSize = 0, lockedNum = 0;
	byte flag;

	for (i = rtFirst; i <= rtLast; i++)
		for (j = res.num[i]; --j >= 0;) {
			flag = res.flags[i][j];
			if (flag & RF_LOCK && res.address[i][j]) {
				lockedSize += ((MemBlkHeader *)res.address[i][j])->size;
				lockedNum++;
			}
		}

	debug(1, "Total allocated size=%d, locked=%d(%d)", _allocatedSize, lockedSize, lockedNum);
}

void ScummEngine::readMAXS(int blockSize) {
	debug(9, "readMAXS: MAXS has blocksize %d", blockSize);

	if (_version == 8) {                    // CMI
		_fileHandle.seek(50 + 50, SEEK_CUR);            // 176 - 8
		_numVariables = _fileHandle.readUint32LE();     // 1500
		_numBitVariables = _fileHandle.readUint32LE();  // 2048
		_fileHandle.readUint32LE();                     // 40
		_numScripts = _fileHandle.readUint32LE();       // 458
		_numSounds = _fileHandle.readUint32LE();        // 789
		_numCharsets = _fileHandle.readUint32LE();      // 1
		_numCostumes = _fileHandle.readUint32LE();      // 446
		_numRooms = _fileHandle.readUint32LE();         // 95
		_fileHandle.readUint32LE();                     // 80
		_numGlobalObjects = _fileHandle.readUint32LE(); // 1401
		_fileHandle.readUint32LE();                     // 60
		_numLocalObjects = _fileHandle.readUint32LE();  // 200
		_numNewNames = _fileHandle.readUint32LE();      // 100
		_numFlObject = _fileHandle.readUint32LE();      // 128
		_numInventory = _fileHandle.readUint32LE();     // 80
		_numArray = _fileHandle.readUint32LE();         // 200
		_numVerbs = _fileHandle.readUint32LE();         // 50

		_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);
		_numGlobalScripts = 2000;

		_shadowPaletteSize = NUM_SHADOW_PALETTE * 256;
	} else if (_version == 7) {
		_fileHandle.seek(50 + 50, SEEK_CUR);
		_numVariables = _fileHandle.readUint16LE();
		_numBitVariables = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();                      // 40 in FT; 16 in Dig
		_numGlobalObjects = _fileHandle.readUint16LE();
		_numLocalObjects = _fileHandle.readUint16LE();
		_numNewNames = _fileHandle.readUint16LE();
		_numVerbs = _fileHandle.readUint16LE();
		_numFlObject = _fileHandle.readUint16LE();
		_numInventory = _fileHandle.readUint16LE();
		_numArray = _fileHandle.readUint16LE();
		_numRooms = _fileHandle.readUint16LE();
		_numScripts = _fileHandle.readUint16LE();
		_numSounds = _fileHandle.readUint16LE();
		_numCharsets = _fileHandle.readUint16LE();
		_numCostumes = _fileHandle.readUint16LE();

		_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);

		if ((_gameId == GID_FT) && (_features & GF_DEMO) && 
		    (_features & GF_PC))
			_numGlobalScripts = 300;
		else
			_numGlobalScripts = 2000;

		_shadowPaletteSize = NUM_SHADOW_PALETTE * 256;
	} else if (_heversion >= 70 && (blockSize == 44 + 8)) { // C++ based engine
		_numVariables = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();
		_numRoomVariables = _fileHandle.readUint16LE();
		_numLocalObjects = _fileHandle.readUint16LE();
		_numArray = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE(); // unknown
		_fileHandle.readUint16LE(); // unknown
		_numFlObject = _fileHandle.readUint16LE();
		_numInventory = _fileHandle.readUint16LE();
		_numRooms = _fileHandle.readUint16LE();
		_numScripts = _fileHandle.readUint16LE();
		_numSounds = _fileHandle.readUint16LE();
		_numCharsets = _fileHandle.readUint16LE();
		_numCostumes = _fileHandle.readUint16LE();
		_numGlobalObjects = _fileHandle.readUint16LE();
		_numImages = _fileHandle.readUint16LE();
		_numSprites = _fileHandle.readUint16LE();
		_numLocalScripts = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE(); // heap related
		_numPalettes = _fileHandle.readUint16LE();
		_numUnk = _fileHandle.readUint16LE();
		_numTalkies = _fileHandle.readUint16LE();
		_numNewNames = 10;

		_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);
		_numGlobalScripts = 2048;

	} else if (_heversion >= 70 && (blockSize == 38 + 8)) { // Scummsys.9x
		_numVariables = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();
		_numRoomVariables = _fileHandle.readUint16LE();
		_numLocalObjects = _fileHandle.readUint16LE();
		_numArray = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE(); // unknown
		_fileHandle.readUint16LE(); // unknown
		_numFlObject = _fileHandle.readUint16LE();
		_numInventory = _fileHandle.readUint16LE();
		_numRooms = _fileHandle.readUint16LE();
		_numScripts = _fileHandle.readUint16LE();
		_numSounds = _fileHandle.readUint16LE();
		_numCharsets = _fileHandle.readUint16LE();
		_numCostumes = _fileHandle.readUint16LE();
		_numGlobalObjects = _fileHandle.readUint16LE();
		_numImages = _fileHandle.readUint16LE();
		_numSprites = _fileHandle.readUint16LE();
		_numLocalScripts = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE(); // heap releated
		_numNewNames = 10;

		_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);
		if (_gameId == GID_FREDDI4)
			_numGlobalScripts = 2048;
		else
			_numGlobalScripts = 200;

	} else if (_heversion >= 70 && blockSize > 38) { // sputm7.2
		if (blockSize != 32 + 8)
				error("MAXS block of size %d not supported, please report", blockSize);
		_numVariables = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();
		_numBitVariables = _numRoomVariables = _fileHandle.readUint16LE();
		_numLocalObjects = _fileHandle.readUint16LE();
		_numArray = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();
		_numVerbs = _fileHandle.readUint16LE();
		_numFlObject = _fileHandle.readUint16LE();
		_numInventory = _fileHandle.readUint16LE();
		_numRooms = _fileHandle.readUint16LE();
		_numScripts = _fileHandle.readUint16LE();
		_numSounds = _fileHandle.readUint16LE();
		_numCharsets = _fileHandle.readUint16LE();
		_numCostumes = _fileHandle.readUint16LE();
		_numGlobalObjects = _fileHandle.readUint16LE();
		_numImages = _fileHandle.readUint16LE();
		_numNewNames = 10;

		_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);
		_numGlobalScripts = 200;

	} else if (_version == 6) {
		if (blockSize != 30 + 8)
			error("MAXS block of size %d not supported", blockSize);
		_numVariables = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();                      // 16 in Sam/DOTT
		_numBitVariables = _fileHandle.readUint16LE();
		_numLocalObjects = _fileHandle.readUint16LE();
		_numArray = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();                      // 0 in Sam/DOTT
		_numVerbs = _fileHandle.readUint16LE();
		_numFlObject = _fileHandle.readUint16LE();
		_numInventory = _fileHandle.readUint16LE();
		_numRooms = _fileHandle.readUint16LE();
		_numScripts = _fileHandle.readUint16LE();
		_numSounds = _fileHandle.readUint16LE();
		_numCharsets = _fileHandle.readUint16LE();
		_numCostumes = _fileHandle.readUint16LE();
		_numGlobalObjects = _fileHandle.readUint16LE();
		_numNewNames = 50;

		_objectRoomTable = NULL;
		_numGlobalScripts = 200;

		_shadowPaletteSize = 256;

		if (_heversion >= 70) {
			_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);
		}
	} else {
		_numVariables = _fileHandle.readUint16LE();      // 800
		_fileHandle.readUint16LE();                      // 16
		_numBitVariables = _fileHandle.readUint16LE();   // 2048
		_numLocalObjects = _fileHandle.readUint16LE();   // 200
		_numArray = 50;
		_numVerbs = 100;
		// Used to be 50, which wasn't enough for MI2 and FOA. See bugs
		// #933610, #936323 and #941275.
		_numNewNames = 150;
		_objectRoomTable = NULL;

		_fileHandle.readUint16LE();                      // 50
		_numCharsets = _fileHandle.readUint16LE();       // 9
		_fileHandle.readUint16LE();                      // 100
		_fileHandle.readUint16LE();                      // 50
		_numInventory = _fileHandle.readUint16LE();      // 80
		_numGlobalScripts = 200;

		_shadowPaletteSize = 256;

		_numFlObject = 50;
	}

	if (_shadowPaletteSize)
		_shadowPalette = (byte *)calloc(_shadowPaletteSize, 1);

	allocateArrays();
	_dynamicRoomOffsets = true;
}

void ScummEngine::allocateArrays() {
	// Note: Buffers are now allocated in scummMain to allow for
	//     early GUI init.

	_objectOwnerTable = (byte *)calloc(_numGlobalObjects, 1);
	_objectStateTable = (byte *)calloc(_numGlobalObjects, 1);
	_classData = (uint32 *)calloc(_numGlobalObjects, sizeof(uint32));
	_newNames = (uint16 *)calloc(_numNewNames, sizeof(uint16));

	_inventory = (uint16 *)calloc(_numInventory, sizeof(uint16));
	_verbs = (VerbSlot *)calloc(_numVerbs, sizeof(VerbSlot));
	_objs = (ObjectData *)calloc(_numLocalObjects, sizeof(ObjectData));
	_roomVars = (int32 *)calloc(_numRoomVariables, sizeof(int32));
	_scummVars = (int32 *)calloc(_numVariables, sizeof(int32));
	_bitVars = (byte *)calloc(_numBitVariables >> 3, 1);
	if (_features & GF_HUMONGOUS)
		_arraySlot = (byte *)calloc(_numArray, 1);

	allocResTypeData(rtCostume, (_features & GF_NEW_COSTUMES) ? MKID('AKOS') : MKID('COST'),
								_numCostumes, "costume", 1);
	allocResTypeData(rtRoom, MKID('ROOM'), _numRooms, "room", 1);
	allocResTypeData(rtRoomImage, MKID('RMIM'), _numRooms, "room image", 1);
	allocResTypeData(rtRoomScripts, MKID('RMSC'), _numRooms, "room script", 1);
	allocResTypeData(rtSound, MKID('SOUN'), _numSounds, "sound", 2);
	allocResTypeData(rtScript, MKID('SCRP'), _numScripts, "script", 1);
	allocResTypeData(rtCharset, MKID('CHAR'), _numCharsets, "charset", 1);
	allocResTypeData(rtObjectName, MKID('NONE'), _numNewNames, "new name", 0);
	allocResTypeData(rtInventory, MKID('NONE'), _numInventory, "inventory", 0);
	allocResTypeData(rtTemp, MKID('NONE'), 10, "temp", 0);
	allocResTypeData(rtScaleTable, MKID('NONE'), 5, "scale table", 0);
	allocResTypeData(rtActorName, MKID('NONE'), _numActors, "actor name", 0);
	allocResTypeData(rtVerb, MKID('NONE'), _numVerbs, "verb", 0);
	allocResTypeData(rtString, MKID('NONE'), _numArray, "array", 0);
	allocResTypeData(rtFlObject, MKID('NONE'), _numFlObject, "flobject", 0);
	allocResTypeData(rtMatrix, MKID('NONE'), 10, "boxes", 0);
	allocResTypeData(rtImage, MKID('AWIZ'), _numImages, "images", 1);
	allocResTypeData(rtTalkie, MKID('TLKE'), _numTalkies, "talkie", 1);

}

void ScummEngine::dumpResource(const char *tag, int idx, const byte *ptr, int length) {
	char buf[256];
	File out;

	uint32 size;
	if (length >= 0)
		size = length;
	else if (_features & GF_OLD_BUNDLE)
		size = READ_LE_UINT16(ptr);
	else if (_features & GF_SMALL_HEADER)
		size = READ_LE_UINT32(ptr);
	else
		size = READ_BE_UINT32(ptr + 4);

#if defined(MACOS_CARBON)
	sprintf(buf, ":dumps:%s%d.dmp", tag, idx);
#else
	sprintf(buf, "dumps/%s%d.dmp", tag, idx);
#endif

	out.open(buf, File::kFileWriteMode);
	if (out.isOpen() == false)
		return;
	out.write(ptr, size);
	out.close();
}

ResourceIterator::ResourceIterator(const byte *searchin, bool smallHeader)
	: _ptr(searchin), _smallHeader(smallHeader) {
	assert(searchin);
	if (_smallHeader) {
		_size = READ_LE_UINT32(searchin);
		_pos = 6;
		_ptr = searchin + 6;
	} else {
		_size = READ_BE_UINT32(searchin + 4);
		_pos = 8;
		_ptr = searchin + 8;
	}
	
}

const byte *ResourceIterator::findNext(uint32 tag) {
	uint32 size = 0;
	const byte *result = 0;
	
	if (_smallHeader) {
		uint16 smallTag = newTag2Old(tag);
		do {
			if (_pos >= _size)
				return 0;
	
			result = _ptr;
			size = READ_LE_UINT32(result);
			if ((int32)size <= 0)
				return 0;	// Avoid endless loop
			
			_pos += size;
			_ptr += size;
		} while (READ_LE_UINT16(result + 4) != smallTag);
	} else {
		do {
			if (_pos >= _size)
				return 0;
	
			result = _ptr;
			size = READ_BE_UINT32(result + 4);
			if ((int32)size <= 0)
				return 0;	// Avoid endless loop
			
			_pos += size;
			_ptr += size;
		} while (READ_UINT32(result) != tag);
	}

	return result;
}

const byte *ScummEngine::findResource(uint32 tag, const byte *searchin) {
	uint32 curpos, totalsize, size;

	debugC(DEBUG_RESOURCE, "findResource(%s, %lx)", tag2str(tag), searchin);

	if (!searchin) {
		if (_heversion >= 70) {
			searchin = _resourceLastSearchBuf;
			totalsize = _resourceLastSearchSize;
			curpos = 0;
		} else {
			assert(searchin);
			return NULL;
		}
	} else {
		searchin += 4;
		_resourceLastSearchSize = totalsize = READ_BE_UINT32(searchin);
		curpos = 8;
		searchin += 4;
	}

	while (curpos < totalsize) {
		if (READ_UINT32(searchin) == tag) {
			_resourceLastSearchBuf = searchin;
			return searchin;
		}

		size = READ_BE_UINT32(searchin + 4);
		if ((int32)size <= 0) {
			error("(%s) Not found in %d... illegal block len %d", tag2str(tag), 0, size);
			return NULL;
		}

		curpos += size;
		searchin += size;
	}

	return NULL;
}

const byte *findResourceSmall(uint32 tag, const byte *searchin) {
	uint32 curpos, totalsize, size;
	uint16 smallTag;

	smallTag = newTag2Old(tag);
	if (smallTag == 0)
		return NULL;

	assert(searchin);

	totalsize = READ_LE_UINT32(searchin);
	searchin += 6;
	curpos = 6;

	while (curpos < totalsize) {
		size = READ_LE_UINT32(searchin);

		if (READ_LE_UINT16(searchin + 4) == smallTag)
			return searchin;

		if ((int32)size <= 0) {
			error("(%s) Not found in %d... illegal block len %d", tag2str(tag), 0, size);
			return NULL;
		}

		curpos += size;
		searchin += size;
	}

	return NULL;
}

uint16 newTag2Old(uint32 oldTag) {
	switch (oldTag) {
	case (MKID('RMHD')):
		return (0x4448);	// HD
	case (MKID('IM00')):
		return (0x4D42);	// BM
	case (MKID('EXCD')):
		return (0x5845);	// EX
	case (MKID('ENCD')):
		return (0x4E45);	// EN
	case (MKID('SCAL')):
		return (0x4153);	// SA
	case (MKID('LSCR')):
		return (0x534C);	// LS
	case (MKID('OBCD')):
		return (0x434F);	// OC
	case (MKID('OBIM')):
		return (0x494F);	// OI
	case (MKID('SMAP')):
		return (0x4D42);	// BM
	case (MKID('CLUT')):
		return (0x4150);	// PA
	case (MKID('BOXD')):
		return (0x5842);	// BX
	case (MKID('CYCL')):
		return (0x4343);	// CC
	case (MKID('EPAL')):
		return (0x5053);	// SP
	default:
		return (0);
	}
}

const char *resTypeFromId(int id) {
	static char buf[100];

	switch (id) {
	case rtRoom:
		return "Room";
	case rtScript:
		return "Script";
	case rtCostume:
		return "Costume";
	case rtSound:
		return "Sound";
	case rtInventory:
		return "Inventory";
	case rtCharset:
		return "Charset";
	case rtString:
		return "String";
	case rtVerb:
		return "Verb";
	case rtActorName:
		return "ActorName";
	case rtBuffer:
		return "Buffer";
	case rtScaleTable:
		return "ScaleTable";
	case rtTemp:
		return "Temp";
	case rtFlObject:
		return "FlObject";
	case rtMatrix:
		return "Matrix";
	case rtBox:
		return "Box";
	case rtObjectName:
		return "ObjectName";
	case rtRoomScripts:
		return "RoomScripts";
	case rtRoomImage:
		return "RoomImage";
	case rtImage:
		return "Image";
	case rtTalkie:
		return "Talkie";
	case rtNumTypes:
		return "NumTypes";
	default:
		sprintf(buf, "%d", id);
		return buf;
	}
}

} // End of namespace Scumm
