/* CodeEditor.hpp - Implementation of the code editor for Astrocelerate.

	The author of the original source code for this code editor is BalazsJako.
	GitHub repository: https://github.com/BalazsJako/ImGuiColorTextEdit/tree/master

	MIT License

	Copyright (c) 2017 BalazsJako

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#pragma once

#define NOMINMAX	// minwindef.h keeps overriding std::min and std::max definitions provided by the algorithms header for some reason, so we have to define this macro to disable minwindef.h definitions.

#include <map>
#include <array>
#include <regex>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#include <iconfontcppheaders/IconsFontAwesome6.h>


#include <Core/Data/Mapping/YAMLKeys.hpp>

#include <Engine/GUI/Data/Appearance.hpp>
#include <Engine/Utils/ImGuiUtils.hpp>


class CodeEditor
{
public:
	enum class PaletteIndex
	{
		Default,
		Keyword,
		Number,
		String,
		CharLiteral,
		Punctuation,
		Preprocessor,
		Identifier,
		KnownIdentifier,
		PreprocIdentifier,
		Comment,
		MultiLineComment,
		Background,
		Cursor,
		Selection,
		Highlight,
		ErrorMarker,
		Breakpoint,
		LineNumber,
		CurrentLineFill,
		CurrentLineFillInactive,
		CurrentLineEdge,
		Max
	};

	enum class SelectionMode
	{
		Normal,
		Word,
		Line
	};

	struct Breakpoint
	{
		int mLine;
		bool mEnabled;
		std::string mCondition;

		Breakpoint()
			: mLine(-1)
			, mEnabled(false)
		{
		}
	};

	// Represents a character coordinate from the user's point of view,
	// i. e. consider an uniform grid (assuming fixed-width font) on the
	// screen as it is rendered, and each cell has its own coordinate, starting from 0.
	// Tabs are counted as [1..mTabSize] count empty spaces, depending on
	// how many space is necessary to reach the next tab stop.
	// For example, coordinate (1, 5) represents the character 'B' in a line "\tABC", when mTabSize = 4,
	// because it is rendered as "    ABC" on the screen.
	struct Coordinates
	{
		int mLine, mColumn;
		Coordinates() : mLine(0), mColumn(0) {}
		Coordinates(int aLine, int aColumn) : mLine(aLine), mColumn(aColumn)
		{
			assert(aLine >= 0);
			assert(aColumn >= 0);
		}
		static Coordinates Invalid() { static Coordinates invalid(-1, -1); return invalid; }

		bool operator ==(const Coordinates &o) const
		{
			return
				mLine == o.mLine &&
				mColumn == o.mColumn;
		}

		bool operator !=(const Coordinates &o) const
		{
			return
				mLine != o.mLine ||
				mColumn != o.mColumn;
		}

		bool operator <(const Coordinates &o) const
		{
			if (mLine != o.mLine)
				return mLine < o.mLine;
			return mColumn < o.mColumn;
		}

		bool operator >(const Coordinates &o) const
		{
			if (mLine != o.mLine)
				return mLine > o.mLine;
			return mColumn > o.mColumn;
		}

		bool operator <=(const Coordinates &o) const
		{
			if (mLine != o.mLine)
				return mLine < o.mLine;
			return mColumn <= o.mColumn;
		}

		bool operator >=(const Coordinates &o) const
		{
			if (mLine != o.mLine)
				return mLine > o.mLine;
			return mColumn >= o.mColumn;
		}
	};

	struct IdentifierDescriptor {
		Coordinates mLocation;
		std::string mName;
		std::string mDeclaration;
		std::optional<std::string> mSimUnit = std::nullopt;
		std::optional<YAMLKeyDescription::KeyDescription::Type> mExpectedType = std::nullopt;

		IdentifierDescriptor(const std::string &defaultName = "Identifier/Attribute", const std::string &defaultDeclaration = "Generic Identifier/Attribute") :
			mName(defaultName),
			mDeclaration(defaultDeclaration)
		{}

		std::string TypeToString() const {
			if (!mExpectedType.has_value())
				return "";

			using enum YAMLKeyDescription::KeyDescription::Type;
			switch (mExpectedType.value()) {
			case SCALAR_STRING:
				return "String";
			case SCALAR_NUMBER:
				return "Number";
			case SCALAR_BOOL:
				return "Boolean";
			case SEQUENCE:
				return "YAML Sequence";
			case SEQUENCE_ARRAY_3:
				return "3-element Vector";
			case SEQUENCE_ARRAY_4:
				return "4-element Vector";
			case MAPPING:
				return "YAML Mapping";
			default:
				return "";
			}
		}
	};

	struct KeywordDescriptor : IdentifierDescriptor {
		KeywordDescriptor() : IdentifierDescriptor("Keyword", "Generic Keyword") {}
	};

	typedef std::string String;
	typedef std::unordered_map<std::string, IdentifierDescriptor> Identifiers;
	typedef std::unordered_map<std::string, KeywordDescriptor> Keywords;
	typedef std::map<int, std::pair<std::string, std::string>> ErrorMarkers;  // ErrorMarkers = map<Line, pair<Title, Message>>
	typedef std::unordered_set<int> Breakpoints;
	typedef std::array<ImU32, (unsigned)PaletteIndex::Max> Palette;
	typedef uint8_t Char;

	struct Glyph
	{
		Char mChar;
		PaletteIndex mColorIndex = PaletteIndex::Default;
		bool mComment : 1;
		bool mMultiLineComment : 1;
		bool mPreprocessor : 1;

		Glyph(Char aChar, PaletteIndex aColorIndex) : mChar(aChar), mColorIndex(aColorIndex),
			mComment(false), mMultiLineComment(false), mPreprocessor(false) {
		}
	};

	typedef std::vector<Glyph> Line;
	typedef std::vector<Line> Lines;

	struct LanguageDefinition
	{
		typedef std::pair<std::string, PaletteIndex> TokenRegexString;
		typedef std::vector<TokenRegexString> TokenRegexStrings;
		typedef bool(*TokenizeCallback)(const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end, PaletteIndex &paletteIndex);

		std::string mName;
		Keywords mKeywords;
		Identifiers mIdentifiers;
		Identifiers mPreprocIdentifiers;
		std::string mCommentStart, mCommentEnd, mSingleLineComment;
		char mPreprocChar;
		bool mAutoIndentation;

		TokenizeCallback mTokenize;

		TokenRegexStrings mTokenRegexStrings;

		bool mCaseSensitive;

		LanguageDefinition()
			: mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true)
		{
		}

		static const LanguageDefinition &YAML();

#if 0
		static const LanguageDefinition &CPlusPlus();
		static const LanguageDefinition &HLSL();
		static const LanguageDefinition &GLSL();
		static const LanguageDefinition &C();
		static const LanguageDefinition &SQL();
		static const LanguageDefinition &AngelScript();
		static const LanguageDefinition &Lua();
#endif
	};

	CodeEditor();
	~CodeEditor() = default;

	void SetLanguageDefinition(const LanguageDefinition &aLanguageDef);
	const LanguageDefinition &GetLanguageDefinition() const { return mLanguageDefinition; }

	const Palette &GetPalette() const { return mPaletteBase; }
	void SetPalette(const Palette &aValue);

	void SetErrorMarkers(const ErrorMarkers &aMarkers) { mErrorMarkers = aMarkers; }
	const ErrorMarkers &GetErrorMarkers() const { return mErrorMarkers; }

	void SetBreakpoints(const Breakpoints &aMarkers) { mBreakpoints = aMarkers; }

	void Render(const char *aTitle, const ImVec2 &aSize = ImVec2(), bool aBorder = false);
	void SetText(const std::string &aText);
	std::string GetText() const;

	void SetTextLines(const std::vector<std::string> &aLines);
	std::vector<std::string> GetTextLines() const;

	std::string GetSelectedText() const;
	std::string GetCurrentLineText()const;

	int GetTotalLines() const { return (int)mLines.size(); }
	bool IsOverwrite() const { return mOverwrite; }

	void SetReadOnly(bool aValue);
	bool IsReadOnly() const { return mReadOnly; }
	void SetTextChanged(bool aValue) { mTextChanged = aValue; }
	bool IsTextChanged() const { return mTextChanged; }
	bool IsCursorPositionChanged() const { return mCursorPositionChanged; }

	bool IsColorizerEnabled() const { return mColorizerEnabled; }
	void SetColorizerEnable(bool aValue);

	Coordinates GetCursorPosition() const { return GetActualCursorCoordinates(); }
	void SetCursorPosition(const Coordinates &aPosition);

	inline void SetHandleMouseInputs(bool aValue) { mHandleMouseInputs = aValue; }
	inline bool IsHandleMouseInputsEnabled() const { return mHandleKeyboardInputs; }

	inline void SetHandleKeyboardInputs(bool aValue) { mHandleKeyboardInputs = aValue; }
	inline bool IsHandleKeyboardInputsEnabled() const { return mHandleKeyboardInputs; }

	inline void SetImGuiChildIgnored(bool aValue) { mIgnoreImGuiChild = aValue; }
	inline bool IsImGuiChildIgnored() const { return mIgnoreImGuiChild; }

	inline void SetShowWhitespaces(bool aValue) { mShowWhitespaces = aValue; }
	inline bool IsShowingWhitespaces() const { return mShowWhitespaces; }

	void SetTabSize(int aValue);
	inline int GetTabSize() const { return mTabSize; }

	void InsertText(const std::string &aValue);
	void InsertText(const char *aValue);

	void MoveUp(int aAmount = 1, bool aSelect = false);
	void MoveDown(int aAmount = 1, bool aSelect = false);
	void MoveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveRight(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveTop(bool aSelect = false);
	void MoveBottom(bool aSelect = false);
	void MoveHome(bool aSelect = false);
	void MoveEnd(bool aSelect = false);

	void SetSelectionStart(const Coordinates &aPosition);
	void SetSelectionEnd(const Coordinates &aPosition);
	void SetSelection(const Coordinates &aStart, const Coordinates &aEnd, SelectionMode aMode = SelectionMode::Normal);
	void SelectWordUnderCursor();
	void SelectAll();
	bool HasSelection() const;

	inline void AddHighlight(const Coordinates &aStart, const Coordinates &aEnd) { mHighlights.emplace_back(std::pair{ aStart, aEnd }); };
	void DeleteHighlight(const Coordinates &aStart, const Coordinates &aEnd);
	inline void ClearHighlights() { mHighlights.clear(); };

	void Copy();
	void Cut();
	void Paste();
	void Delete();

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo(int aSteps = 1);
	void Redo(int aSteps = 1);

	void IndentSpaces();
	void UnindentSpaces();

	void HandleCommentInline();
	void CommentInline();
	void UncommentInline();

	inline void SetShowFindReplaceWindow(bool aValue) { mShowFindReplaceWindow = aValue; };

	void JumpToPosition(const Coordinates &aPosition);

	/* Finds all occurrences of a specified pattern from specified start to end coordinates; returns a vector of start-end coordinate pairs. */
	std::vector<std::pair<Coordinates, Coordinates>> FindSubstrings(const Coordinates &aStart, const Coordinates &aEnd, const std::string &aPattern, bool aCaseSensitive);
	std::vector<std::pair<Coordinates, Coordinates>> FindSubstrings(const std::string &aPattern, bool aCaseSensitive);  // Find substrings within entire document

	static const Palette &GetDarkPalette();
	static const Palette &GetLightPalette();
	static const Palette &GetRetroBluePalette();

