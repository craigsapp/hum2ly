//
// Programmer:	Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Aug  6 10:53:40 CEST 2016
// Last Modified: Sat Aug  6 10:53:43 CEST 2016
// Filename:	  Convert-string.h
// URL:	       https://github.com/craigsapp/hum2ly/blob/master/hum2ly.h
// Syntax:	    C++11
// vim:	       ts=3 noexpandtab
//
// Description:   Converts a Humdrum file into a lilypond file.
//

#define _USE_HUMLIB_OPTIONS_
#include "humlib.h"
#include <iostream>
#include <math.h>

using namespace std;

namespace hum {


class StateVariables {
	public:
		StateVariables(void) { clear(); }
		~StateVariables() { clear(); }
		void clear();

		HumNum duration;  // duration of last note/rest
		int dots;         // augmentation dots of last note/rest
		int pitch;        // pitch of last note
};

void StateVariables::clear(void) {
	duration = -1;
	dots = -1;
	pitch = -99999;
}


class HumdrumToLilypondConverter {
	public:
		HumdrumToLilypondConverter(void);
		~HumdrumToLilypondConverter() {}

		bool convert          (ostream& out, HumdrumFile& infile);
		bool convert          (ostream& out, const string& input);
		bool convert          (ostream& out, istream& input);
		void setIndent        (const string& indent) { m_indent = indent; }
		void setOptions       (int argc, char** argv);

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
		void convertDuration    (ostream& out, HumNum& duration, int dots);
		void convertDuration    (ostream& out, HumNum& durationnodots,
		                       HumNum& duration, int dots);
		string arabicToRomanNumeral(int arabic, int casetype = 1);
		bool convertInterpretationToken(ostream& out, HTp token);
		void addErrorMessage  (const string& message, HTp token = NULL);
		void printErrorMessages(ostream& out);
		bool convertClef      (ostream& out, HTp token);
		HTp getKeyDesignation (HTp token);
		bool convertKeySignature(ostream& out, HTp token);
		void printHeader      (ostream& tempout);
		void convertArticulations(ostream& out, const string& stok);

	private:
		vector<HTp>     m_kernstarts;
		vector<int>     m_rkern;
		vector<int>     m_segments;
		vector<string>  m_labels;
		HumdrumFile     m_infile;
		string          m_indent;
		stringstream    m_staffout;
		stringstream    m_scoreout;
		StateVariables  m_states;
		Options         m_options;
		vector<string>  m_errors;
};


}


///////////////////////////////////////////////////////////////////////////

#define _INCLUDE_HUM2LY_MAIN_
#ifdef _INCLUDE_HUM2LY_MAIN_

int main(int argc, char** argv) {
	hum::Options options;
	options.define("v|version=s:2.18.2", "lilypond version");
	options.process(argc, argv);

	hum::HumdrumFile infile;
	if (options.getArgCount() == 0) {
		infile.read(cin);
	} else {
		infile.read(options.getArg(1));
	}

	hum::HumdrumToLilypondConverter converter;
	converter.setOptions(argc, argv);
	stringstream out;
	bool status = converter.convert(out, infile);
	if (status) {
		cout << out.str();
	}

	exit(0);
}

#endif /* _INCLUDE_HUM2LY_MAIN_ */

///////////////////////////////////////////////////////////////////////////


