#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "CodeEditor.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>


template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1,
	InputIt2 first2, InputIt2 last2, BinaryPredicate p) {
	for (; first1 != last1 && first2 != last2; ++first1, ++first2)
	{
		if (!p(*first1, *first2))
			return false;
	}
	return first1 == last1 && first2 == last2;
}


CodeEditor::CodeEditor()
	: mLineSpacing(1.0f)
	, mUndoIndex(0)
	, mTabSize(4)
	, mOverwrite(false)
	, mReadOnly(false)
	, mWithinRender(false)
	, mScrollToCursor(false)
	, mScrollToTop(false)
	, mTextChanged(false)
	, mColorizerEnabled(true)
	, mTextStart(20.0f)
	, mLeftMargin(10)
	, mCursorPositionChanged(false)
	, mColorRangeMin(0)
	, mColorRangeMax(0)
	, mSelectionMode(SelectionMode::Normal)
	, mCheckComments(true)
	, mLastClick(-1.0f)
	, mHandleKeyboardInputs(true)
	, mHandleMouseInputs(true)
	, mIgnoreImGuiChild(false)
	, mShowWhitespaces(true)
	, mStartTime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{
	SetPalette(GetDarkPalette());
	SetLanguageDefinition(LanguageDefinition::YAML());
	mLines.push_back(Line());
}


void CodeEditor::SetLanguageDefinition(const LanguageDefinition &aLanguageDef) {
	mLanguageDefinition = aLanguageDef;
	mRegexList.clear();

	for (auto &r : mLanguageDefinition.mTokenRegexStrings)
		mRegexList.push_back(std::make_pair(std::regex(r.first, std::regex_constants::optimize), r.second));

	Colorize();
}


void CodeEditor::SetPalette(const Palette &aValue) {
	mPaletteBase = aValue;
}


std::string CodeEditor::GetText(const Coordinates &aStart, const Coordinates &aEnd) const {
	std::string result;

	auto lstart = aStart.mLine;
	auto lend = aEnd.mLine;
	auto istart = GetCharacterIndex(aStart);
	auto iend = GetCharacterIndex(aEnd);
	size_t s = 0;

	for (size_t i = lstart; i < lend; i++)
		s += mLines[i].size();

	result.reserve(s + s / 8);


	for (size_t ln = lstart; ln <= lend; ln++) {
		if (ln >= mLines.size())
			break;

		auto &line = mLines[ln];
		for (size_t i = istart; i < line.size(); i++)
			if (ln < lend || (ln == lend && i < iend))
				result += line[i].mChar;
		istart = 0;

		if (ln < lend)
			result += '\n';
	}

	/*
	while (istart < iend || lstart < lend)
	{
		if (lstart >= (int)mLines.size())
			break;

		auto &line = mLines[lstart];
		if (istart < (int)line.size())
		{
			result += line[istart].mChar;
			istart++;
		}
		else
		{
			if (lstart < lend) {
				// Only add newline if the current line has not caught up to the end line (to avoid extra trailing '\n' that would increase the file's line count by 1 on each `GetText`)
				result += '\n';

				istart = 0;
				lstart++;
			}
		}
	}
	*/

	return result;
}


CodeEditor::Coordinates CodeEditor::GetActualCursorCoordinates() const {
	return SanitizeCoordinates(mState.mCursorPosition);
}


CodeEditor::Coordinates CodeEditor::SanitizeCoordinates(const Coordinates &aValue) const {
	auto line = aValue.mLine;
	auto column = aValue.mColumn;
	if (line >= (int)mLines.size())
	{
		if (mLines.empty())
		{
			line = 0;
			column = 0;
		}
		else
		{
			line = (int)mLines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	}
	else
	{
		column = mLines.empty() ? 0 : std::min(column, GetLineMaxColumn(line));
		return Coordinates(line, column);
	}
}


// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(CodeEditor::Char c) {
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}


// "Borrowed" from ImGui source
static inline int ImTextCharToUtf8(char *buf, int buf_size, unsigned int c) {
	if (c < 0x80)
	{
		buf[0] = (char)c;
		return 1;
	}
	if (c < 0x800)
	{
		if (buf_size < 2) return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000)
	{
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00)
	{
		if (buf_size < 4) return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));
		return 4;
	}
	//else if (c < 0x10000)
	{
		if (buf_size < 3) return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));
		return 3;
	}
}


void CodeEditor::Advance(Coordinates &aCoordinates) const {
	if (aCoordinates.mLine < (int)mLines.size())
	{
		auto &line = mLines[aCoordinates.mLine];
		auto cindex = GetCharacterIndex(aCoordinates);

		if (cindex + 1 < (int)line.size())
		{
			auto delta = UTF8CharLength(line[cindex].mChar);
			cindex = std::min(cindex + delta, (int)line.size() - 1);
		}
		else
		{
			++aCoordinates.mLine;
			cindex = 0;
		}
		aCoordinates.mColumn = GetCharacterColumn(aCoordinates.mLine, cindex);
	}
}


void CodeEditor::DeleteRange(const Coordinates &aStart, const Coordinates &aEnd) {
	assert(aEnd >= aStart);
	assert(!mReadOnly);

	//printf("D(%d.%d)-(%d.%d)\n", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);

	if (aEnd == aStart)
		return;

	auto start = GetCharacterIndex(aStart);
	auto end = GetCharacterIndex(aEnd);

	if (aStart.mLine == aEnd.mLine)
	{
		auto &line = mLines[aStart.mLine];
		auto n = GetLineMaxColumn(aStart.mLine);
		if (aEnd.mColumn >= n)
			line.erase(line.begin() + start, line.end());
		else
			line.erase(line.begin() + start, line.begin() + end);
	}
	else
	{
		auto &firstLine = mLines[aStart.mLine];
		auto &lastLine = mLines[aEnd.mLine];

		firstLine.erase(firstLine.begin() + start, firstLine.end());
		lastLine.erase(lastLine.begin(), lastLine.begin() + end);

		if (aStart.mLine < aEnd.mLine)
			firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

		if (aStart.mLine < aEnd.mLine)
			RemoveLine(aStart.mLine + 1, aEnd.mLine + 1);
	}

	mTextChanged = true;
}


int CodeEditor::InsertTextAt(Coordinates & /* inout */ aWhere, const char *aValue) {
	assert(!mReadOnly);

	int cindex = GetCharacterIndex(aWhere);
	int totalLines = 0;
	while (*aValue != '\0')
	{
		assert(!mLines.empty());

		if (*aValue == '\r')
		{
			// skip
			++aValue;
		}
		else if (*aValue == '\n')
		{
			if (cindex < (int)mLines[aWhere.mLine].size())
			{
				auto &newLine = InsertLine(aWhere.mLine + 1);
				auto &line = mLines[aWhere.mLine];
				newLine.insert(newLine.begin(), line.begin() + cindex, line.end());
				line.erase(line.begin() + cindex, line.end());
			}
			else
			{
				InsertLine(aWhere.mLine + 1);
			}
			++aWhere.mLine;
			aWhere.mColumn = 0;
			cindex = 0;
			++totalLines;
			++aValue;
		}
		else
		{
			auto &line = mLines[aWhere.mLine];
			auto d = UTF8CharLength(*aValue);
			while (d-- > 0 && *aValue != '\0')
				line.insert(line.begin() + cindex++, Glyph(*aValue++, PaletteIndex::Default));
			++aWhere.mColumn;
		}

		mTextChanged = true;
	}

	return totalLines;
}


void CodeEditor::AddUndo(UndoRecord &aValue) {
	assert(!mReadOnly);
	//printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
	//	aValue.mBefore.mCursorPosition.mLine, aValue.mBefore.mCursorPosition.mColumn,
	//	aValue.mAdded.c_str(), aValue.mAddedStart.mLine, aValue.mAddedStart.mColumn, aValue.mAddedEnd.mLine, aValue.mAddedEnd.mColumn,
	//	aValue.mRemoved.c_str(), aValue.mRemovedStart.mLine, aValue.mRemovedStart.mColumn, aValue.mRemovedEnd.mLine, aValue.mRemovedEnd.mColumn,
	//	aValue.mAfter.mCursorPosition.mLine, aValue.mAfter.mCursorPosition.mColumn
	//	);

	mUndoBuffer.resize((size_t)(mUndoIndex + 1));
	mUndoBuffer.back() = aValue;
	++mUndoIndex;
}


CodeEditor::Coordinates CodeEditor::ScreenPosToCoordinates(const ImVec2 &aPosition) const {
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

	int lineNo = std::max(0, (int)floor(local.y / mCharAdvance.y));

	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < (int)mLines.size())
	{
		auto &line = mLines.at(lineNo);

		int columnIndex = 0;
		float columnX = 0.0f;

		while ((size_t)columnIndex < line.size())
		{
			float columnWidth = 0.0f;

			if (line[columnIndex].mChar == '\t')
			{
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + std::floor((1.0f + columnX) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
				columnWidth = newColumnX - oldX;
				if (mTextStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX = newColumnX;
				columnCoord = (columnCoord / mTabSize) * mTabSize + mTabSize;
				columnIndex++;
			}
			else
			{
				char buf[7];
				auto d = UTF8CharLength(line[columnIndex].mChar);
				int i = 0;
				while (i < 6 && d-- > 0)
					buf[i++] = line[columnIndex++].mChar;
				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
				if (mTextStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX += columnWidth;
				columnCoord++;
			}
		}
	}

	return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}


CodeEditor::Coordinates CodeEditor::FindWordStart(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto &line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	while (cindex > 0 && isspace(line[cindex].mChar))
		--cindex;

	auto cstart = (PaletteIndex)line[cindex].mColorIndex;
	while (cindex > 0)
	{
		auto c = line[cindex].mChar;
		if ((c & 0xC0) != 0x80)	// not UTF code sequence 10xxxxxx
		{
			if (c <= 32 && isspace(c))
			{
				cindex++;
				break;
			}
			if (cstart != (PaletteIndex)line[size_t(cindex - 1)].mColorIndex)
				break;
		}
		--cindex;
	}
	return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));
}


CodeEditor::Coordinates CodeEditor::FindWordEnd(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto &line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	bool prevspace = (bool)isspace(line[cindex].mChar);
	auto cstart = (PaletteIndex)line[cindex].mColorIndex;
	while (cindex < (int)line.size())
	{
		auto c = line[cindex].mChar;
		auto d = UTF8CharLength(c);
		if (cstart != (PaletteIndex)line[cindex].mColorIndex)
			break;

		if (prevspace != !!isspace(c))
		{
			if (isspace(c))
				while (cindex < (int)line.size() && isspace(line[cindex].mChar))
					++cindex;
			break;
		}
		cindex += d;
	}
	return Coordinates(aFrom.mLine, GetCharacterColumn(aFrom.mLine, cindex));
}


CodeEditor::Coordinates CodeEditor::FindNextWord(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	// skip to the next non-word character
	auto cindex = GetCharacterIndex(aFrom);
	bool isword = false;
	bool skip = false;
	if (cindex < (int)mLines[at.mLine].size())
	{
		auto &line = mLines[at.mLine];
		isword = isalnum(line[cindex].mChar);
		skip = isword;
	}

	while (!isword || skip)
	{
		if (at.mLine >= mLines.size())
		{
			auto l = std::max(0, (int)mLines.size() - 1);
			return Coordinates(l, GetLineMaxColumn(l));
		}

		auto &line = mLines[at.mLine];
		if (cindex < (int)line.size())
		{
			isword = isalnum(line[cindex].mChar);

			if (isword && !skip)
				return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));

			if (!isword)
				skip = false;

			cindex++;
		}
		else
		{
			cindex = 0;
			++at.mLine;
			skip = false;
			isword = false;
		}
	}

	return at;
}


int CodeEditor::GetCharacterIndex(const Coordinates &aCoordinates) const {
	if (aCoordinates.mLine >= mLines.size())
		return -1;
	auto &line = mLines[aCoordinates.mLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoordinates.mColumn;)
	{
		if (line[i].mChar == '\t')
			c = (c / mTabSize) * mTabSize + mTabSize;
		else
			++c;
		i += UTF8CharLength(line[i].mChar);
	}
	return i;
}


int CodeEditor::GetCharacterColumn(int aLine, int aIndex) const {
	if (aLine >= mLines.size())
		return 0;
	auto &line = mLines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i < (int)line.size())
	{
		auto c = line[i].mChar;
		i += UTF8CharLength(c);
		if (c == '\t')
			col = (col / mTabSize) * mTabSize + mTabSize;
		else
			col++;
	}
	return col;
}


int CodeEditor::GetLineCharacterCount(int aLine) const {
	if (aLine >= mLines.size())
		return 0;
	auto &line = mLines[aLine];
	int c = 0;
	for (unsigned i = 0; i < line.size(); c++)
		i += UTF8CharLength(line[i].mChar);
	return c;
}


int CodeEditor::GetLineMaxColumn(int aLine) const {
	if (aLine >= mLines.size())
		return 0;
	auto &line = mLines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size(); )
	{
		auto c = line[i].mChar;
		if (c == '\t')
			col = (col / mTabSize) * mTabSize + mTabSize;
		else
			col++;
		i += UTF8CharLength(c);
	}
	return col;
}


bool CodeEditor::IsOnWordBoundary(const Coordinates &aAt) const {
	if (aAt.mLine >= (int)mLines.size() || aAt.mColumn == 0)
		return true;

	auto &line = mLines[aAt.mLine];
	auto cindex = GetCharacterIndex(aAt);
	if (cindex >= (int)line.size())
		return true;

	if (mColorizerEnabled)
		return line[cindex].mColorIndex != line[size_t(cindex - 1)].mColorIndex;

	return isspace(line[cindex].mChar) != isspace(line[cindex - 1].mChar);
}


void CodeEditor::RemoveLine(int aStart, int aEnd) {
	assert(!mReadOnly);
	assert(aEnd >= aStart);
	assert(mLines.size() > (size_t)(aEnd - aStart));

	ErrorMarkers etmp;
	for (auto &i : mErrorMarkers)
	{
		ErrorMarkers::value_type e(i.first >= aStart ? i.first - 1 : i.first, i.second);
		if (e.first >= aStart && e.first <= aEnd)
			continue;
		etmp.insert(e);
	}
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : mBreakpoints)
	{
		if (i >= aStart && i <= aEnd)
			continue;
		btmp.insert(i >= aStart ? i - 1 : i);
	}
	mBreakpoints = std::move(btmp);

	mLines.erase(mLines.begin() + aStart, mLines.begin() + aEnd);
	assert(!mLines.empty());

	mTextChanged = true;
}


void CodeEditor::RemoveLine(int aIndex) {
	assert(!mReadOnly);
	assert(mLines.size() > 1);

	ErrorMarkers etmp;
	for (auto &i : mErrorMarkers)
	{
		ErrorMarkers::value_type e(i.first > aIndex ? i.first - 1 : i.first, i.second);
		if (e.first - 1 == aIndex)
			continue;
		etmp.insert(e);
	}
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : mBreakpoints)
	{
		if (i == aIndex)
			continue;
		btmp.insert(i >= aIndex ? i - 1 : i);
	}
	mBreakpoints = std::move(btmp);

	mLines.erase(mLines.begin() + aIndex);
	assert(!mLines.empty());

	mTextChanged = true;
}


