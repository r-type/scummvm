/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
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
#include "scumm.h"

/* Open a room */
void Scumm::openRoom(int room) {
	uint room_offs;
	char buf[256];

	debug(9, "openRoom(%d)",room);

	/* Don't load the same room again */
	if (_lastLoadedRoom == room)
		return;
	_lastLoadedRoom = room;

	/* Room -1 means close file */
	if (room==-1) {
		_encbyte = 0;
		deleteRoomOffsets();
		fileClose(_fileHandle);
		_fileHandle = NULL;
		return;
	}

	/* Either xxx.lfl or monkey.xxx file name */
	while (!_resFilePrefix) {
#if REAL_CODE
		room_offs = _roomFileOffsets[room];
#else
		room_offs = room ? _roomFileOffsets[room] : 0;
#endif
		
		if (room_offs == 0xFFFFFFFF)
			break;

		if (room_offs != 0 && room != 0) {
			_fileOffset = _roomFileOffsets[room];
			return;
		}
		sprintf(buf, "%s.%.3d", _exe_name, 
			room==0 ? 0 : res.roomno[rtRoom][room]);

		_encbyte = 0x69;
		if (openResourceFile(buf)) {
			if (room==0)
				return;
			readRoomsOffsets();
			_fileOffset = _roomFileOffsets[room];

			if (_fileOffset != 8)
				return;

			error("Room %d not in %s", room, buf);
			return;
		}
		askForDisk(buf);
	}

	do {
		sprintf(buf, "%.3d.lfl", room);
		_encbyte = 0;
		if (openResourceFile(buf)) 
			break;
		askForDisk(buf);
	} while(1);

	deleteRoomOffsets();
	_fileOffset = 0; /* start of file */
}

/* Delete the currently loaded room offsets */
void Scumm::deleteRoomOffsets() {
	if (!_dynamicRoomOffsets)
		return;
	
	for (int i=0; i<_maxRooms; i++) {
		if (_roomFileOffsets[i]!=0xFFFFFFFF)
			_roomFileOffsets[i] = 0;
	}
}

/* Read room offsets */
void Scumm::readRoomsOffsets() {
	int num,room;

	debug(9, "readRoomOffsets()");

	deleteRoomOffsets();
	if (!_dynamicRoomOffsets)
		return;

	fileSeek(_fileHandle, 16, SEEK_SET);
	
	num = fileReadByte();
	while (num) {
		num--;
		
		room = fileReadByte();
		if (_roomFileOffsets[room]!=0xFFFFFFFF) {
			_roomFileOffsets[room] = fileReadDwordLE();
		} else {
			fileReadDwordLE();
		}
	}
}

bool Scumm::openResourceFile(const char *filename) {
	char buf[256];
	
	debug(9, "openResourceFile(%s)",filename);

	if (_resFilePath) {
		sprintf(buf, "%s.%d\\%s", _resFilePath, _resFilePathId, filename);
	} else if (_resFilePrefix) {
		sprintf(buf, "%s%s", _resFilePrefix, filename);
	} else {
		strcpy(buf, filename);
	}

	if (_fileHandle != NULL) {
		fileClose(_fileHandle);
		_fileHandle = NULL;
	}

	_fileHandle = fileOpen(buf, 1);
	
	return _fileHandle != NULL;
}

void Scumm::askForDisk(const char *filename) {
	error("Cannot find '%s'", filename);
}