namespace hum {


//////////////////////////////
//
// HumdumToLilypondConverter::setOptions --
//

void HumdrumToLilypondConverter::setOptions(int argc, char** argv) {
	m_options.process(argc, argv);
}




//////////////////////////////
//
// HumdrumToLilypondConverter::HumdrumToLilypondConverter -- Construtor.
//

HumdrumToLilypondConverter::HumdrumToLilypondConverter(void) {
	m_options.define("v|version=s:2.18.2", "lilypond version");
	m_indent = "  ";
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convert -- Convert a Humdrum file into
//    lilypond content.
//

bool HumdrumToLilypondConverter::convert(ostream& out, HumdrumFile& infile) {
	m_infile = infile;
	return convert(out);
}


bool HumdrumToLilypondConverter::convert(ostream& out, istream& input) {
	m_infile.read(input);
	return convert(out);
}


bool HumdrumToLilypondConverter::convert(ostream& out, const string& input) {
	m_infile.read(input.c_str());
	return convert(out);
}


bool HumdrumToLilypondConverter::convert(ostream& out) {
	HumdrumFile& infile = m_infile;
   stringstream tempout;
	bool status = true; // for keeping track of problems in conversion process.

	printHeaderComments(tempout);

	tempout << "\\version \"" << m_options.getString("version") << "\"\n\n";

	printHeader(tempout);

	// Create a list of the parts and which spine represents them.
	vector<HTp>& kernstarts = m_kernstarts;
	kernstarts = infile.getKernSpineStartList();

	if (kernstarts.size() == 0) {
		// no parts in file, give up.  Perhaps return an error.
		return status;
	}

	// Reverse the order, since top part is last spine.
	reverse(kernstarts.begin(), kernstarts.end());
	vector<int>& rkern = m_rkern;
	rkern.resize(infile.getSpineCount() + 1);
	std::fill(rkern.begin(), rkern.end(), -1);
	for (int i=0; i<(int)kernstarts.size(); i++) {
		rkern[kernstarts[i]->getTrack()] = i;
	}

	extractSegments();

	m_scoreout << "\\score {\n";
	m_scoreout << m_indent << "<<\n";

	string partname;
	for (int i=0; i<(int)kernstarts.size(); i++) {
		partname = "part" + arabicToRomanNumeral(i+1);
		m_staffout << partname << " = \\new Staff {\n" << m_indent;
		m_scoreout << m_indent << "{ \\" << partname << " }\n";
		status &= convertPart(tempout, partname, i);
		m_staffout << "\n}\n\n";
		if (!status) {
			break;
		}
	}

	m_scoreout << m_indent << ">>\n";
	m_scoreout << "}\n";

	tempout << m_staffout.str();
	tempout << m_scoreout.str();

	printFooterComments(tempout);

	printErrorMessages(out);
	out << tempout.str();

	return status;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::printHeader --
//

void HumdrumToLilypondConverter::printHeader(ostream& tempout) {
	tempout << "\\header {\n";
	tempout << m_indent << "tagline = \"\"\n";
	tempout << "}\n\n";
}


//////////////////////////////
//
// HumdrumToLilypondConverter::extractSegments -- Generate a list
//   of segmentation labels in the file.
//

void HumdrumToLilypondConverter::extractSegments(void) {
	HumdrumFile& infile = m_infile;
	vector<int>& segments = m_segments;
	vector<string>& labels = m_labels;
	bool beforeDataQ = true;
	string label;

	for (int i=0; i<infile.getLineCount(); i++) {
		if (infile[i].isData()) {
			beforeDataQ = false;
		}
		if (!infile[i].isInterpretation()) {
			continue;
		}
		if (infile[i].token(0)->isLabel()) {
			label = infile[i].token(0)->substr(2);
			labels.push_back(label);
			if ((!beforeDataQ) && (segments.size() == 0)) {
				segments.push_back(0);
			} else if (beforeDataQ) {
				segments.push_back(0);
			} else {
				segments.push_back(i);
			}
		}
	}

	if (segments.size() == 0) {
		segments.push_back(0);
	}

	segments.push_back(infile.getLineCount());
}



//////////////////////////////
//
// HumdrumToLilypondConverter::printHeaderComments --
//


void HumdrumToLilypondConverter::printHeaderComments(ostream& out) {
	HumdrumFile& infile = m_infile;
	string token;
	int count = 0;
	bool starting;
	for (int i=0; i<infile.getLineCount(); i++) {
		if (infile[i].isData()) {
			break;
		}
		if (infile[i].isBarline()) {
			break;
		}
		if (infile[i].isInterpretation() && !infile[i].isExclusive()) {
			break;
		}
		if (infile[i].hasSpines()) {
			continue;
		}
		token = *infile[i].token(0);
		if (token.size() == 0) {
			continue;
		}
		starting = true;
		count++;
		for (int j=0; j<(int)token.size(); j++) {
			if (starting && (token[j] == '!')) {
				out << "%";
				continue;
			}
			starting = false;
			out << token[j];
		}
		out << "\n";
	}
	if (count) {
		out << "\n";
	}
}



//////////////////////////////
//
// HumdrumToLilypondConverter::printFooterComments --
//

void HumdrumToLilypondConverter::printFooterComments(ostream& out) {
	HumdrumFile& infile = m_infile;

	stringstream tout;
	int count = 0;
	string token;
	bool starting;
	for (int i=infile.getLineCount()-1; i>0; i--) {
		if (infile[i].isData()) {
			break;
		}
		if (infile[i].isBarline()) {
			break;
		}
		if (infile[i].isInterpretation() && (infile[i] != "*-")) {
			break;
		}
		if (infile[i].hasSpines()) {
			continue;
		}
		token = *infile[i].token(0);
		if (token.size() == 0) {
			continue;
		}
		starting = true;
		for (int j=0; j<(int)token.size(); j++) {
			if (starting && (token[j] == '!')) {
				tout << "%";
				continue;
			}
			starting = false;
			tout << token[j];
		}
		tout << "\n";
	}

	if (count) {
		out << "\n";
		out << tout.str();
	}
}




//////////////////////////////
//
// HumdrumToLilypondConverter::convertPart -- Convert a single Humdrum
//    part into a lilypond part.
//

bool HumdrumToLilypondConverter::convertPart(ostream& out,
		const string& partname, int partindex) {
	vector<int>& segments  = m_segments;
	vector<string>& labels = m_labels;
	HumdrumFile& infile    = m_infile;
	StateVariables& states = m_states;

	string segmentname;
	bool status = true;

	states.clear();

	if (labels.size() > 0) {
		for (int i=0; i<(int)segments.size()-1; i++) {
			segmentname = partname + "Z" + labels[i];
			m_staffout << "\\" << segmentname << " ";
			out << segmentname << " =";
			states.pitch = printRelativeStartingPitch(out, partindex, segments[i],
					segments[i+1]);
			out << " {\n";
			status &= convertSegment(out, partindex, segments[i], segments[i+1]);
			if (!status) {
				break;
			}
			out << "}\n\n";
		}
	} else {
		segmentname = partname;
		out << segmentname << " =";
		states.pitch = printRelativeStartingPitch(out, partindex, 0,
				infile.getLineCount());
		out << "{\n";
		status &= convertSegment(out, partindex, 0, infile.getLineCount());
		out << "}\n\n";
	}

	return status;
}



///////////////////////////////
//
// HumdrumToLilypondConverter::printRelativeStartingPitch --
//

int HumdrumToLilypondConverter::printRelativeStartingPitch(ostream& out,
		int partindex, int startline, int endline) {
	int pitch = getSegmentStartingPitch(partindex, startline, endline);
	if (pitch <= -1000) {
		return pitch;
	}
	int octave = pitch / 40; // no very low pitches for now.
	int diatonic = pitch % 40;
	if (diatonic > 19) {
		octave++;
	}

	out << " \\relative c";
	int ocount = octave - 3;
	int i;
	if (ocount > 0) {
		for (i=0; i<ocount; i++) {
			out << "'";
		}
	} else if (ocount < 0) {
		for (i=0; i<-ocount; i++) {
			out << ",";
		}
	}
	return pitch;
}


//////////////////////////////
//
// HumdrumToLilypondConverter::getSegmentStartingPitch --
//

int HumdrumToLilypondConverter::getSegmentStartingPitch(int partindex,
		int startline, int endline) {

	vector<HTp> starts = getStartTokens(partindex, startline, endline);
	if (starts.size() == 0) {
		return -99999;
	}

	HTp token = starts[0];

	while ((token != NULL) && (token->getLineIndex() < endline)) {
		if (!token->isData()) {
			token = token->getNextToken();
			continue;
		} else if (token->isNull()) {
			token = token->getNextToken();
			continue;
		} else if (token->isRest()) {
			token = token->getNextToken();
			continue;
		}
		return Convert::kernToBase40(*token);
	}
	return -99999;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::getStartTokens --
//

vector<HTp> HumdrumToLilypondConverter::getStartTokens(int partindex,
		int startline, int endline) {
	HumdrumFile& infile = m_infile;
	vector<HTp>& kernstarts = m_kernstarts;

	vector<HTp> output;
	int targettrack = kernstarts[partindex]->getTrack();
	for (int i=startline; i<endline; i++) {
		if (!infile[i].hasSpines()) {
			continue;
		}
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (infile[i].token(j)->getTrack() == targettrack) {
				output.push_back(infile[i].token(j));
			}
		}
		break;
	}

	return output;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convertSegment --
//

bool HumdrumToLilypondConverter::convertSegment(ostream& out, int partindex,
		int startline, int endline) {

	vector<HTp> starttokens = getStartTokens(partindex, startline, endline);

	if (starttokens.size() == 0) {
		// should not be missing a part (no parts that don't start
		// at the beginning and end at the end.
		return false;
	}

	// only dealing with single layer music for now:
	return convertPartSegment(out, starttokens[0], endline);
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convertPartSegment --
//

bool HumdrumToLilypondConverter::convertPartSegment(ostream& out,
		HTp token, int endline) {

	bool status = true;

	if (token == NULL) {
		return false;
	}

	int line = token->getLineIndex();
	if (line >= endline) {
		return true;
	}

	HTp nexttoken = token->getNextToken();
	if (nexttoken && token->isExclusive()) {
		return convertPartSegment(out, nexttoken, endline);
	}

	if (token->isNull()) {
		// do nothing for now, later check for dynamics, lyrics, etc.
	} else if (token->isData()) {
		status &= convertDataToken(out, token);
		out << "\t\t% " << *token << endl;
	} else if (token->isInterpretation()) {
		convertInterpretationToken(out, token);
		out << "\t\t% " << *token << endl;
	} else if (token->isBarline()) {
		out << "\t\t% " << *token << endl;
	} else {
		out << "\t\t% " << *token << endl;
	}

	if (!status) {
		return status;
	}

	if (nexttoken) {
		return convertPartSegment(out, nexttoken, endline);
	} else {
		return status;
	}
}




//////////////////////////////
//
// HumdrumToLilypondConverter::convertInterpretationToken --
//

bool HumdrumToLilypondConverter::convertInterpretationToken(ostream& out, 
		HTp token) {
	bool status = true;
	if (!token) {
		return status;
	}

	if (token->isClef()) {
		out << m_indent;  // temporary
		return convertClef(out, token);
	} else if (token->isKeySignature()) {
		out << m_indent;  // temporary
		return convertKeySignature(out, token);
	}

	return status;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convertKeySignature -- A valid key signature 
//   is presumed to be the input.
//

bool HumdrumToLilypondConverter::convertKeySignature(ostream& out, HTp token) {
	bool status = true;

	bool fs = token->find("f#") != string::npos;
	bool cs = token->find("c#") != string::npos;
	bool gs = token->find("g#") != string::npos;
	bool ds = token->find("d#") != string::npos;
	bool as = token->find("a#") != string::npos;
	bool es = token->find("e#") != string::npos;
	bool bs = token->find("b#") != string::npos;

	bool bb = token->find("b-") != string::npos;
	bool eb = token->find("e-") != string::npos;
	bool ab = token->find("a-") != string::npos;
	bool db = token->find("d-") != string::npos;
	bool gb = token->find("g-") != string::npos;
	bool cb = token->find("c-") != string::npos;
	bool fb = token->find("f-") != string::npos;

	int accids = 0;
	if (fs && !cs && !gs && !ds && !as && !es && !bs) {
		accids = 1;
	} else if (fs && cs && !gs && !ds && !as && !es && !bs) {
		accids = 2;
	} else if (fs && cs && gs && !ds && !as && !es && !bs) {
		accids = 3;
	} else if (fs && cs && gs && ds && !as && !es && !bs) {
		accids = 4;
	} else if (fs && cs && gs && ds && as && !es && !bs) {
		accids = 5;
	} else if (fs && cs && gs && ds && as && es && !bs) {
		accids = 6;
	} else if (fs && cs && gs && ds && as && es && bs) {
		accids = 7;
	} else if (bb && !eb && !ab && !db && !gb && !cb && !fb) {
		accids = -1;
	} else if (bb && eb && !ab && !db && !gb && !cb && !fb) {
		accids = -2;
	} else if (bb && eb && ab && !db && !gb && !cb && !fb) {
		accids = -3;
	} else if (bb && eb && ab && db && !gb && !cb && !fb) {
		accids = -4;
	} else if (bb && eb && ab && db && gb && !cb && !fb) {
		accids = -5;
	} else if (bb && eb && ab && db && gb && cb && !fb) {
		accids = -6;
	} else if (bb && eb && ab && db && gb && cb && fb) {
		accids = -7;
	} else if (!bb && !eb && !ab && !db && !gb && !cb && !fb 
			&& !fs && !cs && !gs && !ds && !as && !es && !bs) {
		accids = 0;
	} else {
		// non-standard key signature
		addErrorMessage("Error: non-standard key signature: " + *token, token);
		return true;
	}

	string mode = "major";
	HTp designation = getKeyDesignation(token);
	string tonic = "";
	char letter;
	if (designation == NULL) {
		// presume major key if no key designation
		switch (accids) {
			case  0: tonic = "c"; break;
			case  1: tonic = "g"; break;
			case  2: tonic = "d"; break;
			case  3: tonic = "a"; break;
			case  4: tonic = "e"; break;
			case  5: tonic = "b"; break;
			case  6: tonic = "fis"; break;
			case  7: tonic = "cis"; break;
			case -1: tonic = "f"; break;
			case -2: tonic = "bes"; break;
			case -3: tonic = "ees"; break;
			case -4: tonic = "aes"; break;
			case -5: tonic = "des"; break;
			case -6: tonic = "ges"; break;
			case -7: tonic = "ces"; break;
		}
	} else {
		for (int i=1; i<(int)designation->size(); i++) {
			letter = (*designation)[i];
			if (letter == ':') {
				break;
			}
			if (letter == '#') {
				tonic += "is";
			} else if (letter == '-') {
				tonic += "es";
			} else {
				tonic += tolower(letter);
			}
		}
		
		if (islower((*designation)[2])) {
			mode = "minor";
		}
		if (designation->find("dor")) {
			mode = "dorian";
		} else if (designation->find("phr")) {
			mode = "phrygian";
		} else if (designation->find("lyd")) {
			mode = "lydian";
		} else if (designation->find("mix")) {
			mode = "mixolydian";
		} else if (designation->find("aeo")) {
			mode = "aeolian";
		} else if (designation->find("loc")) {
			mode = "locrian";
		} else if (designation->find("ion")) {
			mode = "ionian";
		}
	}


	if ((mode == "major") || (mode == "ionian")) {
		if ((accids == 0) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "cis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "ees")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "aes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "des")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "ges")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "ces")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	if ((mode == "minor") || (mode == "aeolian")) {
		if ((accids == 0) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "cis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "gis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "dis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "ais")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "ees")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "aes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	if (mode == "dorian") {
		if ((accids == 0) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "cis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "gis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "dis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "ees")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "aes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "des")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	if (mode == "phrygian") {
		if ((accids == 0) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "cis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "gis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "dis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "ais")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "eis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "ees")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	if (mode == "lydian") {
		if ((accids == 0) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "ees")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "aes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "des")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "ges")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "ces")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "fes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	if (mode == "mixolydian") {
		if ((accids == 0) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "cis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "gis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "ees")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "aes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "des")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "ges")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	if (mode == "locrian") {
		if ((accids == 0) && (tonic == "b")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 1) && (tonic == "fis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 2) && (tonic == "cis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 3) && (tonic == "gis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 4) && (tonic == "dis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 5) && (tonic == "ais")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 6) && (tonic == "eis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == 7) && (tonic == "bis")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -1) && (tonic == "e")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -2) && (tonic == "a")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -3) && (tonic == "d")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -4) && (tonic == "g")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -5) && (tonic == "c")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -6) && (tonic == "f")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		} else if ((accids == -7) && (tonic == "bes")) {
			out << "\\key " << tonic << " \\" << mode; return status;
		}
	}

	string error = "Error: Unknown key signatue " + (*token);
	if (designation) {
		error += " in combination with the key " + (*designation);
	}
	addErrorMessage(error, token);

	return status;
}


//////////////////////////////
//
// HumdrumToLilypondConverter::getKeyDesignation --
//

HTp HumdrumToLilypondConverter::getKeyDesignation(HTp token) {
	if (!token) {
		return NULL;
	}
	HumNum timestamp = token->getDurationFromStart();

	HTp ttok = token->getNextToken();
	while (ttok && (ttok->getDurationFromStart() == timestamp)) {
		if (ttok->isData()) {
			break;
		}
		if (ttok->isKeyDesignation()) {
			return ttok;
		}
		ttok = ttok->getNextToken();
	}

	ttok = token->getPreviousToken();
	while (ttok && (ttok->getDurationFromStart() == timestamp)) {
		if (ttok->isData()) {
			break;
		}
		if (ttok->isKeyDesignation()) {
			return ttok;
		}
		ttok = ttok->getPreviousToken();
	}

	return NULL;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convertClef --
// 
// http://lilypond.org/doc/v2.19/Documentation/notation/clef-styles
//

bool HumdrumToLilypondConverter::convertClef(ostream& out, HTp token) {
	bool status = true;
	if (*token == "*clefG2") {
		out << "\\clef \"treble\"";
	} else if (*token == "*clefF4") {
		out << "\\clef \"bass\"";
	} else if (*token == "*clefC3") {
		out << "\\clef \"alto\"";
	} else if (*token == "*clefGv2") {
		out << "\\clef \"treble_8\"";
	} else if (*token == "*clefC4") {
		out << "\\clef \"tenor\"";
	} else if (*token == "*clefX") {
		out << "\\clef \"percussion\"";
	} else if (*token == "*clefC2") {
		out << "\\clef mezzosoprano\"";
	} else if (*token == "*clefC5") {
		out << "\\clef baritone\"";
	} else if (*token == "*clefG1") {
		out << "\\clef french\"";
	} else if (*token == "*clefC1") {
		out << "\\clef soprano\"";
	} else if (*token == "*clefF3") {
		out << "\\clef varbaritone\"";
	} else {
		addErrorMessage("Error: unknown clef: " + *token, token);
	}

	return status;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convetDataToken --
//

bool HumdrumToLilypondConverter::convertDataToken(ostream& out, HTp token) {
	if (token->isNull()) {
		return true;
	}
	if (!token->isData()) {
		return true;
	}
	if (token->isRest()) {
		return convertRest(out, token);
	} else if (token->isChord()) {
		return convertChord(out, token);
	} else {
		return convertNote(out, token);
	}
}



//////////////////////////////
//
//  HumdrumToLilypondConverter::convertRest --
//

bool HumdrumToLilypondConverter::convertRest(ostream& out, HTp token) {
	StateVariables& states = m_states;

	// Rests should not be in chords, so filter out any chordness.
	string stok = token->getSubtoken(0);

	out << "r";

	// print duration
	HumNum duration = Convert::recipToDuration(stok);
	HumNum durationnodots;
	int dots = characterCount(stok, '.');
	if ((dots != states.dots) || (duration != states.duration)) {
		durationnodots = Convert::recipToDurationNoDots(stok);
		convertDuration(out, duration, durationnodots, dots);
	}

	convertArticulations(out, *token);

	return true;
}



//////////////////////////////
//
//  HumdrumToLilypondConverter::convertChord --
//


bool HumdrumToLilypondConverter::convertChord(ostream& out, HTp token) {
	cerr << "Cannot convert chords yet" << endl;
	return false;
}



//////////////////////////////
//
//  HumdrumToLilypondConverter::convertNote --
//

bool HumdrumToLilypondConverter::convertNote(ostream& out, HTp token,
		int index) {
	StateVariables& states = m_states;

	out << m_indent;  // indenting every note for now

	string stok = token->getSubtoken(0);

	// print pitch
	int pitch = Convert::kernToBase40(stok);
   int diatonic = Convert::kernToDiatonicPC(stok);
	switch(diatonic) {
		case 0: out << "c"; break;
		case 1: out << "d"; break;
		case 2: out << "e"; break;
		case 3: out << "f"; break;
		case 4: out << "g"; break;
		case 5: out << "a"; break;
		case 6: out << "b"; break;
	}
	int accidental = Convert::kernToAccidentalCount(stok);
	switch (accidental) {
		case 2: out << "isis"; break;
		case 1: out << "is"; break;
		case 0:  break;
		case -1: out << "es"; break;
		case -2: out << "eses"; break;
	}
   int interval;
	int sign;
	if (states.pitch != pitch) {
		interval = (pitch - states.pitch);
		sign = 1;
		if (interval < 0) {
			sign = -1;
			interval = -interval;
		}

		// only one octave melodic change for now:
		if (interval <= 20) {
        // do nothing
		} else if ((interval > 20) && (sign > 0)) {
         out << "'";
		} else if ((interval > 20) && (sign < 0)) {
         out << ",";
        // do nothing
		}
		states.pitch = pitch;
	}

	// print duration
	HumNum duration = Convert::recipToDuration(stok);
	HumNum durationnodots;
	int dots = characterCount(stok, '.');
	if ((dots != states.dots) || (duration != states.duration)) {
		durationnodots = Convert::recipToDurationNoDots(stok);
		convertDuration(out, duration, durationnodots, dots);
	}

   // ties:
	if (stok.find("[") != string::npos) {
		out << "~";
	} else if (stok.find("_") != string::npos) {
		out << "~";
	}

   // slurs:
	if (stok.find(")") != string::npos) {
		out << ")";
	}
	if (stok.find("(") != string::npos) {
		out << "(";
	}

	convertArticulations(out, stok);

	return true;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convertArticulations --
//

void HumdrumToLilypondConverter::convertArticulations(ostream& out,
		const string& stok) {
	if (stok.find(";") != string::npos) {
		out << "\\fermata";
	}
}



//////////////////////////////
//
// HumdrumToLilypondConverter::convertDuration --
//

void HumdrumToLilypondConverter::convertDuration(ostream& out, HumNum& duration, 
		HumNum& durationnodots, int dots) {
	StateVariables& states = m_states;

	states.dots = dots;
	states.duration = duration;

	int top = durationnodots.getNumerator();
	int bot = durationnodots.getDenominator();
	HumNum newdur = bot;
	newdur /= top;

	newdur *= 4;
	if (newdur.getDenominator() != 1) {
		// complicated rhythm such as triplet whole note, so deal with later 
		return;
	}

	out << newdur;
	for (int i=0; i<(int)dots; i++) {
		out << ".";
	}
}



/////////////////////////////
//
// HumdrumToLilypondConverter::characterCount --
//

int HumdrumToLilypondConverter::characterCount(const string &text, char symbol) {
    return (int)std::count(text.begin(), text.end(), symbol);
}



/////////////////////////////
//
// HumdrumToLilypondConverter::arabicToRomanNumeral --
//

string HumdrumToLilypondConverter::arabicToRomanNumeral(int arabic,
		int casetype) {
	string output;

	if (arabic < 1) {
		return output;
	}

	vector<int> numbers = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
	vector<string> uc = { "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
	vector<string> lc = { "m", "cm", "d", "cd", "c", "xc", "l", "xl", "x", "ix", "v", "iv", "i"};

	for (int i=0; i<(int)numbers.size(); i++) {
		while (arabic >= numbers[i]) {
			if (casetype) {
				output += uc[i];
			} else {
				output += lc[i];
			}
			arabic -= numbers[i];
		}
	}
	return output;
}



//////////////////////////////
//
// HumdrumToLilypondConverter::addErrorMessage --
//

void HumdrumToLilypondConverter::addErrorMessage(const string& message,
		HTp token) {
	m_errors.push_back(message);
	if (token != NULL) {
		addErrorMessage("\tLine:  " + to_string(token->getLineNumber()));
		addErrorMessage("\tField: " + to_string(token->getFieldNumber()));
	}
}



//////////////////////////////
//
// HumdrumToLilypondConverter::printErrorMessages(ostream& out) {
//

void HumdrumToLilypondConverter::printErrorMessages(ostream& out) {
	for (int i=0; i<(int)m_errors.size(); i++) {
		out << "% " << m_errors[i] << endl;
	}
	if (m_errors.size() > 0) {
		out << endl;
	}
}



}  // namespace hum