CodeEditor::Line &CodeEditor::InsertLine(int aIndex) {
	assert(!mReadOnly);

	auto &result = *mLines.insert(mLines.begin() + aIndex, Line());

	ErrorMarkers etmp;
	for (auto &i : mErrorMarkers)
		etmp.insert(ErrorMarkers::value_type(i.first >= aIndex ? i.first + 1 : i.first, i.second));
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : mBreakpoints)
		btmp.insert(i >= aIndex ? i + 1 : i);
	mBreakpoints = std::move(btmp);

	return result;
}


std::string CodeEditor::GetWordUnderCursor() const {
	auto c = GetCursorPosition();
	return GetWordAt(c);
}


std::string CodeEditor::GetWordAt(const Coordinates &aCoords) const {
	auto start = FindWordStart(aCoords);
	auto end = FindWordEnd(aCoords);

	std::string r;

	auto istart = GetCharacterIndex(start);
	auto iend = GetCharacterIndex(end);

	for (auto it = istart; it < iend; ++it)
		r.push_back(mLines[aCoords.mLine][it].mChar);

	return r;
}


ImU32 CodeEditor::GetGlyphColor(const Glyph &aGlyph) const {
	if (!mColorizerEnabled)
		return mPalette[(int)PaletteIndex::Default];
	if (aGlyph.mComment)
		return mPalette[(int)PaletteIndex::Comment];
	if (aGlyph.mMultiLineComment)
		return mPalette[(int)PaletteIndex::MultiLineComment];
	auto const color = mPalette[(int)aGlyph.mColorIndex];
	if (aGlyph.mPreprocessor)
	{
		const auto ppcolor = mPalette[(int)PaletteIndex::Preprocessor];
		const int c0 = ((ppcolor & 0xff) + (color & 0xff)) / 2;
		const int c1 = (((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
		const int c2 = (((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
		const int c3 = (((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
		return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
	}
	return color;
}


std::pair<CodeEditor::Coordinates, CodeEditor::Coordinates> CodeEditor::GetDocStartEndCoords() const {
	Coordinates docStart(0, 0);
	Coordinates docEnd(static_cast<int>(mLines.size()) - 1, GetLineMaxColumn(static_cast<int>(mLines.size()) - 1));

	return { docStart, docEnd };
}


void CodeEditor::HandleKeyboardInputs() {
	ImGuiIO &io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused()) {
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

		io.WantCaptureKeyboard = true;

		// ----- COMMON KEYBOARD OPERATIONS -----
		if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))			// Up
			MoveUp(1, shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))			// Down
			MoveDown(1, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))					// Left
			MoveLeft(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow))					// Right
			MoveRight(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_PageUp))						// Page Up
			MoveUp(GetPageSize() - 4, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_PageDown))					// Page Down
			MoveDown(GetPageSize() - 4, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGuiKey_Home))				// Ctrl + Home
			MoveTop(shift);
		else if (ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_End))					// Ctrl + End
			MoveBottom(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_Home))				// Home
			MoveHome(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_End))				// End
			MoveEnd(shift);

		else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Insert))	// Insert
			mOverwrite ^= true;
		//else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Insert))	// Ctrl + Insert
		//	Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_C))			// Ctrl + C
			Copy();

		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_X))			// Ctrl + X
			Cut();
		//else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Delete))	// Shift + Delete
		//	Cut();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_A))			// Ctrl + A
			SelectAll();

		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_F))			// Ctrl + F
			SetShowFindReplaceWindow(!mShowFindReplaceWindow);


		// ----- KEYBOARD OPERATIONS FOR EDITABLE FILES -----
		if (!IsReadOnly()) {
			if		(ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Z))			// Ctrl + Z: Undo
				Undo();
			//else if (!ctrl && !shift && alt && ImGui::IsKeyPressed(ImGuiKey_Backspace))	// Alt + Backspace: Undo
			//	Undo();
			else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Z))			// Ctrl + Shift + Z: Redo
				Redo();

			else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Delete))	// Delete: Delete character after cursor/selection
				Delete();
			else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Backspace))// Backspace: Delete character before cursor/selection
				Backspace();

			//else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Insert))	// Shift + Insert: Paste
			//	Paste();
			else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_V))			// Ctrl + V: Paste
				Paste();

			else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Enter))	// Enter: Newline
				EnterCharacter('\n', false);

			else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
				if (!ctrl && shift && !alt) {											// Shift + Tab: Unindent
					if (mLanguageDefinition.mName == "YAML")
						UnindentSpaces();
				}
				else if (!ctrl && !shift && !alt) {										// Tab: Indent
					if (mLanguageDefinition.mName == "YAML")
						IndentSpaces();
					else
						EnterCharacter('\t', shift);
				}
			}

			else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Slash))		// Ctrl + /
				HandleCommentInline();


			if (!io.InputQueueCharacters.empty()) {
				if (mFindReplaceWindowFocused)
					// Let Find & Replace capture input if its window is focused
					return;

				for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
					auto c = io.InputQueueCharacters[i];
					if (c != 0 && (c == '\n' || c >= 32))
						EnterCharacter(c, shift);
				}
				//io.InputQueueCharacters.resize(0);
			}
		}
	}
}


void CodeEditor::IndentSpaces() {
	// Languages like YAML do not allow tabs for indentation, so we add 4 spaces instead, and treat them as an atomic UndoRecord, not 4 separate ones
		// Create string with mTabSize spaces; update if mTabSize is changed
	static int currentTabSize = mTabSize;
	static std::string spaces(mTabSize, ' ');
	if (currentTabSize != mTabSize) {
		currentTabSize = mTabSize;
		spaces = std::string(mTabSize, ' ');
	}


	auto cursorCoords = GetActualCursorCoordinates();

	auto realStart = mState.mSelectionStart;
	auto realEnd = mState.mSelectionEnd;
	auto visualStart = realStart;
	auto visualEnd = realEnd;


	UndoRecord u{};
	u.mBefore = mState;

	if (HasSelection() && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine) {
		// If multiline selection: we add 4 spaces to each line at Column 0
		
		// if the cursor at the end line of the selection is at Column 0, we ignore that line and move back to the previous line
		realStart.mColumn = 0;
		if (realEnd.mColumn == 0 && realEnd.mLine > 0)
			realEnd.mLine--;
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);


		// STATE SNAPSHOT: pre-indent
		u.mRemovedStart = realStart;
		u.mRemovedEnd = realEnd;
		u.mRemoved = GetText(realStart, realEnd);


		for (int line = realStart.mLine; line <= realEnd.mLine; line++) {
			Coordinates lineStart(line, 0);
			InsertTextAt(lineStart, spaces.c_str());
		}


		// Similarly to most code/text editors, we just shift the visual cursor by 4 spaces as opposed to the real cursor, which is at max. column for that line
		visualStart.mColumn				+= spaces.size();
		visualEnd.mColumn				+= spaces.size();
		cursorCoords.mColumn			+= spaces.size();


		// STATE SNAPSHOT: post-indent
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);
		u.mAddedStart = realStart;
		u.mAddedEnd = realEnd;
		u.mAdded = GetText(realStart, realEnd);


		// Update visuals
		SetSelection(visualStart, visualEnd);
		SetCursorPosition(cursorCoords);
	}
	else {
		// If no multiline selection: we insert 4 spaces at the cursor, replacing any selection on that line with it if applicable
		if (HasSelection()) {
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = realStart;
			u.mRemovedEnd = realEnd;
			DeleteSelection();
		}

		u.mAddedStart = cursorCoords;
			InsertTextAt(cursorCoords, spaces.c_str());
		u.mAddedEnd = cursorCoords;			// NOTE: InsertTextAt updates cursor coordinates in-place
		u.mAdded = spaces;

		// Update visuals
		SetCursorPosition(cursorCoords);
	}

	u.mAfter = mState;
	AddUndo(u);

	mTextChanged = true;
	Colorize(cursorCoords.mLine - 1, 3);
	EnsureCursorVisible();
}


void CodeEditor::UnindentSpaces() {
	auto cursorCoords = GetActualCursorCoordinates();

	auto realStart = mState.mSelectionStart;
	auto realEnd = mState.mSelectionEnd;
	auto visualStart = realStart;
	auto visualEnd = realEnd;

	UndoRecord u;
	u.mBefore = mState;

	if (HasSelection() && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine) {
		// If multiline selection: for each line, delete spaces in Columns [0..max(tabSize, maxLineColBeforeNonSpaceChar)]

		// if the cursor at the end line of the selection is at Column 0, we ignore that line and move back to the previous line
		realStart.mColumn = 0;
		if (realEnd.mColumn == 0 && realEnd.mLine > 0)
			realEnd.mLine--;
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);


		// STATE SNAPSHOT: pre-unindent
		u.mRemovedStart = realStart;
		u.mRemovedEnd = realEnd;
		u.mRemoved = GetText(realStart, realEnd);


		// For each line: count spaces available for removal, starting from Column 0
		for (int i = realStart.mLine; i <= realEnd.mLine; i++) {
			auto &line = mLines[i];
			
			int leadingSpaces = 0;
			for (int j = 0; j < std::min(mTabSize, static_cast<int>(line.size())); j++, leadingSpaces++)
				if (line[j].mChar != ' ')
					break;

			if (leadingSpaces == 0)
				continue;

			Coordinates removeStart(i, 0);
			Coordinates removeEnd(i, leadingSpaces);

			DeleteRange(removeStart, removeEnd);

			if (i == cursorCoords.mLine)	cursorCoords.mColumn	= std::max(0, cursorCoords.mColumn - leadingSpaces);
			if (i == realStart.mLine)		visualStart.mColumn		= std::max(0, visualStart.mColumn - leadingSpaces);
			if (i == realEnd.mLine)			visualEnd.mColumn		= std::max(0, visualEnd.mColumn - leadingSpaces);
		}


		// STATE SNAPSHOT: post-unindent
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);
		u.mAddedStart = realStart;
		u.mAddedEnd = realEnd;
		u.mAdded = GetText(realStart, realEnd);


		// Update visuals
		SetSelection(visualStart, visualEnd);
		SetCursorPosition(cursorCoords);
	}
	else {
		// If not multiline selection, delete spaces before the cursor, in Columns [max(0, nonSpaceCharCol+1, cursorCol-tabSize)..cursorCol-1]

		// Count spaces before the cursor
		auto &line = mLines[cursorCoords.mLine];
		int cursorLineIdx = GetCharacterIndex(cursorCoords);
		int leadingSpaces = 0;
		for (int i = cursorLineIdx - 1; i >= 0 && i >= (cursorLineIdx - mTabSize); i--, leadingSpaces++)
			if (line[i].mChar != ' ')
				break;

		if (leadingSpaces == 0)
			return;


		// Delete spaces & update visuals
		int maxColDelta = std::max(0, cursorCoords.mColumn - leadingSpaces);
		Coordinates deleteStart(cursorCoords.mLine, maxColDelta);

		u.mRemovedStart = deleteStart;
		u.mRemovedEnd = cursorCoords;
		u.mRemoved = GetText(deleteStart, cursorCoords);

		DeleteRange(deleteStart, cursorCoords);

		cursorCoords.mColumn = maxColDelta;
		SetCursorPosition(cursorCoords);
	}


	u.mAfter = mState;
	AddUndo(u);

	mTextChanged = true;
	Colorize(cursorCoords.mLine - 1, 3);
	EnsureCursorVisible();
}


void CodeEditor::HandleCommentInline() {
	// Comment or uncomment, depending on whether every line in the selection (or single line) has the comment mark at its first few Columns (assume no comment blocks)
	
	bool hasMarks = false;		// Do the selected line(s) have comment marks? This bool only answers this, and does not guarantee consistency (i.e., either ALL are commented out or ALL are not)
	bool inconsistent = false;  // Inconsistent: some lines have comment marks at the start, others don't
	{
		const std::string &mark = mLanguageDefinition.mSingleLineComment;
		auto start = mState.mSelectionStart;
		auto end = mState.mSelectionEnd;
		if (end.mColumn == 0 && end.mLine > 0 && start.mLine != end.mLine)
			end.mLine--;

		bool prevLineHasMark = false;

		std::string firstXChars{};
		for (int line = start.mLine; line <= end.mLine; line++) {
			firstXChars = GetText(Coordinates(line, 0), SanitizeCoordinates({ line, static_cast<int>(mark.size()) }));
			bool lineHasMark = (firstXChars == mLanguageDefinition.mSingleLineComment);

			firstXChars.clear();

			if (lineHasMark)
				hasMarks = true;

			inconsistent = (line > start.mLine && lineHasMark != prevLineHasMark);
			if (inconsistent)
				break;

			prevLineHasMark = lineHasMark;
		}
	}


	if (!hasMarks)
		CommentInline();		// Comment if one or multiple lines have no comment mark; consistency does not matter
	else if (!inconsistent)
		UncommentInline();		// Uncomment ONLY if ALL lines have comment marks
}


void CodeEditor::CommentInline() {
	const std::string &mark = mLanguageDefinition.mSingleLineComment;

	auto cursorCoords = GetActualCursorCoordinates();

	auto realStart = mState.mSelectionStart;
	auto realEnd = mState.mSelectionEnd;
	auto visualStart = realStart;
	auto visualEnd = realEnd;


	UndoRecord u{};
	u.mBefore = mState;

	// For each uncommented line: add a comment mark at Column 0
	{
		realStart.mColumn = 0;
		if (realEnd.mColumn == 0 && realEnd.mLine > 0 && realStart.mLine != realEnd.mLine)
			realEnd.mLine--;
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);


		// STATE SNAPSHOT: pre-indent
		u.mRemovedStart = realStart;
		u.mRemovedEnd = realEnd;
		u.mRemoved = GetText(realStart, realEnd);


		std::string firstXChars{};
		for (int line = realStart.mLine; line <= realEnd.mLine; line++) {
			// Skip if line is already commented
			firstXChars.clear();
			firstXChars = GetText(Coordinates(line, 0), SanitizeCoordinates({ line, static_cast<int>(mark.size()) }));
			if (firstXChars == mLanguageDefinition.mSingleLineComment)
				continue;

			// Insert mark
			Coordinates lineStart(line, 0);
			InsertTextAt(lineStart, mark.c_str());
		}


		// Similarly to most code/text editors, we just shift the visual cursor by 4 spaces as opposed to the real cursor, which is at max. column for that line
		visualStart.mColumn		+= mark.size();
		visualEnd.mColumn		+= mark.size();
		cursorCoords.mColumn	+= mark.size();


		// STATE SNAPSHOT: post-indent
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);
		u.mAddedStart = realStart;
		u.mAddedEnd = realEnd;
		u.mAdded = GetText(realStart, realEnd);


		// Update visuals
		SetSelection(visualStart, visualEnd);
		SetCursorPosition(cursorCoords);
	}

	u.mAfter = mState;
	AddUndo(u);

	mTextChanged = true;
	Colorize(cursorCoords.mLine - 1, 3);
	EnsureCursorVisible();
}


