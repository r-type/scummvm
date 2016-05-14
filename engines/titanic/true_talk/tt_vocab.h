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

#ifndef TITANIC_ST_VOCAB_H
#define TITANIC_ST_VOCAB_H

#include "titanic/support/string.h"
#include "titanic/true_talk/tt_string.h"
#include "titanic/true_talk/tt_word.h"

namespace Titanic {

class TTvocab {
private:
	TTword *_pHead;
	TTword *_pTail;
	TTword *_word;
	int _fieldC;
	int _field10;
	int _field14;
	int _field18;
private:
	/**
	 * Load the vocab data
	 */
	int load(const CString &name);

	/**
	 * Adds a specified word to the vocab list
	 */
	void addWord(TTword *word);

	/**
	 * Scans the vocab list for an existing word match
	 */
	TTword *findWord(const TTstring &str);
public:
	TTvocab(int val);
	~TTvocab();

	TTword *getPrimeWord(TTstring &str, TTword **words);
};

} // End of namespace Titanic

#endif /* TITANIC_ST_VOCAB_H */