void Scumm::readIndexFileV5(int mode) {
	uint32 blocktype,itemsize;
	int numblock = 0;
#if defined(SCUMM_BIG_ENDIAN)
	int i;
#endif

	debug(9, "readIndexFile(%d)",mode);

	openRoom(-1);
	openRoom(0);
	
	while (1) {
		blocktype = fileReadDword();

		if (fileReadFailed(_fileHandle))
			break;
		itemsize = fileReadDwordBE();

		numblock++;

		switch(blocktype) {
		case MKID('DCHR'):
			readResTypeList(rtCharset,MKID('CHAR'),"charset");
			break;

		case MKID('DOBJ'):
			_numGlobalObjects = fileReadWordLE();
			_objectFlagTable = (byte*)alloc(_numGlobalObjects);
			if (mode==1) {
				fileSeek(_fileHandle, itemsize - 10, 1);
				break;
			}

			_classData = (uint32*)alloc(_numGlobalObjects * sizeof(uint32));
			fileRead(_fileHandle, _objectFlagTable, _numGlobalObjects);
			fileRead(_fileHandle, _classData, _numGlobalObjects * sizeof(uint32));
#if defined(SCUMM_BIG_ENDIAN)
			for (i=0; i<_numGlobalObjects; i++)
				_classData[i] = FROM_LE_32(_classData[i]);
#endif
			break;

		case MKID('RNAM'):
			fileSeek(_fileHandle, itemsize-8,1);
			break;

		case MKID('DROO'):
			readResTypeList(rtRoom,MKID('ROOM'),"room");
			break;

		case MKID('DSCR'):
			readResTypeList(rtScript,MKID('SCRP'),"script");
			break;

		case MKID('DCOS'):
			readResTypeList(rtCostume,MKID('COST'),"costume");
			break;

		case MKID('MAXS'):
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			fileReadWordLE();
			break;

		case MKID('DSOU'):
			readResTypeList(rtSound,MKID('SOUN'),"sound");
			break;

		default:
			error("Bad ID %c%c%c%c found in directory!", blocktype&0xFF, blocktype>>8, blocktype>>16, blocktype>>24);
			return;
		}
	}

	clearFileReadFailed(_fileHandle);

	if (numblock!=8)
		error("Not enough blocks read from directory");

	openRoom(-1);
	
	_numGlobalScripts = _maxScripts;
	_dynamicRoomOffsets = true;
}

void Scumm::readIndexFileV6() {
	uint32 blocktype,itemsize;
	int numblock = 0;
	int num, i;

	debug(9, "readIndexFile()");

	openRoom(-1);
	openRoom(0);
	
	while (1) {
		blocktype = fileReadDword();

		if (fileReadFailed(_fileHandle))
			break;
		itemsize = fileReadDwordBE();

		numblock++;

			switch(blocktype) {
		case MKID('DCHR'):
			readResTypeList(rtCharset,MKID('CHAR'),"charset");
			break;

		case MKID('DOBJ'):
			 num = fileReadWordLE();
			 assert(num == _numGlobalObjects);
			fileRead(_fileHandle, _objectFlagTable, num);
			fileRead(_fileHandle, _classData, num * sizeof(uint32));
#if defined(SCUMM_BIG_ENDIAN)
			for (i=0; i<_numGlobalObjects; i++)
				_classData[i] = FROM_LE_32(_classData[i]);
#endif
			break;

		case MKID('RNAM'):
			fileSeek(_fileHandle, itemsize-8,1);
			break;

		case MKID('DROO'):
			readResTypeList(rtRoom,MKID('ROOM'),"room");
			break;

		case MKID('DSCR'):
			readResTypeList(rtScript,MKID('SCRP'),"script");
			break;

		case MKID('DCOS'):
			readResTypeList(rtCostume,MKID('COST'),"costume");
			break;

		case MKID('MAXS'):
			readMAXS();
			break;

		case MKID('DSOU'):
			readResTypeList(rtSound,MKID('SOUN'),"sound");
			break;

		case MKID('AARY'):
			readArrayFromIndexFile();
			break;

		default:
			error("Bad ID %c%c%c%c found in directory!", blocktype&0xFF, blocktype>>8, blocktype>>16, blocktype>>24);
			return;
		}
	}

	clearFileReadFailed(_fileHandle);

	if (numblock!=9)
		error("Not enough blocks read from directory");

	openRoom(-1);
}