void CodeEditor::UncommentInline() {
	const std::string &mark = mLanguageDefinition.mSingleLineComment;

	auto cursorCoords = GetActualCursorCoordinates();

	auto realStart = mState.mSelectionStart;
	auto realEnd = mState.mSelectionEnd;
	auto visualStart = realStart;
	auto visualEnd = realEnd;

	UndoRecord u;
	u.mBefore = mState;

	// For each line: uncomment only if the first comment mark appears within the first few columns
	{
		realStart.mColumn = 0;
		if (realEnd.mColumn == 0 && realEnd.mLine > 0 && realStart.mLine != realEnd.mLine)
			realEnd.mLine--;
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);


		// STATE SNAPSHOT: pre-unindent
		u.mRemovedStart = realStart;
		u.mRemovedEnd = realEnd;
		u.mRemoved = GetText(realStart, realEnd);


		for (int i = realStart.mLine; i <= realEnd.mLine; i++) {
			auto &line = mLines[i];

			if (line.size() < mark.size())
				continue;

			Coordinates removeStart(i, 0);
			Coordinates removeEnd(i, mark.size());

			// Check if the comment mark exists
			std::string firstXChars = GetText(removeStart, removeEnd);
			if (firstXChars != mark)
				continue;
			firstXChars.clear();


			// Delete & update visuals
			DeleteRange(removeStart, removeEnd);

			int markSz = static_cast<int>(mark.size());
			if (i == cursorCoords.mLine)	cursorCoords.mColumn	= std::max(0, cursorCoords.mColumn - markSz);
			if (i == realStart.mLine)		visualStart.mColumn		= std::max(0, visualStart.mColumn - markSz);
			if (i == realEnd.mLine)			visualEnd.mColumn		= std::max(0, visualEnd.mColumn - markSz);
		}


		// STATE SNAPSHOT: post-unindent
		realEnd.mColumn = GetLineMaxColumn(realEnd.mLine);
		u.mAddedStart = realStart;
		u.mAddedEnd = realEnd;
		u.mAdded = GetText(realStart, realEnd);


		// Update visuals
		SetSelection(visualStart, visualEnd);
		SetCursorPosition(cursorCoords);
	}


	u.mAfter = mState;
	AddUndo(u);

	mTextChanged = true;
	Colorize(cursorCoords.mLine - 1, 3);
	EnsureCursorVisible();
}


void CodeEditor::HandleMouseInputs() {
	ImGuiIO &io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered())
	{
		if (!shift && !alt)
		{
			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);
			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (!ctrl)
				{
					mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					mSelectionMode = SelectionMode::Line;
					SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
				}

				mLastClick = -1.0f;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (!ctrl)
				{
					mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					if (mSelectionMode == SelectionMode::Line)
						mSelectionMode = SelectionMode::Normal;
					else
						mSelectionMode = SelectionMode::Word;
					SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
				}

				mLastClick = (float)ImGui::GetTime();
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				if (ctrl)
					mSelectionMode = SelectionMode::Word;
				else
					mSelectionMode = SelectionMode::Normal;
				SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);

				mLastClick = (float)ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
			{
				io.WantCaptureMouse = true;
				mState.mCursorPosition = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
			}
		}
	}
}


void CodeEditor::Render() {
	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	mCharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);

	/* Update palette with the current alpha from style */
	for (int i = 0; i < (int)PaletteIndex::Max; ++i)
	{
		auto color = ImGui::ColorConvertU32ToFloat4(mPaletteBase[i]);
		color.w *= ImGui::GetStyle().Alpha;
		mPalette[i] = ImGui::ColorConvertFloat4ToU32(color);
	}

	assert(mLineBuffer.empty());

	auto contentSize = ImGui::GetWindowContentRegionMax();
	auto drawList = ImGui::GetWindowDrawList();
	float longest(mTextStart);

	if (mScrollToTop)
	{
		mScrollToTop = false;
		ImGui::SetScrollY(0.f);
	}

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	auto scrollX = ImGui::GetScrollX();
	auto scrollY = ImGui::GetScrollY();

	auto lineNo = (int)floor(scrollY / mCharAdvance.y);
	auto globalLineMax = (int)mLines.size();
	auto lineMax = std::max(0, std::min((int)mLines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));

	// Deduce mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
	char buf[16];
	snprintf(buf, 16, " %d ", globalLineMax);
	mTextStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + mLeftMargin;


	// ========== FIND & REPLACE ==========
	if (mShowFindReplaceWindow) {
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImVec2 parentPos = ImGui::GetWindowPos();
		ImVec2 parentSize = ImGui::GetWindowSize();

		float padding = 20.0f;

		static const std::string currentDocumentComboStr = "Current document";
		static const std::string selectionComboStr = "Selection";
		static int selectedSearchRange = 0;

		const char *searchRanges[] = { currentDocumentComboStr.c_str(), selectionComboStr.c_str() };
		const char *selectedOption = searchRanges[selectedSearchRange];

		static bool foundNoMatches = false;
		static bool caseSensitive = false;

		static std::vector<std::pair<Coordinates, Coordinates>> searchResults{};
		static bool searchResultsDirty = false;
		static size_t currentSearchIdx = 0;

		static Coordinates lastSelectStart(0, 0), lastSelectEnd(0, 0);
		bool selectionChanged = (
			selectedOption == selectionComboStr &&													// in selection mode &&
			(mState.mSelectionStart != lastSelectStart || mState.mSelectionEnd != lastSelectEnd)	// selection has changed
		);


		static std::function<void(const std::string &, bool)> tryPopulateSearchResults = [this, &selectedOption, &selectionChanged](const std::string &findPattern, bool forceUpdate) {
			if (mFindBuffer.empty()) {
				searchResults.clear();
				return;
			}

			
			if (
				selectionChanged || searchResultsDirty || forceUpdate
			) {
				// Populate search results with a specified search range
				if (selectedOption == selectionComboStr && HasSelection()) {
					searchResults = FindSubstrings(mState.mSelectionStart, mState.mSelectionEnd, mFindBuffer, caseSensitive);
					lastSelectStart = mState.mSelectionStart;
					lastSelectEnd = mState.mSelectionEnd;
				}
				else if (selectedOption == currentDocumentComboStr) {
					searchResults = FindSubstrings(mFindBuffer, caseSensitive);
				}


				// Highlight search result substrings
				ClearHighlights();
				for (const auto &[start, end] : searchResults)
					AddHighlight(start, end);


				// Reset indices & flags
				if (!searchResults.empty()) {
					currentSearchIdx %= searchResults.size();
					searchResultsDirty = false;
				}
			}
		};


		// Moves the cursor to the current found substring
		static std::function<void()> jumpToCurrentResult = [this, &selectedOption]() {
			Coordinates findStart = searchResults[currentSearchIdx].first;
			Coordinates findEnd = searchResults[currentSearchIdx].second;

			JumpToPosition(findEnd);

			if (selectedOption == selectionComboStr)
				SetCursorPosition(findStart);
			else
				SetSelection(findStart, findEnd);
		};


		// Updates the search result index
		static std::function<void()> advanceResultIdx = []() {
			currentSearchIdx = (currentSearchIdx + 1) % searchResults.size();
		};


		ImGuiUtils::PushStyleClearButton();
		{
			ImGui::SetNextWindowPos(ImVec2((parentPos.x + parentSize.x) - padding, parentPos.y + padding), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
			if (ImGui::BeginChild("###FindAndReplace", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
				mFindReplaceWindowFocused = ImGui::IsWindowFocused();

				// Attempt to populate search results in the background
				tryPopulateSearchResults(mFindBuffer, false);

				ImGuiUtils::BoldText("Find & Replace");

				ImGuiUtils::PaddedSeparator();

				// Find
				ImGui::BeginGroup();
				{
					if (foundNoMatches) {
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 0, 0, 1)); // Red border
					}
					if (ImGui::InputText("##InputTextFind", &mFindBuffer)) {
						foundNoMatches = false;
						searchResultsDirty = true;
					}
					if (foundNoMatches) {
						ImGui::PopStyleColor();
						ImGui::PopStyleVar();
					}
					ImGuiUtils::InputPlaceholderText(mFindBuffer.empty(), "Find...");


					ImGui::SameLine();
					ImGuiUtils::Padding();
					ImGui::SameLine();


					ImGui::PushFont(g_guiCtx.primaryFont);
					{
						static size_t displayResultIdx = 0;

						if (ImGui::Button(ICON_FA_ARROW_RIGHT)) {
							// Ensure search results are not stale
							tryPopulateSearchResults(mFindBuffer, true);

							if (!searchResults.empty()) {
								foundNoMatches = false;
								displayResultIdx = currentSearchIdx + 1;

								jumpToCurrentResult();
								advanceResultIdx();
							}
							else
								foundNoMatches = true;
						}
						ImGuiUtils::CursorOnHover();
						ImGuiUtils::TextTooltip(0,
							"Find Next%s",
							(searchResults.empty()) ? "" : std::format(" ({}/{})", displayResultIdx, searchResults.size()).c_str()
						);
					}
					ImGui::PopFont();
				}
				ImGui::EndGroup();


				ImGuiUtils::Padding();


				// Replace (only for editable files)
				static std::optional<size_t> singleReplaceFirstIdx = std::nullopt;

				if (!IsReadOnly()) {
					ImGui::BeginGroup();
					{
						ImGui::InputText("##InputTextReplace", &mReplaceBuffer);
						ImGuiUtils::InputPlaceholderText(mReplaceBuffer.empty(), "Replace...");

						ImGui::SameLine();
						ImGuiUtils::Padding();
						ImGui::SameLine();

						ImGui::PushFont(g_guiCtx.primaryFont);
						{
							if (ImGui::Button(ICON_FA_ARROWS_ROTATE)) {
								// Ensure search results are not stale
								tryPopulateSearchResults(mFindBuffer, true);


								if (!searchResults.empty()) {
									if (!singleReplaceFirstIdx.has_value()) {
										// Set first replace index for proper search wrapping
										singleReplaceFirstIdx = currentSearchIdx;
									}


									auto replaceStart = searchResults[currentSearchIdx].first;
									auto replaceEnd = searchResults[currentSearchIdx].second;

									// Update visuals
									jumpToCurrentResult();
									DeleteHighlight(replaceStart, replaceEnd);


									// Single replace: 1 UndoRecord per replace
									UndoRecord u{};
									u.mBefore = mState;

									Coordinates removeStart(replaceStart.mLine, 0);
									Coordinates removeEnd(replaceEnd.mLine, GetLineMaxColumn(replaceEnd.mLine));

										// STATE SNAPSHOT: pre-replace
									u.mRemovedStart = removeStart;
									u.mRemovedEnd = removeEnd;
									u.mRemoved = GetText(removeStart, removeEnd);

										// Replace
									DeleteRange(replaceStart, replaceEnd);
									InsertTextAt(replaceStart, mReplaceBuffer.c_str());

										// STATE SNAPSHOT: post-replace
									removeEnd.mColumn = GetLineMaxColumn(removeEnd.mLine);
									u.mAddedStart = removeStart;
									u.mAddedEnd = removeEnd;
									u.mAdded = GetText(removeStart, removeEnd);

									const int replaceDiff = static_cast<int>(mReplaceBuffer.size()) - static_cast<int>(mFindBuffer.size());


									// Update visuals
									Coordinates cursorEnd(replaceEnd.mLine, replaceEnd.mColumn + replaceDiff);
								
									SetCursorPosition(cursorEnd);
									SetSelection(replaceStart, cursorEnd);


									// Add undo record & update editor state
									u.mAfter = mState;
									AddUndo(u);

									mTextChanged = true;
									Colorize(replaceStart.mLine - 1, 3);
									EnsureCursorVisible();


									// Other substrings on the same line have shifted positions; update accordingly
									int searchIdx = currentSearchIdx;
									while (++searchIdx < searchResults.size()) {
										auto &[start, end] = searchResults[searchIdx];

										if (start.mLine > replaceStart.mLine)
											break;

										start.mColumn += replaceDiff;
										end.mColumn += replaceDiff;
									}


									// Go to next replace (or search results if nothing else to replace)
									searchResultsDirty = true;
									advanceResultIdx();
									if (currentSearchIdx == singleReplaceFirstIdx) {
										// Search index has wrapped back to the first replace index; everything has been replaced
										searchResults.clear();
										singleReplaceFirstIdx = std::nullopt;
									}
								}
							}
							ImGuiUtils::CursorOnHover();
							ImGuiUtils::TextTooltip(0, "Replace Next");


							ImGui::SameLine();


							if (ImGui::Button(ICON_FA_ARROWS_SPIN)) {
								// Ensure search results are not stale
								tryPopulateSearchResults(mFindBuffer, true);


								if (!searchResults.empty()) {
									// Bulk replace: 1 Undo record for all replaces
									UndoRecord u{};
									u.mBefore = mState;

										// Define replace start/end coordinates & text range
											// Since searchResults already sorted since FindSubstring does a top-to-bottom text search,
											// the first element is always the first found substring, and the last element is always the last found substring
									auto firstReplace = searchResults.front();
									auto lastReplace = searchResults.back();

									Coordinates replaceStart(firstReplace.first.mLine, 0);
									Coordinates replaceEnd(lastReplace.second.mLine, GetLineMaxColumn(lastReplace.second.mLine));


										// STATE SNAPSHOT: pre-replace
									u.mRemovedStart = replaceStart;
									u.mRemovedEnd = replaceEnd;
									u.mRemoved = GetText(replaceStart, replaceEnd);


										// Replace
									Coordinates newEnd{};
									const int replaceDiff = static_cast<int>(mReplaceBuffer.size()) - static_cast<int>(mFindBuffer.size());
									for (auto &[start, end] : searchResults) {
										DeleteRange(start, end);
										InsertTextAt(start, mReplaceBuffer.c_str());
										newEnd = Coordinates(end.mLine, end.mColumn + replaceDiff);


										// Other substrings on the same line have shifted positions; update accordingly
										int searchIdx = currentSearchIdx;
										while (++searchIdx < searchResults.size()) {
											auto &[otherStart, otherEnd] = searchResults[searchIdx];

											if (otherStart.mLine > start.mLine)
												break;

											otherStart.mColumn += replaceDiff;
											otherEnd.mColumn += replaceDiff;
										}
									}

										// STATE SNAPSHOT: post-replace
									replaceEnd.mColumn = GetLineMaxColumn(replaceEnd.mLine);
									u.mAddedStart = replaceStart;
									u.mAddedEnd = replaceEnd;
									u.mAdded = GetText(replaceStart, replaceEnd);

									SetCursorPosition(newEnd);

									u.mAfter = mState;
									AddUndo(u);

									mTextChanged = true;
									Colorize(replaceStart.mLine - 1, replaceEnd.mLine - replaceStart.mLine + 2);
									EnsureCursorVisible();


									// Clear highlights and search results & mark dirty
									ClearHighlights();
									searchResults.clear();
									searchResultsDirty = true;
								}
							}
							ImGuiUtils::CursorOnHover();
							ImGuiUtils::TextTooltip(0, "Replace All");
						}
						ImGui::PopFont();
					}
					ImGui::EndGroup();
				}


				ImGuiUtils::Padding();


				// Settings
				ImGui::BeginGroup();
				{
					// Case sensitivity
					bool highlightBorders = caseSensitive;  // We don't directly use `caseSensitive` to prevent changes to the variable made between the `Pop` and `Push` blocks producing incoherent state
					if (highlightBorders) {
						// Change button border style if case sensitivity is enabled
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
						ImGui::PushStyleColor(ImGuiCol_Border,
							ImGuiTheme::GetThemeColors(g_guiCtx.GUI.currentAppearance).FrameBgActive
						);
					}
					if (ImGui::Button(ICON_FA_A)) {
						caseSensitive = !caseSensitive;
						searchResultsDirty = true;
					}
					if (highlightBorders) {
						ImGui::PopStyleColor();
						ImGui::PopStyleVar();
					}
					ImGuiUtils::CursorOnHover();
					ImGuiUtils::TextTooltip(0, "Case-sensitive search");


					ImGui::SameLine();
					ImGuiUtils::Padding();
					ImGui::SameLine();


					// Search range selection
					if (ImGui::Combo("##CodeEditorFindReplaceSearchRangeCombo", &selectedSearchRange, searchRanges, IM_ARRAYSIZE(searchRanges))) {
						std::cout << selectedSearchRange << '\n';
						searchResultsDirty = true;
					}
				}
				ImGui::EndGroup();


				ImGui::EndChild();
			}
		}
		ImGuiUtils::PopStyleClearButton();


		ImGui::SetCursorPos(cursorPos);
	}


	// ========== LINES & TEXT ==========
	if (!mLines.empty())
	{
		float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

		while (lineNo <= lineMax)
		{
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

			auto &line = mLines[lineNo];
			longest = std::max(mTextStart + TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);
			auto columnNo = 0;
			Coordinates lineStartCoord(lineNo, 0);
			Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo));


			// Draw selection & highlights for the current line
			static std::function<void(const CodeEditor::Coordinates&, const CodeEditor::Coordinates&, CodeEditor::PaletteIndex)> drawSelectedArea =
				[&](const CodeEditor::Coordinates &aStart, const CodeEditor::Coordinates &aEnd, CodeEditor::PaletteIndex paletteIdx) {
					float sstart = -1.0f;
					float ssend = -1.0f;

					assert(aStart <= aEnd);
					if (aStart <= lineEndCoord)
						sstart = aStart > lineStartCoord ? TextDistanceToLineStart(aStart) : 0.0f;
					if (aEnd > lineStartCoord)
						ssend = TextDistanceToLineStart(aEnd < lineEndCoord ? aEnd : lineEndCoord);

					if (aEnd.mLine > lineNo)
						ssend += mCharAdvance.x;

					if (sstart != -1 && ssend != -1 && sstart < ssend)
					{
						ImVec2 vstart(lineStartScreenPos.x + mTextStart + sstart, lineStartScreenPos.y);
						ImVec2 vend(lineStartScreenPos.x + mTextStart + ssend, lineStartScreenPos.y + mCharAdvance.y);
						drawList->AddRectFilled(vstart, vend, mPalette[(int)paletteIdx]);
					}
			};

				// Highlights (drawn after the line and cursor highlighting logic so that it is always visible; scroll to before colorized text rendering)

				// Selection
			drawSelectedArea(mState.mSelectionStart, mState.mSelectionEnd, PaletteIndex::Selection);


			// Draw breakpoints
			auto start = ImVec2(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

			if (mBreakpoints.count(lineNo + 1) != 0)
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, mPalette[(int)PaletteIndex::Breakpoint]);
			}


			// Draw error markers
			auto errorIt = mErrorMarkers.find(lineNo + 1);
			if (errorIt != mErrorMarkers.end())
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, mPalette[(int)PaletteIndex::ErrorMarker]);

				if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end))
				{
					ImGui::BeginTooltip();

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
					{
						ImGui::Text("%s", errorIt->second.first.c_str());		// Error title
						ImGui::Separator();
						ImGui::Text("%s", errorIt->second.second.c_str());		// Error details
					}
					ImGui::PopStyleColor();

					ImGui::EndTooltip();
				}
			}


			if (mState.mCursorPosition.mLine == lineNo)
			{
				auto focused = ImGui::IsWindowFocused();

				// Highlight the current line (where the cursor is); ignore if there is a selection or an error on that line (for the red highlighting)
				if (!HasSelection() && errorIt == mErrorMarkers.end())
				{
					auto end = ImVec2(start.x + contentSize.x + scrollX, start.y + mCharAdvance.y);
					drawList->AddRectFilled(start, end, mPalette[(int)(focused ? PaletteIndex::CurrentLineFill : PaletteIndex::CurrentLineFillInactive)]);
					drawList->AddRect(start, end, mPalette[(int)PaletteIndex::CurrentLineEdge], 1.0f);
				}

				// Render the cursor only when the editor is focused
				if (focused)
				{
					auto timeEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					auto elapsed = timeEnd - mStartTime;
					if (elapsed > 400)
					{
						float width = 1.0f;
						auto cindex = GetCharacterIndex(mState.mCursorPosition);
						float cx = TextDistanceToLineStart(mState.mCursorPosition);

						if (mOverwrite && cindex < (int)line.size())
						{
							auto c = line[cindex].mChar;
							if (c == '\t')
							{
								auto x = (1.0f + std::floor((1.0f + cx) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
								width = x - cx;
							}
							else
							{
								char buf2[2];
								buf2[0] = line[cindex].mChar;
								buf2[1] = '\0';
								width = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf2).x;
							}
						}
						ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
						ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + mCharAdvance.y);
						drawList->AddRectFilled(cstart, cend, mPalette[(int)PaletteIndex::Cursor]);
						if (elapsed > 800)
							mStartTime = timeEnd;
					}
				}
			}


			// Draw text highlights
			for (const auto &[start, end] : mHighlights)
				drawSelectedArea(start, end, PaletteIndex::Highlight);


			// Render colorized text
			auto prevColor = line.empty() ? mPalette[(int)PaletteIndex::Default] : GetGlyphColor(line[0]);
			ImVec2 bufferOffset;

			for (int i = 0; i < line.size();)
			{
				auto &glyph = line[i];
				auto color = GetGlyphColor(glyph);

				if ((color != prevColor || glyph.mChar == '\t' || glyph.mChar == ' ') && !mLineBuffer.empty())
				{
					const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
					drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
					auto textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, mLineBuffer.c_str(), nullptr, nullptr);
					bufferOffset.x += textSize.x;
					mLineBuffer.clear();
				}
				prevColor = color;

				if (glyph.mChar == '\t')
				{
					auto oldX = bufferOffset.x;
					bufferOffset.x = (1.0f + std::floor((1.0f + bufferOffset.x) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
					++i;

					if (mShowWhitespaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x1 = textScreenPos.x + oldX + 1.0f;
						const auto x2 = textScreenPos.x + bufferOffset.x - 1.0f;
						const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						const ImVec2 p1(x1, y);
						const ImVec2 p2(x2, y);
						const ImVec2 p3(x2 - s * 0.2f, y - s * 0.2f);
						const ImVec2 p4(x2 - s * 0.2f, y + s * 0.2f);
						drawList->AddLine(p1, p2, 0x90909090);
						drawList->AddLine(p2, p3, 0x90909090);
						drawList->AddLine(p2, p4, 0x90909090);
					}
				}
				else if (glyph.mChar == ' ')
				{
					if (mShowWhitespaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x = textScreenPos.x + bufferOffset.x + spaceSize * 0.5f;
						const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						drawList->AddCircleFilled(ImVec2(x, y), 1.5f, 0x80808080, 4);
					}
					bufferOffset.x += spaceSize;
					i++;
				}
				else
				{
					auto l = UTF8CharLength(glyph.mChar);
					while (l-- > 0)
						mLineBuffer.push_back(line[i++].mChar);
				}
				++columnNo;
			}


			// Draw line number (right aligned)
			snprintf(buf, 16, "%d  ", lineNo + 1);

			auto lineNoWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;
			drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - lineNoWidth, lineStartScreenPos.y), mPalette[(int)PaletteIndex::LineNumber], buf);


			if (!mLineBuffer.empty())
			{
				const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
				drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
				mLineBuffer.clear();
			}

			++lineNo;
		}


		// Draw a tooltip on known identifiers/preprocessor symbols
		const ImVec2 tooltipSz(500.0f, 0.0f);

		if (ImGui::IsMousePosValid()) {
			auto token = GetWordAt(ScreenPosToCoordinates(ImGui::GetMousePos()));
			if (!token.empty()) {
				auto idIt = mLanguageDefinition.mIdentifiers.find(token);
				auto kwIt = mLanguageDefinition.mKeywords.find(token);
				auto piIt = mLanguageDefinition.mPreprocIdentifiers.find(token);


				// Identifiers || Preprocessor symbols
				if (idIt != mLanguageDefinition.mIdentifiers.end() || piIt != mLanguageDefinition.mPreprocIdentifiers.end())
				{
					const IdentifierDescriptor &id = 
						(idIt != mLanguageDefinition.mIdentifiers.end()) ? idIt->second
																		 : piIt->second;

					bool hasUnit = id.mSimUnit.has_value();
					bool hasExpectedType = id.mExpectedType.has_value();

					bool hasOptionalDetails = hasUnit || hasExpectedType;

					std::string valType = (hasExpectedType) ? id.TypeToString() : "";


					ImGui::SetNextWindowSize(tooltipSz);
					ImGui::BeginTooltip();
					{
						// Header
						ImGui::PushFont(g_guiCtx.Font.regularMono);
						{
							if (hasUnit || hasExpectedType)
								ImGui::Text(
									"%s%s: %s",
									id.mName.c_str(),
									(hasUnit) ? std::format(" ({})", id.mSimUnit.value()).c_str() : "",
									(hasExpectedType) ? valType.c_str() : ""
								);
							else
								ImGui::Text(id.mName.c_str());
						}
						ImGui::PopFont();

						ImGuiUtils::PaddedSeparator();

						ImGuiUtils::BoldText("Description");
						ImGuiUtils::ItalicText(id.mDeclaration.c_str());

						if (hasOptionalDetails) {
							ImGuiUtils::Padding(20.0f);

							if (hasUnit) {
								ImGuiUtils::BoldText("Simulation unit: ");
								ImGui::SameLine();
								ImGuiUtils::ItalicText(id.mSimUnit.value().c_str());
							}

							if (hasExpectedType) {
								ImGuiUtils::BoldText("Expected value type: ");
								ImGui::SameLine();
								ImGuiUtils::ItalicText(valType.c_str());
							}
						}
					}
					ImGui::EndTooltip();
				}


				// Keywords
				else if (kwIt != mLanguageDefinition.mKeywords.end()) {
					const KeywordDescriptor &kw = kwIt->second;

					bool hasExpectedType = kw.mExpectedType.has_value();

					std::string valType = (hasExpectedType) ? kw.TypeToString() : "";


					ImGui::SetNextWindowSize(tooltipSz);
					ImGui::BeginTooltip();
					{
						// Header
						ImGui::PushFont(g_guiCtx.Font.regularMono);
						{
							if (hasExpectedType)
								ImGui::Text(
									"%s: %s",
									kw.mName.c_str(),
									(hasExpectedType) ? valType.c_str() : ""
								);
							else
								ImGui::Text(kw.mName.c_str());
						}
						ImGui::PopFont();

						ImGuiUtils::PaddedSeparator();

						ImGuiUtils::BoldText("Description");
						ImGuiUtils::ItalicText(kw.mDeclaration.c_str());

							
						if (hasExpectedType) {
							ImGuiUtils::Padding(20.0f);

							ImGuiUtils::BoldText("Expected value type: ");
							ImGui::SameLine();
							ImGuiUtils::ItalicText(valType.c_str());
						}
					}
					ImGui::EndTooltip();
				}
			}
		}
	}


	ImGui::Dummy(ImVec2((longest + 2), mLines.size() * mCharAdvance.y));

	if (mScrollToCursor)
	{
		EnsureCursorVisible();
		ImGui::SetWindowFocus();
		mScrollToCursor = false;
	}
}


