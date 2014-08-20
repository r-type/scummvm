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

#ifndef ACCESS_EVENTS_H
#define ACCESS_EVENTS_H

#include "common/scummsys.h"
#include "common/events.h"
#include "common/stack.h"

namespace Access {

enum CursorType { 
	CURSOR_NONE = -1,
	CURSOR_ARROW = 0, CURSOR_CROSSHAIRS, CURSOR_2, CURSOR_3, CURSOR_LOOK, 
	CURSOR_USE, CURSOR_TAKE, CURSOR_CLIMB, CURSOR_TALK, CURSOR_HELP
};

#define GAME_FRAME_RATE 100
#define GAME_FRAME_TIME (1000 / GAME_FRAME_RATE)

class AccessEngine;

class EventsManager {
private:
	AccessEngine *_vm;
	uint32 _frameCounter;
	uint32 _priorFrameTime;

	bool checkForNextFrameCounter();

	void nextFrame();
public:
	CursorType _cursorId;
	CursorType _normalMouse;
	bool _leftButton, _rightButton;
	Common::Point _mousePos;
	int _mouseCol, _mouseRow;
	int _mouseMode;
	bool _cursorExitFlag;
	Common::FixedStack<Common::KeyState> _keypresses;
public:
	/**
	 * Constructor
	 */
	EventsManager(AccessEngine *vm);

	/**
	 * Destructor
	 */
	~EventsManager();

	/**
	 * Sets the cursor
	 */
	void setCursor(CursorType cursorId);

	/**
	 * Return the current cursor Id
	 */
	CursorType getCursor() const { return _cursorId; }

	/**
	 * Show the mouse cursor
	 */
	void showCursor();

	/**
	 * Hide the mouse cursor
	 */
	void hideCursor();

	/**
	 * Returns if the mouse cursor is visible
	 */
	bool isCursorVisible();

	void pollEvents();

	void zeroKeys();

	bool getKey(Common::KeyState &key);

	void delay(int time);

	void debounceLeft();

	void waitKeyMouse();
};

} // End of namespace Access

#endif /* ACCESS_EVENTS_H */