void Scumm::readArrayFromIndexFile() {
	int num;
	int a,b,c;

	while ((num = fileReadWordLE()) != 0) {
		a = fileReadWordLE();
		b = fileReadWordLE();
		c = fileReadWordLE();
		if (c==1)
			defineArray(num, 1, a, b);
		else
			defineArray(num, 5, a, b);
	}
}

void Scumm::readResTypeList(int id, uint32 tag, const char *name) {
	int num;
	int i;
	
	debug(9, "readResTypeList(%d,%x,%s)",id,FROM_LE_32(tag),name);
	
	num = fileReadWordLE();

	if (_majorScummVersion == 6) {
		if (num != res.num[id]) {
			error("Invalid number of %ss (%d) in directory", name, num);	
		}
	} else {
		if (num>=0xFF) {
			error("Too many %ss (%d) in directory", name, num);
		}
		allocResTypeData(id, tag, num, name, 1);
	}
	
	fileRead(_fileHandle, res.roomno[id], num*sizeof(uint8));
	fileRead(_fileHandle, res.roomoffs[id], num*sizeof(uint32));

#if defined(SCUMM_BIG_ENDIAN)
	for (i=0; i<num; i++)
		res.roomoffs[id][i] = FROM_LE_32(res.roomoffs[id][i]);
#endif
}


void Scumm::allocResTypeData(int id, uint32 tag, int num, const char *name, int mode) {
	debug(9, "allocResTypeData(%d,%x,%d,%s,%d)",id,FROM_LE_32(tag),num,name,mode);
	assert(id>=0 && id<sizeof(res.mode)/sizeof(res.mode[0]));

	if (num>=512) {
		error("Too many %ss (%d) in directory", name, num);
	}

	res.mode[id] = mode;
	res.num[id] = num;
	res.tags[id] = tag;
	res.name[id] = name;
	res.address[id] = (byte**)alloc(num*sizeof(void*));
	res.flags[id] = (byte*)alloc(num*sizeof(byte));

	if (mode) {
		res.roomno[id] = (byte*)alloc(num*sizeof(byte));
		res.roomoffs[id] = (uint32*)alloc(num*sizeof(uint32));
	}
}

void Scumm::loadCharset(int no) {
	int i;
	byte *ptr;

	debug(9, "loadCharset(%d)",no);

	checkRange(_maxCharsets-1, 1, no, "Loading illegal charset %d");
//	ensureResourceLoaded(6, no);
	ptr = getResourceAddress(6, no);

	for (i=0; i<15; i++) {
		_charsetData[no][i+1] = ptr[i+14];
	}
}

void Scumm::nukeCharset(int i) {
	checkRange(_maxCharsets-1, 1, i, "Nuking illegal charset %d");
	nukeResource(rtCharset, i);
}

void Scumm::ensureResourceLoaded(int type, int i) {
	void *addr;

	debug(9, "ensureResourceLoaded(%d,%d)", type, i);

	if (type==rtRoom && i>127) {
		i = _resourceMapper[i&127];
	}

	if (i==0)
		return;

	addr = res.address[type][i];
	if (addr)
		return;

	loadResource(type, i);

	if (type==rtRoom && i==_roomResource)
		_vars[VAR_ROOM_FLAG] = 1;
}

int Scumm::loadResource(int type, int index) {
	int roomNr, i;
	uint32 fileOffs;
	uint32 size, tag;
	
	debug(9, "loadResource(%d,%d)", type,index);

	roomNr = getResourceRoomNr(type, index);
	if (roomNr == 0 || index >= res.num[type]) {
		error("%s %d undefined", 
			res.name[type],index);
	}

	if (type==rtRoom) {
		fileOffs = 0;
	} else {
		fileOffs = res.roomoffs[type][index];
		if (fileOffs==0xFFFFFFFF)
			return 0;
	}
		
	do {
		for (i=0; i<5; i++) {
			openRoom(roomNr);

			fileSeek(_fileHandle, fileOffs + _fileOffset, SEEK_SET);
			if (type==rtSound) {
				fileReadDwordLE();
				fileReadDwordLE();
				return readSoundResource(type, index);
			}
			
			tag = fileReadDword();

			if (tag != res.tags[type]) {
				error("%s %d not in room %d at %d+%d", 
					res.name[type], type, roomNr, _fileOffset, fileOffs);
			}

			size = fileReadDwordBE();
			fileSeek(_fileHandle, -8, SEEK_CUR);

			fileRead(_fileHandle, createResource(type, index, size), size);

			/* dump the resource */
#ifdef DUMP_SCRIPTS
			if(type==rtScript) {
				dumpResource("script-", index, getResourceAddress(rtScript, index));
			}
#endif

			if (!fileReadFailed(_fileHandle)) {
				return 1;
			}

			nukeResource(type, index);
		}

		error("Cannot read resource");
	} while(1);
}