void CodeEditor::Render(const char *aTitle, const ImVec2 &aSize, bool aBorder) {
	mWithinRender = true;
	mTextChanged = false;
	mCursorPositionChanged = false;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(mPalette[(int)PaletteIndex::Background]));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	if (!mIgnoreImGuiChild)
		ImGui::BeginChild(aTitle, aSize, aBorder, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);

	if (mHandleKeyboardInputs) {
		HandleKeyboardInputs();
		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, false);
	}

	if (mHandleMouseInputs)
		HandleMouseInputs();

	ColorizeInternal();
	Render();

	if (mHandleKeyboardInputs)
		ImGui::PopItemFlag();

	if (!mIgnoreImGuiChild)
		ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	mWithinRender = false;
}


void CodeEditor::SetText(const std::string &aText) {
	mLines.clear();
	mLines.emplace_back(Line());
	for (auto chr : aText)
	{
		if (chr == '\r')
		{
			// ignore the carriage return character
		}
		else if (chr == '\n')
			mLines.emplace_back(Line());
		else
		{
			mLines.back().emplace_back(Glyph(chr, PaletteIndex::Default));
		}
	}

	mTextChanged = true;
	mScrollToTop = true;

	mUndoBuffer.clear();
	mUndoIndex = 0;

	Colorize();
}


void CodeEditor::SetTextLines(const std::vector<std::string> &aLines) {
	mLines.clear();

	if (aLines.empty())
	{
		mLines.emplace_back(Line());
	}
	else
	{
		mLines.resize(aLines.size());

		for (size_t i = 0; i < aLines.size(); ++i)
		{
			const std::string &aLine = aLines[i];

			mLines[i].reserve(aLine.size());
			for (size_t j = 0; j < aLine.size(); ++j)
				mLines[i].emplace_back(Glyph(aLine[j], PaletteIndex::Default));
		}
	}

	mTextChanged = true;
	mScrollToTop = true;

	mUndoBuffer.clear();
	mUndoIndex = 0;

	Colorize();
}


