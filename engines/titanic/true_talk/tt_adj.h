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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef TITANIC_TT_ADJ_H
#define TITANIC_TT_ADJ_H

#include "titanic/true_talk/tt_major_word.h"

namespace Titanic {

class TTadj : public TTmajorWord {
private:
	static bool _staticFlag;
protected:
	int _field30;
public:
	TTadj(TTstring &str, int val1, int val2, int val3, int val4);
	TTadj(TTadj *src);

	/**
	 * Load the word
	 */
	int load(SimpleFile *file);

	/**
	 * Creates a copy of the word
	 */
	virtual TTword *copy();

	virtual bool proc14(int val) const { return _field30 == val; }
	virtual int proc15() const { return _field30; }
	virtual bool proc16() const { return _field30 >= 7; }
	virtual bool proc17() const { return _field30 <= 3; }
	virtual bool proc18() const { return _field30 > 3 && _field30 < 7; }

	/**
	 * Dumps data associated with the word to file
	 */
	virtual int save(SimpleFile *file) const {
		return saveData(file, _field30);
	}
};

} // End of namespace Titanic

#endif /* TITANIC_TT_ADJ_H */
