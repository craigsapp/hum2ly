//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Aug  6 10:53:40 CEST 2016
// Last Modified: Tue Aug  9 12:53:15 CEST 2016
// Filename:      main.cpp
// URL:           https://github.com/craigsapp/hum2ly/blob/master/main.cpp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// Description:   Command-line interface for converting Humdrum files into 
//                lilypond files.
//

#include "hum2ly.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	hum::HumdrumToLilypondConverter converter;
	hum::Options options = converter.getOptionDefinitions();
	options.process(argc, argv);

	hum::HumdrumFile infile;
	string filename;
	if (options.getArgCount() == 0) {
		filename = "<STDIN>";
		infile.read(cin);
	} else {
		filename = options.getArg(1);
		infile.read(filename);
	}

	converter.setOptions(argc, argv);
	stringstream out;
	bool status = converter.convert(out, infile);
	if (!status) {
		cerr << "Error converting file: " << filename << endl;
	}
	cout << out.str();

	exit(0);
}