void CodeEditor::EnterCharacter(ImWchar aChar, bool aShift) {
	assert(!mReadOnly);

	UndoRecord u;

	u.mBefore = mState;

	if (HasSelection())
	{
		if (aChar == '\t' && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine)
		{

			auto start = mState.mSelectionStart;
			auto end = mState.mSelectionEnd;
			auto originalEnd = end;

			if (start > end)
				std::swap(start, end);
			start.mColumn = 0;
			//			end.mColumn = end.mLine < mLines.size() ? mLines[end.mLine].size() : 0;
			if (end.mColumn == 0 && end.mLine > 0)
				--end.mLine;
			if (end.mLine >= (int)mLines.size())
				end.mLine = mLines.empty() ? 0 : (int)mLines.size() - 1;
			end.mColumn = GetLineMaxColumn(end.mLine);

			//if (end.mColumn >= GetLineMaxColumn(end.mLine))
			//	end.mColumn = GetLineMaxColumn(end.mLine) - 1;

			u.mRemovedStart = start;
			u.mRemovedEnd = end;
			u.mRemoved = GetText(start, end);

			bool modified = false;

			for (int i = start.mLine; i <= end.mLine; i++)
			{
				auto &line = mLines[i];
				if (aShift)
				{
					if (!line.empty())
					{
						if (line.front().mChar == '\t')
						{
							line.erase(line.begin());
							modified = true;
						}
						else
						{
							for (int j = 0; j < mTabSize && !line.empty() && line.front().mChar == ' '; j++)
							{
								line.erase(line.begin());
								modified = true;
							}
						}
					}
				}
				else
				{
					line.insert(line.begin(), Glyph('\t', CodeEditor::PaletteIndex::Background));
					modified = true;
				}
			}

			if (modified)
			{
				start = Coordinates(start.mLine, GetCharacterColumn(start.mLine, 0));
				Coordinates rangeEnd;
				if (originalEnd.mColumn != 0)
				{
					end = Coordinates(end.mLine, GetLineMaxColumn(end.mLine));
					rangeEnd = end;
					u.mAdded = GetText(start, end);
				}
				else
				{
					end = Coordinates(originalEnd.mLine, 0);
					rangeEnd = Coordinates(end.mLine - 1, GetLineMaxColumn(end.mLine - 1));
					u.mAdded = GetText(start, rangeEnd);
				}

				u.mAddedStart = start;
				u.mAddedEnd = rangeEnd;
				u.mAfter = mState;

				mState.mSelectionStart = start;
				mState.mSelectionEnd = end;
				AddUndo(u);

				mTextChanged = true;

				EnsureCursorVisible();
			}

			return;
		} // c == '\t'
		else
		{
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = mState.mSelectionStart;
			u.mRemovedEnd = mState.mSelectionEnd;
			DeleteSelection();
		}
	} // HasSelection

	auto coord = GetActualCursorCoordinates();
	u.mAddedStart = coord;

	assert(!mLines.empty());

	if (aChar == '\n')
	{
		InsertLine(coord.mLine + 1);
		auto &line = mLines[coord.mLine];
		auto &newLine = mLines[coord.mLine + 1];

		if (mLanguageDefinition.mAutoIndentation)
			for (size_t it = 0; it < line.size() && isascii(line[it].mChar) && isblank(line[it].mChar); ++it)
				newLine.push_back(line[it]);

		const size_t whitespaceSize = newLine.size();
		auto cindex = GetCharacterIndex(coord);
		newLine.insert(newLine.end(), line.begin() + cindex, line.end());
		line.erase(line.begin() + cindex, line.begin() + line.size());
		SetCursorPosition(Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, (int)whitespaceSize)));
		u.mAdded = (char)aChar;
	}
	else
	{
		char buf[7];
		int e = ImTextCharToUtf8(buf, 7, aChar);
		if (e > 0)
		{
			buf[e] = '\0';
			auto &line = mLines[coord.mLine];
			auto cindex = GetCharacterIndex(coord);

			if (mOverwrite && cindex < (int)line.size())
			{
				auto d = UTF8CharLength(line[cindex].mChar);

				u.mRemovedStart = mState.mCursorPosition;
				u.mRemovedEnd = Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex + d));

				while (d-- > 0 && cindex < (int)line.size())
				{
					u.mRemoved += line[cindex].mChar;
					line.erase(line.begin() + cindex);
				}
			}

			for (auto p = buf; *p != '\0'; p++, ++cindex)
				line.insert(line.begin() + cindex, Glyph(*p, PaletteIndex::Default));
			u.mAdded = buf;

			SetCursorPosition(Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex)));
		}
		else
			return;
	}

	mTextChanged = true;

	u.mAddedEnd = GetActualCursorCoordinates();
	u.mAfter = mState;

	AddUndo(u);

	Colorize(coord.mLine - 1, 3);
	EnsureCursorVisible();
}


void CodeEditor::SetReadOnly(bool aValue) {
	mReadOnly = aValue;
}


void CodeEditor::SetColorizerEnable(bool aValue) {
	mColorizerEnabled = aValue;
}


void CodeEditor::SetCursorPosition(const Coordinates &aPosition) {
	if (mState.mCursorPosition != aPosition)
	{
		mState.mCursorPosition = aPosition;
		mCursorPositionChanged = true;
		EnsureCursorVisible();
	}
}


void CodeEditor::SetSelectionStart(const Coordinates &aPosition) {
	mState.mSelectionStart = SanitizeCoordinates(aPosition);
	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);
}


void CodeEditor::SetSelectionEnd(const Coordinates &aPosition) {
	mState.mSelectionEnd = SanitizeCoordinates(aPosition);
	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);
}


void CodeEditor::SetSelection(const Coordinates &aStart, const Coordinates &aEnd, SelectionMode aMode) {
	auto oldSelStart = mState.mSelectionStart;
	auto oldSelEnd = mState.mSelectionEnd;

	mState.mSelectionStart = SanitizeCoordinates(aStart);
	mState.mSelectionEnd = SanitizeCoordinates(aEnd);
	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);

	switch (aMode)
	{
	case CodeEditor::SelectionMode::Normal:
		break;
	case CodeEditor::SelectionMode::Word:
	{
		mState.mSelectionStart = FindWordStart(mState.mSelectionStart);
		if (!IsOnWordBoundary(mState.mSelectionEnd))
			mState.mSelectionEnd = FindWordEnd(FindWordStart(mState.mSelectionEnd));
		break;
	}
	case CodeEditor::SelectionMode::Line:
	{
		const auto lineNo = mState.mSelectionEnd.mLine;
		const auto lineSize = (size_t)lineNo < mLines.size() ? mLines[lineNo].size() : 0;
		mState.mSelectionStart = Coordinates(mState.mSelectionStart.mLine, 0);
		mState.mSelectionEnd = Coordinates(lineNo, GetLineMaxColumn(lineNo));
		break;
	}
	default:
		break;
	}

	if (mState.mSelectionStart != oldSelStart ||
		mState.mSelectionEnd != oldSelEnd)
		mCursorPositionChanged = true;
}


void CodeEditor::SetTabSize(int aValue) {
	mTabSize = std::max(0, std::min(32, aValue));
}


void CodeEditor::InsertText(const std::string &aValue) {
	InsertText(aValue.c_str());
}


void CodeEditor::InsertText(const char *aValue) {
	if (aValue == nullptr)
		return;

	auto pos = GetActualCursorCoordinates();
	auto start = std::min(pos, mState.mSelectionStart);
	int totalLines = pos.mLine - start.mLine;

	totalLines += InsertTextAt(pos, aValue);

	SetSelection(pos, pos);
	SetCursorPosition(pos);
	Colorize(start.mLine - 1, totalLines + 2);
}


void CodeEditor::DeleteSelection() {
	assert(mState.mSelectionEnd >= mState.mSelectionStart);

	if (mState.mSelectionEnd == mState.mSelectionStart)
		return;

	DeleteRange(mState.mSelectionStart, mState.mSelectionEnd);

	SetSelection(mState.mSelectionStart, mState.mSelectionStart);
	SetCursorPosition(mState.mSelectionStart);
	Colorize(mState.mSelectionStart.mLine, 1);
}


void CodeEditor::MoveUp(int aAmount, bool aSelect) {
	auto oldPos = mState.mCursorPosition;
	mState.mCursorPosition.mLine = std::max(0, mState.mCursorPosition.mLine - aAmount);
	if (oldPos != mState.mCursorPosition)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else
			{
				mInteractiveStart = mState.mCursorPosition;
				mInteractiveEnd = oldPos;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);

		EnsureCursorVisible();
	}
}


void CodeEditor::MoveDown(int aAmount, bool aSelect) {
	assert(mState.mCursorPosition.mColumn >= 0);
	auto oldPos = mState.mCursorPosition;
	mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + aAmount));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else
			{
				mInteractiveStart = oldPos;
				mInteractiveEnd = mState.mCursorPosition;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);

		EnsureCursorVisible();
	}
}


static bool IsUTFSequence(char c) {
	return (c & 0xC0) == 0x80;
}


void CodeEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode) {
	if (mLines.empty())
		return;

	auto oldPos = mState.mCursorPosition;
	mState.mCursorPosition = GetActualCursorCoordinates();
	auto line = mState.mCursorPosition.mLine;
	auto cindex = GetCharacterIndex(mState.mCursorPosition);

	while (aAmount-- > 0)
	{
		if (cindex == 0)
		{
			if (line > 0)
			{
				--line;
				if ((int)mLines.size() > line)
					cindex = (int)mLines[line].size();
				else
					cindex = 0;
			}
		}
		else
		{
			--cindex;
			if (cindex > 0)
			{
				if ((int)mLines.size() > line)
				{
					while (cindex > 0 && IsUTFSequence(mLines[line][cindex].mChar))
						--cindex;
				}
			}
		}

		mState.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));
		if (aWordMode)
		{
			mState.mCursorPosition = FindWordStart(mState.mCursorPosition);
			cindex = GetCharacterIndex(mState.mCursorPosition);
		}
	}

	mState.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));

	assert(mState.mCursorPosition.mColumn >= 0);
	if (aSelect)
	{
		if (oldPos == mInteractiveStart)
			mInteractiveStart = mState.mCursorPosition;
		else if (oldPos == mInteractiveEnd)
			mInteractiveEnd = mState.mCursorPosition;
		else
		{
			mInteractiveStart = mState.mCursorPosition;
			mInteractiveEnd = oldPos;
		}
	}
	else
		mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
	SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}


void CodeEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode) {
	auto oldPos = mState.mCursorPosition;

	if (mLines.empty() || oldPos.mLine >= mLines.size())
		return;

	auto cindex = GetCharacterIndex(mState.mCursorPosition);
	while (aAmount-- > 0)
	{
		auto lindex = mState.mCursorPosition.mLine;
		auto &line = mLines[lindex];

		if (cindex >= line.size())
		{
			if (mState.mCursorPosition.mLine < mLines.size() - 1)
			{
				mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + 1));
				mState.mCursorPosition.mColumn = 0;
			}
			else
				return;
		}
		else
		{
			cindex += UTF8CharLength(line[cindex].mChar);
			mState.mCursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
			if (aWordMode)
				mState.mCursorPosition = FindNextWord(mState.mCursorPosition);
		}
	}

	if (aSelect)
	{
		if (oldPos == mInteractiveEnd)
			mInteractiveEnd = SanitizeCoordinates(mState.mCursorPosition);
		else if (oldPos == mInteractiveStart)
			mInteractiveStart = mState.mCursorPosition;
		else
		{
			mInteractiveStart = oldPos;
			mInteractiveEnd = mState.mCursorPosition;
		}
	}
	else
		mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
	SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}


void CodeEditor::MoveTop(bool aSelect) {
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(0, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			mInteractiveEnd = oldPos;
			mInteractiveStart = mState.mCursorPosition;
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);
	}
}


void CodeEditor::CodeEditor::MoveBottom(bool aSelect) {
	auto oldPos = GetCursorPosition();
	auto newPos = Coordinates((int)mLines.size() - 1, 0);
	SetCursorPosition(newPos);
	if (aSelect)
	{
		mInteractiveStart = oldPos;
		mInteractiveEnd = newPos;
	}
	else
		mInteractiveStart = mInteractiveEnd = newPos;
	SetSelection(mInteractiveStart, mInteractiveEnd);
}


void CodeEditor::MoveHome(bool aSelect) {
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else
			{
				mInteractiveStart = mState.mCursorPosition;
				mInteractiveEnd = oldPos;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);
	}
}


void CodeEditor::MoveEnd(bool aSelect) {
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, GetLineMaxColumn(oldPos.mLine)));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else
			{
				mInteractiveStart = oldPos;
				mInteractiveEnd = mState.mCursorPosition;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);
	}
}


void CodeEditor::Delete() {
	assert(!mReadOnly);

	if (mLines.empty())
		return;

	UndoRecord u;
	u.mBefore = mState;

	if (HasSelection())
	{
		u.mRemoved = GetSelectedText();
		u.mRemovedStart = mState.mSelectionStart;
		u.mRemovedEnd = mState.mSelectionEnd;

		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		auto &line = mLines[pos.mLine];

		if (pos.mColumn == GetLineMaxColumn(pos.mLine))
		{
			if (pos.mLine == (int)mLines.size() - 1)
				return;

			u.mRemoved = '\n';
			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			Advance(u.mRemovedEnd);

			auto &nextLine = mLines[pos.mLine + 1];
			line.insert(line.end(), nextLine.begin(), nextLine.end());
			RemoveLine(pos.mLine + 1);
		}
		else
		{
			auto cindex = GetCharacterIndex(pos);
			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			u.mRemovedEnd.mColumn++;
			u.mRemoved = GetText(u.mRemovedStart, u.mRemovedEnd);

			auto d = UTF8CharLength(line[cindex].mChar);
			while (d-- > 0 && cindex < (int)line.size())
				line.erase(line.begin() + cindex);
		}

		mTextChanged = true;

		Colorize(pos.mLine, 1);
	}

	u.mAfter = mState;
	AddUndo(u);
}


void CodeEditor::Backspace() {
	assert(!mReadOnly);

	if (mLines.empty())
		return;

	UndoRecord u;
	u.mBefore = mState;

	if (HasSelection())
	{
		u.mRemoved = GetSelectedText();
		u.mRemovedStart = mState.mSelectionStart;
		u.mRemovedEnd = mState.mSelectionEnd;

		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);

		if (mState.mCursorPosition.mColumn == 0)
		{
			if (mState.mCursorPosition.mLine == 0)
				return;

			u.mRemoved = '\n';
			u.mRemovedStart = u.mRemovedEnd = Coordinates(pos.mLine - 1, GetLineMaxColumn(pos.mLine - 1));
			Advance(u.mRemovedEnd);

			auto &line = mLines[mState.mCursorPosition.mLine];
			auto &prevLine = mLines[mState.mCursorPosition.mLine - 1];
			auto prevSize = GetLineMaxColumn(mState.mCursorPosition.mLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			ErrorMarkers etmp;
			for (auto &i : mErrorMarkers)
				etmp.insert(ErrorMarkers::value_type(i.first - 1 == mState.mCursorPosition.mLine ? i.first - 1 : i.first, i.second));
			mErrorMarkers = std::move(etmp);

			RemoveLine(mState.mCursorPosition.mLine);
			--mState.mCursorPosition.mLine;
			mState.mCursorPosition.mColumn = prevSize;
		}
		else
		{
			auto &line = mLines[mState.mCursorPosition.mLine];
			auto cindex = GetCharacterIndex(pos) - 1;
			auto cend = cindex + 1;
			while (cindex > 0 && IsUTFSequence(line[cindex].mChar))
				--cindex;

			//if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
			//	--cindex;

			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			--u.mRemovedStart.mColumn;
			--mState.mCursorPosition.mColumn;

			while (cindex < line.size() && cend-- > cindex)
			{
				u.mRemoved += line[cindex].mChar;
				line.erase(line.begin() + cindex);
			}
		}

		mTextChanged = true;

		EnsureCursorVisible();
		Colorize(mState.mCursorPosition.mLine, 1);
	}

	u.mAfter = mState;
	AddUndo(u);
}


void CodeEditor::SelectWordUnderCursor() {
	auto c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}


void CodeEditor::SelectAll() {
	SetSelection(Coordinates(0, 0), Coordinates((int)mLines.size(), 0));
}


bool CodeEditor::HasSelection() const {
	return mState.mSelectionEnd > mState.mSelectionStart;
}


