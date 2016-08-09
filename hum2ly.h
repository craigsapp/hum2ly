//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Aug  6 10:53:40 CEST 2016
// Last Modified: Tue Aug  9 12:43:50 CEST 2016
// Filename:      hum2ly.h
// URL:           https://github.com/craigsapp/hum2ly/blob/master/hum2ly.h
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// Description:   Convert a Humdrum file into a lilypond file.
//

#ifndef _HUM2LY_H
#define _HUM2LY_H

#define _USE_HUMLIB_OPTIONS_
#include "humlib.h"

#include <iostream>
#include <math.h>

namespace hum {

using namespace std;


//////////////////////////////
//
// StateVariables -- This helper class keeps track of various
//    data states, such as for expressing music in \relative form.
//

class StateVariables {
	public:
		StateVariables(void) { clear(); }
		~StateVariables() { clear(); }
		void clear();

		HumNum duration;  // duration of last note/chord/rest
		int dots;         // augmentation dots of last note/chord/rest
		int pitch;        // pitch of previous note
		int cpitch;       // pitch of previous note in chord
		vector<int> chordpitches; // pitches of last chord (for "q" shortcut)
};



//////////////////////////////
//
// HumdrumToLilypondConverter -- The main class for converting Humdrum data
//    into Lilypond data.
//

class HumdrumToLilypondConverter {
	public:
		HumdrumToLilypondConverter(void);
		~HumdrumToLilypondConverter() {}

		bool    convert              (ostream& out, HumdrumFile& infile);
		bool    convert              (ostream& out, const string& input);
		bool    convert              (ostream& out, istream& input);
		void    setIndent            (const string& indent)
		                                   { m_indent = indent; }
		void    setOptions           (int argc, char** argv);
		void    setOptions           (const vector<string>& argvlist);
		Options getOptionDefinitions (void);

	protected:
		bool convert          (ostream& out);
		bool convertPart      (ostream& out, const string& partname,
		                       int partindex);
		void extractSegments  (void);
		bool convertSegment   (ostream& out, int partindex, int startline,
		                       int endline);
		void printHeaderComments(ostream& out);
		void printFooterComments(ostream& out);
		bool convertPartSegment(ostream& out, HTp starttoken, int endline);
		bool convertDataToken (ostream& out, HTp token);
		bool convertRest      (ostream& out, HTp token);
		bool convertChord     (ostream& out, HTp token);
		bool convertNote      (ostream& out, HTp token, int index = 0);
		int  printRelativeStartingPitch(ostream& out, int partindex,
		                       int startline, int endline);
		vector<HTp> getStartTokens(int partindex, int startline, int endline);
		int getSegmentStartingPitch(int partindex, int startline, int endline);
		int characterCount    (const string &text, char symbol);
		void convertDuration  (ostream& out, HumNum& duration, int dots);
		void convertDuration  (ostream& out, HumNum& durationnodots,
		                       HumNum& duration, int dots);
		string arabicToRomanNumeral(int arabic, int casetype = 1);
		bool convertInterpretationToken(ostream& out, HTp token);
		void addErrorMessage  (const string& message, HTp token = NULL);
		void printErrorMessages(ostream& out);
		bool convertClef      (ostream& out, HTp token);
		HTp  getKeyDesignation (HTp token);
		bool convertKeySignature(ostream& out, HTp token);
		void printHeader      (ostream& tempout);
		void convertArticulations(ostream& out, const string& stok);

	private:
		vector<HTp>     m_kernstarts;  // part to track mapping
		vector<int>     m_rkern;       // track to part mapping
		vector<int>     m_segments;    // line index for start of each segement
		vector<string>  m_labels;      // starting label for each segement
		HumdrumFile     m_infile;      // Humdrum file to convert
		string          m_indent;      // whitespace for each indenting levels
		stringstream    m_staffout;    // staff assembly output
		stringstream    m_scoreout;    // score assembly output
		StateVariables  m_states;      // keep track of pitch/rhythm changes
		Options         m_options;     // command-line options
		vector<string>  m_errors;      // storage for conversion errors
};


}  // end of namespace hum


#endif /* _HUM2LY_H */