int Scumm::readSoundResource(int type, int index) {
	uint32 resStart, size, tag, size2;
	byte *ptr;
	int i;

	debug(9, "readSoundResource(%d,%d)", type, index);

	resStart = 0;

	fileReadDwordLE();
	size = fileReadDwordBE();

	while (size>resStart) {
		tag = fileReadDword();
		size2 = fileReadDwordBE();
		
		resStart += size2 + 8;
		
		for (i=0,ptr=_soundTagTable; i<_numSoundTags; i++,ptr+=4) {
/* endian OK, tags are in native format */
			if (READ_UINT32_UNALIGNED(ptr) == tag) {
				fileSeek(_fileHandle, -8, SEEK_CUR);
				fileRead(_fileHandle,createResource(type, index, size2+8), size2+8);
				return 1;
			}
		}

		fileSeek(_fileHandle, size2, SEEK_CUR);
	}

	res.roomoffs[type][index] = 0xFFFFFFFF;
	return 0;
}

int Scumm::getResourceRoomNr(int type, int index) {
	if (type==rtRoom)
		return index;
	return res.roomno[type][index];
}

byte *Scumm::getResourceAddress(int type, int index) {
	byte *ptr;

	debug(9, "getResourceAddress(%d,%d)", type, index);

	CHECK_HEAP

	validateResource("getResourceAddress", type, index);
	
	if (res.mode[type] && !res.address[type][index]) {
		ensureResourceLoaded(type, index);
	}	

	setResourceCounter(type, index, 1);

	ptr=(byte*)res.address[type][index];
	if (!ptr)
		return NULL;

	return ptr + sizeof(ResHeader);
}

byte *Scumm::getStringAddress(int i) {
	byte *b = getResourceAddress(rtString, i);
	if (!b)
		return b;
	
	if (_majorScummVersion==6)
		return ((ArrayHeader*)b)->data;
	return b;
}

void Scumm::setResourceCounter(int type, int index, byte flag) {
	res.flags[type][index] &= 0x80;
	res.flags[type][index] |= flag;
}

/* 2 bytes safety area to make "precaching" of bytes in the gdi drawer easier */
#define SAFETY_AREA 2

byte *Scumm::createResource(int type, int index, uint32 size) {
	byte *ptr;

	CHECK_HEAP

	debug(9, "createResource(%d,%d,%d)", type, index,size);

	if (size > 65536*4+37856)
		error("Invalid size allocating");

	validateResource("allocating", type, index);
	nukeResource(type, index);

	expireResources(size);

	CHECK_HEAP
	
	ptr = (byte*)alloc(size + sizeof(ResHeader) + SAFETY_AREA);
	if (ptr==NULL) {
		error("Out of memory while allocating %d", size);
	}

	_allocatedSize += size;

	res.address[type][index] = ptr;
	((ResHeader*)ptr)->size = size;
	setResourceCounter(type, index, 1);
	return ptr + sizeof(ResHeader); /* skip header */
}

void Scumm::validateResource(const char *str, int type, int index) {
	if (type<rtFirst || type>rtLast || (uint)index >= (uint)res.num[type]) {
		error("%d Illegal Glob type %d num %d", str, type, index);
	}
}