void CodeEditor::DeleteHighlight(const Coordinates &aStart, const Coordinates &aEnd) {
	std::optional<size_t> newHighlightAfterIdx = std::nullopt;
	std::pair<Coordinates, Coordinates> newHighlight{};

	for (size_t i = 0; i < mHighlights.size(); i++) {
		auto &[start, end] = mHighlights[i];

		if (start >= aEnd || end <= aStart)
			continue;

		// NOTATION:  | = highlight bound		. = text		_ = highlighted text

		if (start <= aStart) {
			// CASE: end within [aStart..aEnd]
			// STATE:	 start       aStart   end      aEnd
			// STATE:   ...|___________|_______|........|
			// CHANGE:                end
			// FINAL:   ...|___________|................|
			if (end >= aStart && end <= aEnd) {
				end = aStart;
				break;
			}
			
			// CASE: end > aEnd
			// STATE:    start       aStart            aEnd          end
			// STATE:   ...|___________|________________|_____________|
			// CHANGE:                end                          newEnd
			// FINAL:   ...|___________|................|_____________|
			else if (end > aEnd) {
				auto newEnd = end;
				end = aStart;

				newHighlightAfterIdx = i;
				newHighlight = std::pair{ aEnd, newEnd };

				break;
			}
		}

		else {
			// CASE: end <= aEnd
			// STATE:	 aStart      start   end       aEnd
			// STATE:   ...|...........|______|.........|
			// CHANGE:                end
			// FINAL:   ...|...........|................|
			if (end <= aEnd) {
				end = start;
				break;
			}


			// CASE: end > aEnd
			// STATE:	 aStart      start             aEnd          end
			// STATE:   ...|...........|________________|_____________|
			// CHANGE:                                start
			// STATE:   ...|...........|................|_____________|
			else if (end > aEnd) {
				start = aEnd;
				break;
			}
		}
	}


	if (newHighlightAfterIdx.has_value())
		// Insert right after old fragmented highlight to maintain order
		mHighlights.insert(mHighlights.begin() + newHighlightAfterIdx.value() + 1, newHighlight);
}


void CodeEditor::Copy() {
	if (HasSelection())
	{
		ImGui::SetClipboardText(GetSelectedText().c_str());
	}
	else
	{
		if (!mLines.empty())
		{
			std::string str;
			auto &line = mLines[GetActualCursorCoordinates().mLine];
			for (auto &g : line)
				str.push_back(g.mChar);
			ImGui::SetClipboardText(str.c_str());
		}
	}
}


void CodeEditor::Cut() {
	if (IsReadOnly())
	{
		Copy();
	}
	else
	{
		if (HasSelection())
		{
			UndoRecord u;
			u.mBefore = mState;
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = mState.mSelectionStart;
			u.mRemovedEnd = mState.mSelectionEnd;

			Copy();
			DeleteSelection();

			u.mAfter = mState;
			AddUndo(u);
		}
	}
}


void CodeEditor::Paste() {
	if (IsReadOnly())
		return;

	auto clipText = ImGui::GetClipboardText();
	if (clipText != nullptr && strlen(clipText) > 0)
	{
		UndoRecord u;
		u.mBefore = mState;

		if (HasSelection())
		{
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = mState.mSelectionStart;
			u.mRemovedEnd = mState.mSelectionEnd;
			DeleteSelection();
		}

		u.mAdded = clipText;
		u.mAddedStart = GetActualCursorCoordinates();

		InsertText(clipText);

		u.mAddedEnd = GetActualCursorCoordinates();
		u.mAfter = mState;
		AddUndo(u);
	}
}


bool CodeEditor::CanUndo() const {
	return !mReadOnly && mUndoIndex > 0;
}


bool CodeEditor::CanRedo() const {
	return !mReadOnly && mUndoIndex < (int)mUndoBuffer.size();
}


void CodeEditor::Undo(int aSteps) {
	while (CanUndo() && aSteps-- > 0)
		mUndoBuffer[--mUndoIndex].Undo(this);
}


void CodeEditor::Redo(int aSteps) {
	while (CanRedo() && aSteps-- > 0)
		mUndoBuffer[mUndoIndex++].Redo(this);
}


void CodeEditor::JumpToPosition(const Coordinates &aPosition) {
	SetCursorPosition(aPosition);

	//EnsureCursorVisible();
	mScrollToCursor = true;  // Force-scroll to cursor instead of using EnsureCursorVisible
}


std::vector<std::pair<CodeEditor::Coordinates, CodeEditor::Coordinates>> CodeEditor::FindSubstrings(
	const Coordinates &aStart,
	const Coordinates &aEnd,
	const std::string &aPattern,
	bool aCaseSensitive
) {
	std::vector<std::pair<CodeEditor::Coordinates, CodeEditor::Coordinates>> occurrences{};

	std::string pattern(aPattern.begin(), aPattern.end());
	if (!aCaseSensitive) {
		for (size_t i = 0; i < aPattern.size(); i++)
			pattern[i] = std::tolower(pattern[i]);
	}


	// Substring searcher using the Boyer-Moore string-searching algorithm
	// (introduced in C++17 in <functional>; see https://en.cppreference.com/cpp/utility/functional/boyer_moore_searcher)
	// NOTE: boyer_moore_searcher has a preprocessing phase where it builds internal tables for caching. This makes performance worse if `FindSubstrings` is called on every user keystroke. The searcher should be cached.
	std::boyer_moore_searcher searcher(pattern.begin(), pattern.end());


	// For each line, find one or multiple patterns
	for (int lnIdx = aStart.mLine; lnIdx <= aEnd.mLine; lnIdx++) {
		auto &line = mLines[lnIdx];
		
		std::vector<char> searchStr{};
		searchStr.resize(line.size());
		for (size_t i = 0; i < line.size(); ++i)
			searchStr[i] = (aCaseSensitive)? line[i].mChar : std::tolower(line[i].mChar);


		// If either at the start or end line, the user might not be selecting the entire line, in which case, we only begin searching from the selection start until the selection end
		auto it = (lnIdx == aStart.mLine) ? (searchStr.begin() + GetCharacterIndex(aStart))	: searchStr.begin();
		auto end = (lnIdx == aEnd.mLine)  ? (searchStr.begin() + GetCharacterIndex(aEnd))	: searchStr.end();

		while (it != end) {
			it = std::search(it, searchStr.end(), searcher);
			
			int patternSz = static_cast<int>(pattern.size());

			if (it != end) {
				int startCol = GetCharacterColumn(lnIdx, std::distance(searchStr.begin(), it));
				occurrences.emplace_back(std::pair{
					Coordinates(lnIdx, startCol),
					Coordinates(lnIdx, startCol + patternSz)
				});

				// Advance the search start by the pattern's size
				// NOTE: Normally, we use std::advance for compatibility for both random-access and sequential STL containers.
				// However, std::advance can lead to undefined behavior if it advances the iterator beyond the end of the container.
				// Instead of manually checking, we use `std::ranges::advance` (C++20 feature), which advances an iterator to, colloquially, min(container.end(), it + N), and if an over-advance hapens, `ranges::advance` returns the overstep size.
				// https://en.cppreference.com/cpp/iterator/ranges/advance
				std::ranges::advance(it, patternSz, searchStr.end());
			}
		}
	}


	return occurrences;
}


std::vector<std::pair<CodeEditor::Coordinates, CodeEditor::Coordinates>> CodeEditor::FindSubstrings(const std::string &aPattern, bool aCaseSensitive) {
	if (mLines.empty())
		return {};

	auto docCoords = GetDocStartEndCoords();

	return FindSubstrings(docCoords.first, docCoords.second, aPattern, aCaseSensitive);
}


const CodeEditor::Palette &CodeEditor::GetDarkPalette() {
	using namespace ImGuiTheme;
	using namespace ImGuiUtils;

	const static Palette p = { {
			ImVec4ToImU32(DarkPalette::TEXT_LIGHT),                 // Default
			ImVec4ToImU32(DarkPalette::CODE_EDITOR_KEYWORD),        // Keyword
			ImVec4ToImU32(DarkPalette::CODE_EDITOR_NUMBER),			// Number
			ImVec4ToImU32(DarkPalette::CODE_EDITOR_STRING),         // String
			ImVec4ToImU32(DarkPalette::TEXT_MUTED),                 // Char literal
			ImVec4ToImU32(DarkPalette::TEXT_LIGHT),                 // Punctuation
			ImVec4ToImU32(DarkPalette::ACCENT_BLUE_DARK),           // Preprocessor (using accent blue)
			ImVec4ToImU32(DarkPalette::TEXT_LIGHT),					// Unknown Identifier
			ImVec4ToImU32(DarkPalette::CODE_EDITOR_IDENTIFIER),		// Known identifier
			ImVec4ToImU32(DarkPalette::ACCENT_BLUE_DARK_HOVER),     // Preproc identifier (using lighter accent)
			ImVec4ToImU32(DarkPalette::TEXT_MUTED),					// Comment (single line)
			ImVec4ToImU32(DarkPalette::TEXT_MUTED),					// Comment (multi line)
			ImVec4ToImU32(DarkPalette::DARK_GRAY_300),              // Background
			ImVec4ToImU32(DarkPalette::TEXT_LIGHT),                 // Cursor
			ImVec4ToImU32(DarkPalette::ACCENT_BLUE_DARK_ACTIVE),    // Selection (using an accent blue with some transparency)
			ImVec4ToImU32(ImVec4(1, 1, 0, 0.25)),					// Highlight (Yellow with 25% alpha)
			ImVec4ToImU32(DarkPalette::CODE_EDITOR_ERROR_MARKER),   // ErrorMarker (Red with 50% alpha)
			ImVec4ToImU32(DarkPalette::CODE_EDITOR_BREAKPOINT),		// Breakpoint (Orange with 25% alpha)
			ImVec4ToImU32(DarkPalette::DARK_GRAY_700),              // Line number
			ImVec4ToImU32(DarkPalette::DARK_GRAY_400),              // Current line fill (using a slightly lighter gray with some transparency)
			ImVec4ToImU32(DarkPalette::DARK_GRAY_500),              // Current line fill (inactive)
			ImVec4ToImU32(DarkPalette::DARK_GRAY_800),              // Current line edge
	} };
	return p;
}


const CodeEditor::Palette &CodeEditor::GetLightPalette() {
	using namespace ImGuiTheme;
	using namespace ImGuiUtils;

	const static Palette p = { {
		ImVec4ToImU32(LightPalette::TEXT_DARK),                 // Default (None)
		ImVec4ToImU32(LightPalette::CODE_EDITOR_KEYWORD),       // Keyword
		ImVec4ToImU32(LightPalette::CODE_EDITOR_NUMBER),		// Number
		ImVec4ToImU32(LightPalette::CODE_EDITOR_STRING),        // String
		ImVec4ToImU32(LightPalette::TEXT_MUTED_LIGHT),          // Char literal (same as string for now)
		ImVec4ToImU32(LightPalette::TEXT_DARK),                 // Punctuation
		ImVec4ToImU32(LightPalette::ACCENT_BLUE_LIGHT),         // Preprocessor
		ImVec4ToImU32(LightPalette::TEXT_DARK),					// Unknown Identifier
		ImVec4ToImU32(LightPalette::CODE_EDITOR_IDENTIFIER),	// Known identifier
		ImVec4ToImU32(LightPalette::ACCENT_BLUE_LIGHT_HOVER),   // Preproc identifier
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_700),            // Comment (single line) - a darker gray
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_700),            // Comment (multi line) - same as single line
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_300),            // Background
		ImVec4ToImU32(LightPalette::TEXT_DARK),                 // Cursor
		ImVec4ToImU32(LightPalette::ACCENT_BLUE_LIGHT_ACTIVE),  // Selection (using an accent blue with some transparency)
		ImVec4ToImU32(ImVec4(1, 1, 0, 0.25)),					// Highlight (Yellow with 25% alpha)
		ImVec4ToImU32(LightPalette::CODE_EDITOR_ERROR_MARKER),  // ErrorMarker (Red with 62.7% alpha, A0 is 160/255)
		ImVec4ToImU32(LightPalette::CODE_EDITOR_BREAKPOINT),    // Breakpoint (Orange with 50% alpha, 80 is 128/255)
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_700),            // Line number
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_400),            // Current line fill (using a slightly darker gray with some transparency)
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_500),            // Current line fill (inactive)
		ImVec4ToImU32(LightPalette::LIGHT_GRAY_800),            // Current line edge
	} };
	return p;
}


const CodeEditor::Palette &CodeEditor::GetRetroBluePalette() {
	const static Palette p = { {
			0xff00ffff,	// None
			0xffffff00,	// Keyword	
			0xff00ff00,	// Number
			0xff808000,	// String
			0xff808000, // Char literal
			0xffffffff, // Punctuation
			0xff008000,	// Preprocessor
			0xff00ffff, // Identifier
			0xffffffff, // Known identifier
			0xffff00ff, // Preproc identifier
			0xff808080, // Comment (single line)
			0xff404040, // Comment (multi line)
			0xff800000, // Background
			0xff0080ff, // Cursor
			0x80ffff00, // Selection
			0xa00000ff, // ErrorMarker
			0x80ff8000, // Breakpoint
			0xff808000, // Line number
			0x40000000, // Current line fill
			0x40808080, // Current line fill (inactive)
			0x40000000, // Current line edge
		} };
	return p;
}


std::string CodeEditor::GetText() const {
	auto [startCoords, endCoords] = GetDocStartEndCoords();
	return GetText(startCoords, endCoords);
}


std::vector<std::string> CodeEditor::GetTextLines() const {
	std::vector<std::string> result;

	result.reserve(mLines.size());

	for (auto &line : mLines)
	{
		std::string text;

		text.resize(line.size());

		for (size_t i = 0; i < line.size(); ++i)
			text[i] = line[i].mChar;

		result.emplace_back(std::move(text));
	}

	return result;
}


std::string CodeEditor::GetSelectedText() const {
	return GetText(mState.mSelectionStart, mState.mSelectionEnd);
}


std::string CodeEditor::GetCurrentLineText() const {
	auto lineLength = GetLineMaxColumn(mState.mCursorPosition.mLine);
	return GetText(
		Coordinates(mState.mCursorPosition.mLine, 0),
		Coordinates(mState.mCursorPosition.mLine, lineLength));
}


void CodeEditor::ProcessInputs() {}


void CodeEditor::Colorize(int aFromLine, int aLines) {
	int toLine = aLines == -1 ? (int)mLines.size() : std::min((int)mLines.size(), aFromLine + aLines);
	mColorRangeMin = std::min(mColorRangeMin, aFromLine);
	mColorRangeMax = std::max(mColorRangeMax, toLine);
	mColorRangeMin = std::max(0, mColorRangeMin);
	mColorRangeMax = std::max(mColorRangeMin, mColorRangeMax);
	mCheckComments = true;
}