private:
	typedef std::vector<std::pair<std::regex, PaletteIndex>> RegexList;

	struct EditorState
	{
		Coordinates mSelectionStart;
		Coordinates mSelectionEnd;
		Coordinates mCursorPosition;
	};

	class UndoRecord
	{
	public:
		UndoRecord() = default;
		~UndoRecord() = default;

		UndoRecord(
			const std::string &aAdded,
			const CodeEditor::Coordinates aAddedStart,
			const CodeEditor::Coordinates aAddedEnd,

			const std::string &aRemoved,
			const CodeEditor::Coordinates aRemovedStart,
			const CodeEditor::Coordinates aRemovedEnd,

			CodeEditor::EditorState &aBefore,
			CodeEditor::EditorState &aAfter);

		void Undo(CodeEditor *aEditor);
		void Redo(CodeEditor *aEditor);

		std::string mAdded;
		Coordinates mAddedStart;
		Coordinates mAddedEnd;

		std::string mRemoved;
		Coordinates mRemovedStart;
		Coordinates mRemovedEnd;

		EditorState mBefore;
		EditorState mAfter;
	};

	typedef std::vector<UndoRecord> UndoBuffer;

	void ProcessInputs();
	void Colorize(int aFromLine = 0, int aCount = -1);
	void ColorizeRange(int aFromLine = 0, int aToLine = 0);
	void ColorizeInternal();
	float TextDistanceToLineStart(const Coordinates &aFrom) const;
	void EnsureCursorVisible();
	int GetPageSize() const;
	std::string GetText(const Coordinates &aStart, const Coordinates &aEnd) const;
	Coordinates GetActualCursorCoordinates() const;
	Coordinates SanitizeCoordinates(const Coordinates &aValue) const;
	void Advance(Coordinates &aCoordinates) const;
	void DeleteRange(const Coordinates &aStart, const Coordinates &aEnd);
	int InsertTextAt(Coordinates &aWhere, const char *aValue);
	void AddUndo(UndoRecord &aValue);
	Coordinates ScreenPosToCoordinates(const ImVec2 &aPosition) const;
	Coordinates FindWordStart(const Coordinates &aFrom) const;
	Coordinates FindWordEnd(const Coordinates &aFrom) const;
	Coordinates FindNextWord(const Coordinates &aFrom) const;
	int GetCharacterIndex(const Coordinates &aCoordinates) const;
	int GetCharacterColumn(int aLine, int aIndex) const;
	int GetLineCharacterCount(int aLine) const;
	int GetLineMaxColumn(int aLine) const;
	bool IsOnWordBoundary(const Coordinates &aAt) const;
	void RemoveLine(int aStart, int aEnd);
	void RemoveLine(int aIndex);
	Line &InsertLine(int aIndex);
	void EnterCharacter(ImWchar aChar, bool aShift);
	void Backspace();
	void DeleteSelection();
	std::string GetWordUnderCursor() const;
	std::string GetWordAt(const Coordinates &aCoords) const;
	ImU32 GetGlyphColor(const Glyph &aGlyph) const;

	std::pair<Coordinates, Coordinates> GetDocStartEndCoords() const;

	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void Render();

	float mLineSpacing;
	Lines mLines;
	EditorState mState;
	UndoBuffer mUndoBuffer;
	int mUndoIndex;

	int mTabSize;
	bool mOverwrite;
	bool mReadOnly;
	bool mWithinRender;
	bool mScrollToCursor;
	bool mScrollToTop;
	bool mTextChanged;
	bool mColorizerEnabled;
	float mTextStart;                   // position (in pixels) where a code line starts relative to the left of the CodeEditor.
	int  mLeftMargin;
	bool mCursorPositionChanged;
	int mColorRangeMin, mColorRangeMax;
	SelectionMode mSelectionMode;
	bool mHandleKeyboardInputs;
	bool mHandleMouseInputs;
	bool mIgnoreImGuiChild;
	bool mShowWhitespaces;


	bool mShowFindReplaceWindow;
	bool mFindReplaceWindowFocused = false;
	std::string mFindBuffer;
	std::string mReplaceBuffer;

	std::vector<std::pair<Coordinates, Coordinates>> mHighlights;

	Palette mPaletteBase;
	Palette mPalette;
	LanguageDefinition mLanguageDefinition;
	RegexList mRegexList;

	bool mCheckComments;
	Breakpoints mBreakpoints;
	ErrorMarkers mErrorMarkers;
	ImVec2 mCharAdvance;
	Coordinates mInteractiveStart, mInteractiveEnd;
	std::string mLineBuffer;
	uint64_t mStartTime;

	float mLastClick;
};