void Scumm::nukeResource(int type, int index) {
	byte *ptr;

	debug(9, "nukeResource(%d,%d)", type, index);

	CHECK_HEAP
	assert( res.address[type] );
	assert( index>=0 && index < res.num[type]);

	if ((ptr = res.address[type][index]) != NULL) {
		res.address[type][index] = 0;
		res.flags[type][index] = 0;
		_allocatedSize -= ((ResHeader*)ptr)->size;
		free(ptr);
	}
}

byte *findResource(uint32 tag, byte *searchin, int index) {
	uint32 maxsize,curpos,totalsize,size;

	searchin += 4;
	totalsize = READ_BE_UINT32_UNALIGNED(searchin);
	curpos = 8;
	searchin += 4;

	while (curpos<totalsize) {
		if (READ_UINT32_UNALIGNED(searchin)==tag && !index--)
			return searchin;

		size = READ_BE_UINT32_UNALIGNED(searchin+4);
		if ((int32)size <= 0) {
			error("(%c%c%c%c) Not found in %d... illegal block len %d", 
				tag&0xFF,(tag>>8)&0xFF,(tag>>16)&0xFF,(tag>>24)&0xFF,
				0,
				size);
			return NULL;
		}

		curpos += size;
		searchin += size;
	} 

	return NULL;
}

void Scumm::lock(int type, int i) {
	validateResource("Locking", type, i);
	res.flags[type][i] |= 0x80;

//	debug(1, "locking %d,%d", type, i);
}

void Scumm::unlock(int type, int i) {
	validateResource("Unlocking", type, i);
	res.flags[type][i] &= ~0x80;

//	debug(1, "unlocking %d,%d", type, i);
}

bool Scumm::isResourceInUse(int type, int i) {
	validateResource("isResourceInUse", type, i);
	switch(type) {
	case rtRoom:
		return _roomResource == (byte)i;
	case rtScript:
		return isScriptInUse(i);
	case rtCostume:
		return isCostumeInUse(i);
	case rtSound:
		return isSoundRunning(i)!=0;
	default:
		return false;
	}
}

void Scumm::increaseResourceCounter() {
	int i,j;
	byte counter;

	for (i=rtFirst; i<=rtLast; i++) {
		for(j=res.num[i]; --j>=0;) {
			counter = res.flags[i][j] & 0x7F;
			if (counter && counter < 0x7F) {
				setResourceCounter(i, j, counter+1);
			}
		}
	}
}