void CodeEditor::ColorizeRange(int aFromLine, int aToLine) {
	if (mLines.empty() || aFromLine >= aToLine)
		return;

	std::string buffer;
	std::cmatch results;
	std::string id;

	int endLine = std::max(0, std::min((int)mLines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i)
	{
		auto &line = mLines[i];

		if (line.empty())
			continue;

		buffer.resize(line.size());
		for (size_t j = 0; j < line.size(); ++j)
		{
			auto &col = line[j];
			buffer[j] = col.mChar;
			col.mColorIndex = PaletteIndex::Default;
		}

		const char *bufferBegin = &buffer.front();
		const char *bufferEnd = bufferBegin + buffer.size();

		auto last = bufferEnd;

		for (auto first = bufferBegin; first != last; )
		{
			const char *token_begin = nullptr;
			const char *token_end = nullptr;
			PaletteIndex token_color = PaletteIndex::Default;

			bool hasTokenizeResult = false;

			if (mLanguageDefinition.mTokenize != nullptr)
			{
				if (mLanguageDefinition.mTokenize(first, last, token_begin, token_end, token_color))
					hasTokenizeResult = true;
			}

			if (hasTokenizeResult == false)
			{
				// todo : remove
				//printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

				for (auto &p : mRegexList)
				{
					if (std::regex_search(first, last, results, p.first, std::regex_constants::match_continuous))
					{
						hasTokenizeResult = true;

						auto &v = *results.begin();
						token_begin = v.first;
						token_end = v.second;
						token_color = p.second;
						break;
					}
				}
			}

			if (hasTokenizeResult == false)
			{
				first++;
			}
			else
			{
				const size_t token_length = token_end - token_begin;

				if (token_color == PaletteIndex::Identifier)
				{
					id.assign(token_begin, token_end);

					// todo : allmost all language definitions use lower case to specify keywords, so shouldn't this use ::tolower ?
					if (!mLanguageDefinition.mCaseSensitive)
						std::transform(id.begin(), id.end(), id.begin(), ::toupper);

					if (!line[first - bufferBegin].mPreprocessor)
					{
						if (mLanguageDefinition.mKeywords.count(id) != 0)
							token_color = PaletteIndex::Keyword;
						else if (mLanguageDefinition.mIdentifiers.count(id) != 0)
							token_color = PaletteIndex::KnownIdentifier;
						else if (mLanguageDefinition.mPreprocIdentifiers.count(id) != 0)
							token_color = PaletteIndex::PreprocIdentifier;
					}
					else
					{
						if (mLanguageDefinition.mPreprocIdentifiers.count(id) != 0)
							token_color = PaletteIndex::PreprocIdentifier;
					}
				}

				for (size_t j = 0; j < token_length; ++j)
					line[(token_begin - bufferBegin) + j].mColorIndex = token_color;

				first = token_end;
			}
		}
	}
}


void CodeEditor::ColorizeInternal() {
	if (mLines.empty() || !mColorizerEnabled)
		return;

	if (mCheckComments)
	{
		auto endLine = mLines.size();
		auto endIndex = 0;
		auto commentStartLine = endLine;
		auto commentStartIndex = endIndex;
		auto withinString = false;
		auto withinSingleLineComment = false;
		auto withinPreproc = false;
		auto firstChar = true;			// there is no other non-whitespace characters in the line before
		auto concatenate = false;		// '\' on the very end of the line
		auto currentLine = 0;
		auto currentIndex = 0;
		while (currentLine < endLine || currentIndex < endIndex)
		{
			auto &line = mLines[currentLine];

			if (currentIndex == 0 && !concatenate)
			{
				withinSingleLineComment = false;
				withinPreproc = false;
				firstChar = true;
			}

			concatenate = false;

			if (!line.empty())
			{
				auto &g = line[currentIndex];
				auto c = g.mChar;

				if (c != mLanguageDefinition.mPreprocChar && !isspace(c))
					firstChar = false;

				if (currentIndex == (int)line.size() - 1 && line[line.size() - 1].mChar == '\\')
					concatenate = true;

				bool inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

				if (withinString)
				{
					line[currentIndex].mMultiLineComment = inComment;

					if (c == '\"')
					{
						if (currentIndex + 1 < (int)line.size() && line[currentIndex + 1].mChar == '\"')
						{
							currentIndex += 1;
							if (currentIndex < (int)line.size())
								line[currentIndex].mMultiLineComment = inComment;
						}
						else
							withinString = false;
					}
					else if (c == '\\')
					{
						currentIndex += 1;
						if (currentIndex < (int)line.size())
							line[currentIndex].mMultiLineComment = inComment;
					}
				}
				else
				{
					if (firstChar && c == mLanguageDefinition.mPreprocChar)
						withinPreproc = true;

					if (c == '\"')
					{
						withinString = true;
						line[currentIndex].mMultiLineComment = inComment;
					}
					else
					{
						auto pred = [](const char &a, const Glyph &b) { return a == b.mChar; };
						auto from = line.begin() + currentIndex;
						auto &startStr = mLanguageDefinition.mCommentStart;
						auto &singleStartStr = mLanguageDefinition.mSingleLineComment;

						if (singleStartStr.size() > 0 &&
							currentIndex + singleStartStr.size() <= line.size() &&
							equals(singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(), pred))
						{
							withinSingleLineComment = true;
						}
						else if (!withinSingleLineComment && currentIndex + startStr.size() <= line.size() &&
							equals(startStr.begin(), startStr.end(), from, from + startStr.size(), pred))
						{
							commentStartLine = currentLine;
							commentStartIndex = currentIndex;
						}

						inComment = inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

						line[currentIndex].mMultiLineComment = inComment;
						line[currentIndex].mComment = withinSingleLineComment;

						auto &endStr = mLanguageDefinition.mCommentEnd;
						if (currentIndex + 1 >= (int)endStr.size() &&
							equals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred))
						{
							commentStartIndex = endIndex;
							commentStartLine = endLine;
						}
					}
				}
				line[currentIndex].mPreprocessor = withinPreproc;
				currentIndex += UTF8CharLength(c);
				if (currentIndex >= (int)line.size())
				{
					currentIndex = 0;
					++currentLine;
				}
			}
			else
			{
				currentIndex = 0;
				++currentLine;
			}
		}
		mCheckComments = false;
	}

	if (mColorRangeMin < mColorRangeMax)
	{
		const int increment = (mLanguageDefinition.mTokenize == nullptr) ? 10 : 10000;
		const int to = std::min(mColorRangeMin + increment, mColorRangeMax);
		ColorizeRange(mColorRangeMin, to);
		mColorRangeMin = to;

		if (mColorRangeMax == mColorRangeMin)
		{
			mColorRangeMin = std::numeric_limits<int>::max();
			mColorRangeMax = 0;
		}
		return;
	}
}


float CodeEditor::TextDistanceToLineStart(const Coordinates &aFrom) const {
	auto &line = mLines[aFrom.mLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom);
	for (size_t it = 0u; it < line.size() && it < colIndex; )
	{
		if (line[it].mChar == '\t')
		{
			distance = (1.0f + std::floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
			++it;
		}
		else
		{
			auto d = UTF8CharLength(line[it].mChar);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++)
				tempCString[i] = line[it].mChar;

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}


void CodeEditor::EnsureCursorVisible() {
	if (!mWithinRender) {
		mScrollToCursor = true;
		return;
	}

	float scrollX = ImGui::GetScrollX();
	float scrollY = ImGui::GetScrollY();

	auto height = ImGui::GetWindowHeight();
	auto width = ImGui::GetWindowWidth();

	auto top = 1 + (int)ceil(scrollY / mCharAdvance.y);
	auto bottom = (int)ceil((scrollY + height) / mCharAdvance.y);

	auto left = (int)ceil(scrollX / mCharAdvance.x);
	auto right = (int)ceil((scrollX + width) / mCharAdvance.x);

	auto pos = GetActualCursorCoordinates();
	auto len = TextDistanceToLineStart(pos);

	if (pos.mLine < top)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine - 1) * mCharAdvance.y));
	if (pos.mLine > bottom - 4)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine + 4) * mCharAdvance.y - height));
	if (len + mTextStart < left + 4)
		ImGui::SetScrollX(std::max(0.0f, len + mTextStart - 4));
	if (len + mTextStart > right - 4)
		ImGui::SetScrollX(std::max(0.0f, len + mTextStart + 4 - width));
}


int CodeEditor::GetPageSize() const {
	auto height = ImGui::GetWindowHeight() - 20.0f;
	return (int)floor(height / mCharAdvance.y);
}


CodeEditor::UndoRecord::UndoRecord(
	const std::string &aAdded,
	const CodeEditor::Coordinates aAddedStart,
	const CodeEditor::Coordinates aAddedEnd,
	const std::string &aRemoved,
	const CodeEditor::Coordinates aRemovedStart,
	const CodeEditor::Coordinates aRemovedEnd,
	CodeEditor::EditorState &aBefore,
	CodeEditor::EditorState &aAfter)
	: mAdded(aAdded)
	, mAddedStart(aAddedStart)
	, mAddedEnd(aAddedEnd)
	, mRemoved(aRemoved)
	, mRemovedStart(aRemovedStart)
	, mRemovedEnd(aRemovedEnd)
	, mBefore(aBefore)
	, mAfter(aAfter)
{
	assert(mAddedStart <= mAddedEnd);
	assert(mRemovedStart <= mRemovedEnd);
}


void CodeEditor::UndoRecord::Undo(CodeEditor *aEditor) {
	if (!mAdded.empty())
	{
		aEditor->DeleteRange(mAddedStart, mAddedEnd);
		aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 2);
	}

	if (!mRemoved.empty())
	{
		auto start = mRemovedStart;
		aEditor->InsertTextAt(start, mRemoved.c_str());
		aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 2);
	}

	aEditor->mState = mBefore;
	aEditor->EnsureCursorVisible();

}


void CodeEditor::UndoRecord::Redo(CodeEditor *aEditor) {
	if (!mRemoved.empty())
	{
		aEditor->DeleteRange(mRemovedStart, mRemovedEnd);
		aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 1);
	}

	if (!mAdded.empty())
	{
		auto start = mAddedStart;
		aEditor->InsertTextAt(start, mAdded.c_str());
		aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 1);
	}

	aEditor->mState = mAfter;
	aEditor->EnsureCursorVisible();
}


bool TokenizeYAMLIdentifier(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end) {
	const char *p = in_begin;

	// A scoped identifier must start with a letter or an underscore.
	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
	{
		p++;

		// Continue advancing the pointer as long as the characters are valid identifier characters.
		while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
		{
			p++;
		}

		// Now, check for the '::' sequence and subsequent identifiers.
		const char *last_valid_end = p;

		while (p + 2 <= in_end && p[0] == ':' && p[1] == ':')
		{
			// Move the pointer past the '::'.
			p += 2;
			const char *next_p = p;

			// Check if the next part is a valid identifier.
			if ((next_p < in_end) && ((*next_p >= 'a' && *next_p <= 'z') || (*next_p >= 'A' && *next_p <= 'Z') || *next_p == '_'))
			{
				next_p++;
				while ((next_p < in_end) && ((*next_p >= 'a' && *next_p <= 'z') || (*next_p >= 'A' && *next_p <= 'Z') || (*next_p >= '0' && *next_p <= '9') || *next_p == '_'))
				{
					next_p++;
				}

				// A valid identifier was found after '::', so we update our pointer and continue.
				p = next_p;
				last_valid_end = p;
			}
			else
			{
				// No valid identifier was found after '::'. This is an invalid sequence,
				// so we stop. The token ends at the end of the last valid identifier.
				break;
			}
		}

		// A valid token was found.
		out_begin = in_begin;
		out_end = last_valid_end;
		return true;
	}

	return false;
}


static bool TokenizeCStyleString(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end) {
	const char *p = in_begin;

	if (*p == '"')
	{
		p++;

		while (p < in_end)
		{
			// handle end of string
			if (*p == '"')
			{
				out_begin = in_begin;
				out_end = p + 1;
				return true;
			}

			// handle escape character for "
			if (*p == '\\' && p + 1 < in_end && p[1] == '"')
				p++;

			p++;
		}
	}

	return false;
}


static bool TokenizeCStyleCharacterLiteral(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end) {
	const char *p = in_begin;

	if (*p == '\'')
	{
		p++;

		// handle escape characters
		if (p < in_end && *p == '\\')
			p++;

		if (p < in_end)
			p++;

		// handle end of character literal
		if (p < in_end && *p == '\'')
		{
			out_begin = in_begin;
			out_end = p + 1;
			return true;
		}
	}

	return false;
}

static bool TokenizeCStyleIdentifier(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end)
{
	const char *p = in_begin;

	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
	{
		p++;

		while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
			p++;

		out_begin = in_begin;
		out_end = p;
		return true;
	}

	return false;
}


static bool TokenizeCStyleNumber(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end) {
	const char *p = in_begin;

	const bool startsWithNumber = *p >= '0' && *p <= '9';

	if (*p != '+' && *p != '-' && !startsWithNumber)
		return false;

	p++;

	bool hasNumber = startsWithNumber;

	while (p < in_end && (*p >= '0' && *p <= '9'))
	{
		hasNumber = true;

		p++;
	}

	if (hasNumber == false)
		return false;

	bool isFloat = false;
	bool isHex = false;
	bool isBinary = false;

	if (p < in_end)
	{
		if (*p == '.')
		{
			isFloat = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '9'))
				p++;
		}
		else if (*p == 'x' || *p == 'X')
		{
			// hex formatted integer of the type 0xef80

			isHex = true;

			p++;

			while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
				p++;
		}
		else if (*p == 'b' || *p == 'B')
		{
			// binary formatted integer of the type 0b01011101

			isBinary = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '1'))
				p++;
		}
	}

	if (isHex == false && isBinary == false)
	{
		// floating point exponent
		if (p < in_end && (*p == 'e' || *p == 'E'))
		{
			isFloat = true;

			p++;

			if (p < in_end && (*p == '+' || *p == '-'))
				p++;

			bool hasDigits = false;

			while (p < in_end && (*p >= '0' && *p <= '9'))
			{
				hasDigits = true;

				p++;
			}

			if (hasDigits == false)
				return false;
		}

		// single precision floating point type
		if (p < in_end && *p == 'f')
			p++;
	}

	if (isFloat == false)
	{
		// integer size type
		while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
			p++;
	}

	out_begin = in_begin;
	out_end = p;
	return true;
}


static bool TokenizeCStylePunctuation(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end) {
	(void)in_end;

	switch (*in_begin)
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '!':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '-':
	case '+':
	case '=':
	case '~':
	case '|':
	case '<':
	case '>':
	case '?':
	case ':':
	case '/':
	case ';':
	case ',':
	case '.':
		out_begin = in_begin;
		out_end = in_begin + 1;
		return true;
	}

	return false;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::YAML() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const std::string keywords[] = {
			YAMLFileConfig::ROOT,

			YAMLSimConfig::ROOT,
			YAMLSimConfig::Kernels,
			YAMLSimConfig::CoordSys,

			YAMLScene::ROOT,
			YAMLScene::Entity,
			YAMLScene::Entity_Components,
			YAMLScene::Entity_Components_Type,
			YAMLScene::Entity_Components_Type_Data
		};
		for (auto &k : keywords) {
			KeywordDescriptor id{};

			if (YAMLKeyDescription::Mapping.contains(k)) {
				const YAMLKeyDescription::KeyDescription &desc = YAMLKeyDescription::Mapping.at(k);

				id.mName = k;
				id.mDeclaration = desc.description;
				id.mExpectedType = desc.type;
			}

			langDef.mKeywords.insert(std::pair{ k, id });
		}


		static const std::string identifiers[] = {
			YAMLFileConfig::Version,
			YAMLFileConfig::Description,

			YAMLSimConfig::Kernel_Path,
			YAMLSimConfig::CoordSys_Frame,
			YAMLSimConfig::CoordSys_Epoch,
			YAMLSimConfig::CoordSys_EpochFormat,


			YAMLScene::Body_Sun,
			YAMLScene::Body_Mercury,
			YAMLScene::Body_Venus,
			YAMLScene::Body_Earth,
			YAMLScene::Body_Moon,
			YAMLScene::Body_Mars,
			YAMLScene::Body_Jupiter,
			YAMLScene::Body_Saturn,
			YAMLScene::Body_Uranus,
			YAMLScene::Body_Neptune,

			YAMLScene::Core_Identifiers,
			YAMLScene::Core_Transform,
			YAMLScene::Physics_RigidBody,
			YAMLScene::Physics_Propagator,
			YAMLScene::Physics_ShapeParameters,
			YAMLScene::Physics_OrbitalElements,
			YAMLScene::Spacecraft_Spacecraft,
			YAMLScene::Spacecraft_Thruster,
			YAMLScene::Render_MeshRenderable,


			YAMLData::Core_Identifiers_EntityType,
			YAMLData::Core_Identifiers_SpiceID,

			YAMLData::Core_Transform_Position,
			YAMLData::Core_Transform_Rotation,
			YAMLData::Core_Transform_Scale,

			YAMLData::Physics_RigidBody_Velocity,
			YAMLData::Physics_RigidBody_Acceleration,
			YAMLData::Physics_RigidBody_Mass,

			YAMLData::Physics_Propagator_PropagatorType,
			YAMLData::Physics_Propagator_TLEPath,

			YAMLData::Physics_ShapeParameters_EquatRadius,
			YAMLData::Physics_ShapeParameters_Flattening,
			YAMLData::Physics_ShapeParameters_GravParam,
			YAMLData::Physics_ShapeParameters_RotVelocity,
			YAMLData::Physics_ShapeParameters_J2,

			YAMLData::Physics_OrbitalElements_SemiMajorAxis,
			YAMLData::Physics_OrbitalElements_Eccentricity,
			YAMLData::Physics_OrbitalElements_Inclination,
			YAMLData::Physics_OrbitalElements_RAAN,
			YAMLData::Physics_OrbitalElements_ArgPeriapsis,
			YAMLData::Physics_OrbitalElements_TrueAnomaly,
			YAMLData::Physics_OrbitalElements_ParentBody,

			YAMLData::Spacecraft_Spacecraft_DragCoefficient,
			YAMLData::Spacecraft_Spacecraft_ReferenceArea,
			YAMLData::Spacecraft_Spacecraft_ReflectivityCoefficient,

			YAMLData::Spacecraft_Thruster_ThrustMagnitude,
			YAMLData::Spacecraft_Thruster_SpecificImpulse,
			YAMLData::Spacecraft_Thruster_CurrentFuelMass,
			YAMLData::Spacecraft_Thruster_MaxFuelMass,

			YAMLData::Render_MeshRenderable_MeshPath,
			YAMLData::Render_MeshRenderable_VisualScale
		};
		for (auto &k : identifiers) {
			IdentifierDescriptor id{};

			if (YAMLKeyDescription::Mapping.contains(k)) {
				const YAMLKeyDescription::KeyDescription &desc = YAMLKeyDescription::Mapping.at(k);

				id.mName = k;
				id.mDeclaration = desc.description;
				id.mSimUnit = desc.unit;
				id.mExpectedType = desc.type;
			}

			langDef.mIdentifiers.insert(std::pair{ k, id });
		}


		langDef.mTokenize = [](const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end, PaletteIndex &paletteIndex) -> bool
			{
				paletteIndex = PaletteIndex::Max;

				while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
					in_begin++;

				if (in_begin == in_end)
				{
					out_begin = in_end;
					out_end = in_end;
					paletteIndex = PaletteIndex::Default;
				}
				else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::String;
				else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::CharLiteral;
				else if (TokenizeYAMLIdentifier(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Identifier;
				else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Number;
				else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Punctuation;

				return paletteIndex != PaletteIndex::Max;
			};

		langDef.mCommentStart = "#";
		langDef.mCommentEnd = "#";
		langDef.mSingleLineComment = "#";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "YAML";

		inited = true;
	}
	return langDef;
}


// Ignore other language definitions
// They are kept only as references for custom-implemented definitions; may be outdated due to API changes, only reference as general guideline
#if 0
const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::CPlusPlus() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const cppKeywords[] = {
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
			"compl", "concept", "const", "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
			"for", "friend", "goto", "if", "import", "inline", "int", "long", "module", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
			"register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized", "template", "this", "thread_local",
			"throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
		};
		for (auto &k : cppKeywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "printf", "sprintf", "snprintf", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper",
			"std", "string", "vector", "map", "unordered_map", "set", "unordered_set", "min", "max"
		};
		for (auto &k : identifiers)
		{
			Identifier id{};
			id.mName = k;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenize = [](const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end, PaletteIndex &paletteIndex) -> bool
			{
				paletteIndex = PaletteIndex::Max;

				while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
					in_begin++;

				if (in_begin == in_end)
				{
					out_begin = in_end;
					out_end = in_end;
					paletteIndex = PaletteIndex::Default;
				}
				else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::String;
				else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::CharLiteral;
				else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Identifier;
				else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Number;
				else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Punctuation;

				return paletteIndex != PaletteIndex::Max;
			};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "C++";

		inited = true;
	}
	return langDef;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::HLSL() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const keywords[] = {
			"AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
			"CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
			"export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
			"linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
			"pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
			"RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
			"static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
			"Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
			"VertexShader", "void", "volatile", "while",
			"bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
			"uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
			"float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
			"float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
			"half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
			"half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
		};
		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
			"asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
			"ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
			"distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
			"f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
			"GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
			"InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
			"ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
			"Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
			"ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
			"radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
			"tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
			"tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
		};
		for (auto &k : identifiers)
		{
			Identifier id{};
			id.mName = k;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "HLSL";

		inited = true;
	}
	return langDef;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::GLSL() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (auto &k : identifiers)
		{
			Identifier id;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "GLSL";

		inited = true;
	}
	return langDef;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::C() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (auto &k : identifiers)
		{
			Identifier id{};
			id.mName = k;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenize = [](const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end, PaletteIndex &paletteIndex) -> bool
			{
				paletteIndex = PaletteIndex::Max;

				while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
					in_begin++;

				if (in_begin == in_end)
				{
					out_begin = in_end;
					out_end = in_end;
					paletteIndex = PaletteIndex::Default;
				}
				else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::String;
				else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::CharLiteral;
				else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Identifier;
				else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Number;
				else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Punctuation;

				return paletteIndex != PaletteIndex::Max;
			};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "C";

		inited = true;
	}
	return langDef;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::SQL() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const keywords[] = {
			"ADD", "EXCEPT", "PERCENT", "ALL", "EXEC", "PLAN", "ALTER", "EXECUTE", "PRECISION", "AND", "EXISTS", "PRIMARY", "ANY", "EXIT", "PRINT", "AS", "FETCH", "PROC", "ASC", "FILE", "PROCEDURE",
			"AUTHORIZATION", "FILLFACTOR", "PUBLIC", "BACKUP", "FOR", "RAISERROR", "BEGIN", "FOREIGN", "READ", "BETWEEN", "FREETEXT", "READTEXT", "BREAK", "FREETEXTTABLE", "RECONFIGURE",
			"BROWSE", "FROM", "REFERENCES", "BULK", "FULL", "REPLICATION", "BY", "FUNCTION", "RESTORE", "CASCADE", "GOTO", "RESTRICT", "CASE", "GRANT", "RETURN", "CHECK", "GROUP", "REVOKE",
			"CHECKPOINT", "HAVING", "RIGHT", "CLOSE", "HOLDLOCK", "ROLLBACK", "CLUSTERED", "IDENTITY", "ROWCOUNT", "COALESCE", "IDENTITY_INSERT", "ROWGUIDCOL", "COLLATE", "IDENTITYCOL", "RULE",
			"COLUMN", "IF", "SAVE", "COMMIT", "IN", "SCHEMA", "COMPUTE", "INDEX", "SELECT", "CONSTRAINT", "INNER", "SESSION_USER", "CONTAINS", "INSERT", "SET", "CONTAINSTABLE", "INTERSECT", "SETUSER",
			"CONTINUE", "INTO", "SHUTDOWN", "CONVERT", "IS", "SOME", "CREATE", "JOIN", "STATISTICS", "CROSS", "KEY", "SYSTEM_USER", "CURRENT", "KILL", "TABLE", "CURRENT_DATE", "LEFT", "TEXTSIZE",
			"CURRENT_TIME", "LIKE", "THEN", "CURRENT_TIMESTAMP", "LINENO", "TO", "CURRENT_USER", "LOAD", "TOP", "CURSOR", "NATIONAL", "TRAN", "DATABASE", "NOCHECK", "TRANSACTION",
			"DBCC", "NONCLUSTERED", "TRIGGER", "DEALLOCATE", "NOT", "TRUNCATE", "DECLARE", "NULL", "TSEQUAL", "DEFAULT", "NULLIF", "UNION", "DELETE", "OF", "UNIQUE", "DENY", "OFF", "UPDATE",
			"DESC", "OFFSETS", "UPDATETEXT", "DISK", "ON", "USE", "DISTINCT", "OPEN", "USER", "DISTRIBUTED", "OPENDATASOURCE", "VALUES", "DOUBLE", "OPENQUERY", "VARYING","DROP", "OPENROWSET", "VIEW",
			"DUMMY", "OPENXML", "WAITFOR", "DUMP", "OPTION", "WHEN", "ELSE", "OR", "WHERE", "END", "ORDER", "WHILE", "ERRLVL", "OUTER", "WITH", "ESCAPE", "OVER", "WRITETEXT"
		};

		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"ABS",  "ACOS",  "ADD_MONTHS",  "ASCII",  "ASCIISTR",  "ASIN",  "ATAN",  "ATAN2",  "AVG",  "BFILENAME",  "BIN_TO_NUM",  "BITAND",  "CARDINALITY",  "CASE",  "CAST",  "CEIL",
			"CHARTOROWID",  "CHR",  "COALESCE",  "COMPOSE",  "CONCAT",  "CONVERT",  "CORR",  "COS",  "COSH",  "COUNT",  "COVAR_POP",  "COVAR_SAMP",  "CUME_DIST",  "CURRENT_DATE",
			"CURRENT_TIMESTAMP",  "DBTIMEZONE",  "DECODE",  "DECOMPOSE",  "DENSE_RANK",  "DUMP",  "EMPTY_BLOB",  "EMPTY_CLOB",  "EXP",  "EXTRACT",  "FIRST_VALUE",  "FLOOR",  "FROM_TZ",  "GREATEST",
			"GROUP_ID",  "HEXTORAW",  "INITCAP",  "INSTR",  "INSTR2",  "INSTR4",  "INSTRB",  "INSTRC",  "LAG",  "LAST_DAY",  "LAST_VALUE",  "LEAD",  "LEAST",  "LENGTH",  "LENGTH2",  "LENGTH4",
			"LENGTHB",  "LENGTHC",  "LISTAGG",  "LN",  "LNNVL",  "LOCALTIMESTAMP",  "LOG",  "LOWER",  "LPAD",  "LTRIM",  "MAX",  "MEDIAN",  "MIN",  "MOD",  "MONTHS_BETWEEN",  "NANVL",  "NCHR",
			"NEW_TIME",  "NEXT_DAY",  "NTH_VALUE",  "NULLIF",  "NUMTODSINTERVAL",  "NUMTOYMINTERVAL",  "NVL",  "NVL2",  "POWER",  "RANK",  "RAWTOHEX",  "REGEXP_COUNT",  "REGEXP_INSTR",
			"REGEXP_REPLACE",  "REGEXP_SUBSTR",  "REMAINDER",  "REPLACE",  "ROUND",  "ROWNUM",  "RPAD",  "RTRIM",  "SESSIONTIMEZONE",  "SIGN",  "SIN",  "SINH",
			"SOUNDEX",  "SQRT",  "STDDEV",  "SUBSTR",  "SUM",  "SYS_CONTEXT",  "SYSDATE",  "SYSTIMESTAMP",  "TAN",  "TANH",  "TO_CHAR",  "TO_CLOB",  "TO_DATE",  "TO_DSINTERVAL",  "TO_LOB",
			"TO_MULTI_BYTE",  "TO_NCLOB",  "TO_NUMBER",  "TO_SINGLE_BYTE",  "TO_TIMESTAMP",  "TO_TIMESTAMP_TZ",  "TO_YMINTERVAL",  "TRANSLATE",  "TRIM",  "TRUNC", "TZ_OFFSET",  "UID",  "UPPER",
			"USER",  "USERENV",  "VAR_POP",  "VAR_SAMP",  "VARIANCE",  "VSIZE "
		};
		for (auto &k : identifiers)
		{
			Identifier id;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = false;
		langDef.mAutoIndentation = false;

		langDef.mName = "SQL";

		inited = true;
	}
	return langDef;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::AngelScript() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const keywords[] = {
			"and", "abstract", "auto", "bool", "break", "case", "cast", "class", "const", "continue", "default", "do", "double", "else", "enum", "false", "final", "float", "for",
			"from", "funcdef", "function", "get", "if", "import", "in", "inout", "int", "interface", "int8", "int16", "int32", "int64", "is", "mixin", "namespace", "not",
			"null", "or", "out", "override", "private", "protected", "return", "set", "shared", "super", "switch", "this ", "true", "typedef", "uint", "uint8", "uint16", "uint32",
			"uint64", "void", "while", "xor"
		};

		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"cos", "sin", "tab", "acos", "asin", "atan", "atan2", "cosh", "sinh", "tanh", "log", "log10", "pow", "sqrt", "abs", "ceil", "floor", "fraction", "closeTo", "fpFromIEEE", "fpToIEEE",
			"complex", "opEquals", "opAddAssign", "opSubAssign", "opMulAssign", "opDivAssign", "opAdd", "opSub", "opMul", "opDiv"
		};
		for (auto &k : identifiers)
		{
			Identifier id{};
			id.mName = k;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "AngelScript";

		inited = true;
	}
	return langDef;
}


const CodeEditor::LanguageDefinition &CodeEditor::LanguageDefinition::Lua() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char *const keywords[] = {
			"and", "break", "do", "", "else", "elseif", "end", "false", "for", "function", "if", "in", "", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
		};

		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "loadfile", "load", "loadstring",  "next",  "pairs",  "pcall",  "print",  "rawequal",  "rawlen",  "rawget",  "rawset",
			"select",  "setmetatable",  "tonumber",  "tostring",  "type",  "xpcall",  "_G",  "_VERSION","arshift", "band", "bnot", "bor", "bxor", "btest", "extract", "lrotate", "lshift", "replace",
			"rrotate", "rshift", "create", "resume", "running", "status", "wrap", "yield", "isyieldable", "debug","getuservalue", "gethook", "getinfo", "getlocal", "getregistry", "getmetatable",
			"getupvalue", "upvaluejoin", "upvalueid", "setuservalue", "sethook", "setlocal", "setmetatable", "setupvalue", "traceback", "close", "flush", "input", "lines", "open", "output", "popen",
			"read", "tmpfile", "type", "write", "close", "flush", "lines", "read", "seek", "setvbuf", "write", "__gc", "__tostring", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "tointeger",
			"floor", "fmod", "ult", "log", "max", "min", "modf", "rad", "random", "randomseed", "sin", "sqrt", "string", "tan", "type", "atan2", "cosh", "sinh", "tanh",
			"pow", "frexp", "ldexp", "log10", "pi", "huge", "maxinteger", "mininteger", "loadlib", "searchpath", "seeall", "preload", "cpath", "path", "searchers", "loaded", "module", "require", "clock",
			"date", "difftime", "execute", "exit", "getenv", "remove", "rename", "setlocale", "time", "tmpname", "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep",
			"reverse", "sub", "upper", "pack", "packsize", "unpack", "concat", "maxn", "insert", "pack", "unpack", "remove", "move", "sort", "offset", "codepoint", "char", "len", "codes", "charpattern",
			"coroutine", "table", "io", "os", "string", "utf8", "bit32", "math", "debug", "package"
		};
		for (auto &k : identifiers)
		{
			Identifier id;
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "--[[";
		langDef.mCommentEnd = "]]";
		langDef.mSingleLineComment = "--";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = false;

		langDef.mName = "Lua";

		inited = true;
	}
	return langDef;
}
#endif