void Scumm::expireResources(uint32 size) {
	int i,j;
	byte flag;
	byte best_counter;
	int best_type, best_res;
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

		for (i=rtFirst; i<=rtLast; i++)
			if (res.mode[i]) {
				for(j=res.num[i]; --j>=0;) {
					flag = res.flags[i][j];
					if (!(flag&0x80) && flag >= best_counter && !isResourceInUse(i,j)) {
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

	debug(1, "Expired resources, mem %d -> %d", oldAllocatedSize, _allocatedSize);
}

void Scumm::freeResources() {
	int i,j;
	for (i=rtFirst; i<=rtLast; i++) {
		for(j=res.num[i]; --j>=0;) {
			if (isResourceLoaded(i,j))
				nukeResource(i,j);
		}
		free(res.address[i]);
		free(res.flags[i]);
		free(res.roomno[i]);
		free(res.roomoffs[i]);
	}
}

void Scumm::loadPtrToResource(int type, int resindex, byte *source) {
	byte *ptr, *alloced;
	int i,len;

	nukeResource(type, resindex);

	len = getStringLen(source);

	if (len <= 0)
		return;

	alloced = createResource(type, resindex, len);

	if (!source) {
		alloced[0] = fetchScriptByte();
		for (i=1; i<len; i++)
			alloced[i] = *_scriptPointer++;
	} else {
		for(i=0; i<len; i++)
			alloced[i] = source[i];
	}
}

bool Scumm::isResourceLoaded(int type, int index) {
	validateResource("isLoaded", type, index);
	return res.address[type][index] != NULL;
}

void Scumm::resourceStats() {
	int i,j;
	uint32 lockedSize = 0, lockedNum = 0;
	byte flag;
	
	for (i=rtFirst; i<=rtLast; i++)
		for(j=res.num[i]; --j>=0;) {
			flag = res.flags[i][j];
			if (flag&0x80 && res.address[i][j]) {
				lockedSize += ((ResHeader*)res.address[i][j])->size;
				lockedNum++;
			}
		}
	
	printf("Total allocated size=%d, locked=%d(%d)\n", _allocatedSize, lockedSize, lockedNum);
}

void Scumm::heapClear(int mode) {
	/* TODO: implement this */
	warning("heapClear: not implemented");
}

void Scumm::unkHeapProc2(int a, int b) {
	warning("unkHeapProc2: not implemented");
} 

void Scumm::loadFlObject(int a, int b) {
	warning("loadFlObject(%d,%d):not implemented", a, b);
}

void Scumm::readMAXS() {
	_numVariables = fileReadWordLE();
	fileReadWordLE();
	_numBitVariables = fileReadWordLE();
	_numLocalObjects = fileReadWordLE();
	_numArray = fileReadWordLE();
	fileReadWordLE();
	_numVerbs = fileReadWordLE();
	_numFlObject = fileReadWordLE();
	_numInventory = fileReadWordLE();
	_numRooms = fileReadWordLE();
	_numScripts = fileReadWordLE();
	_numSounds = fileReadWordLE();
	_numCharsets = fileReadWordLE();
	_numCostumes = fileReadWordLE();
	_numGlobalObjects = fileReadWordLE();

	allocResTypeData(rtCostume, MKID('COST'), _numCostumes, "costume", 1);
	allocResTypeData(rtRoom, MKID('ROOM'), _numRooms, "room", 1);
	allocResTypeData(rtSound, MKID('SOUN'), _numSounds, "sound", 1);
	allocResTypeData(rtScript, MKID('SCRP'), _numScripts, "script", 1);
	allocResTypeData(rtCharset, MKID('CHAR'), _numCharsets, "charset", 1);
	allocResTypeData(rtObjectName, MKID('NONE'),50,"new name", 0);
	
	allocateArrays();

	_objectFlagTable = (byte*)alloc(_numGlobalObjects);
	_arrays = (byte*)alloc(_numArray);
	_newNames = (uint16*)alloc(50 * sizeof(uint16));
	_classData = (uint32*)alloc(_numGlobalObjects * sizeof(uint32));

	_numGlobalScripts = 200;
	_dynamicRoomOffsets = 1;
}

void Scumm::allocateArrays() {
	_inventory = (uint16*)alloc(_numInventory * sizeof(uint16));
	_verbs = (VerbSlot*)alloc(_numVerbs * sizeof(VerbSlot));
	_objs = (ObjectData*)alloc(_numLocalObjects * sizeof(ObjectData));
	_vars = (int16*)alloc(_numVariables * sizeof(int16));
	_bitVars = (byte*)alloc(_numBitVariables >> 3);

	allocResTypeData(rtInventory, MKID('NONE'),	_numInventory, "inventory", 0);
	allocResTypeData(rtTemp,MKID('NONE'),10, "temp", 0);
	allocResTypeData(rtScaleTable,MKID('NONE'),5, "scale table", 0);
	allocResTypeData(rtActorName, MKID('NONE'),13,"actor name", 0);
	allocResTypeData(rtBuffer, MKID('NONE'),10,"buffer", 0);
 	allocResTypeData(rtVerb, MKID('NONE'),_numVerbs,"verb", 0);
	allocResTypeData(rtString, MKID('NONE'),_numArray,"array", 0);
	allocResTypeData(rtFlObject, MKID('NONE'),_numFlObject,"flobject", 0);
	allocResTypeData(rtMatrix, MKID('NONE'),10,"boxes", 0);
}
