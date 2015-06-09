#include <FitterFactory.h>

#include <fstream>
#include <iostream>
#include <cstdlib>

#include <TFile.h>
#include <TKey.h>
#include <TMath.h>
#include <TROOT.h>

#include <FitterFactory.h>

#include "ffconfig.h"

#ifdef HAS_ROOTTOOLS
#include <RootTools.h>
#endif

int main(int argc, char * argv[])
{
	if (argc < 3)
	{
		std::cerr << " Usage: " << argv[0] << " file.root file_with_params" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// load custom functions definitions
#ifdef HAS_ROOTTOOLS
	RootTools::MyMath();
#endif

	// create params output filename
	size_t pflen = strlen(argv[2]);
	char * opf = new char[pflen+5];
	sprintf(opf, "%s%s", argv[2], ".out");

	// create fitting factory
	FitterFactory ff;
	ff.initFactoryFromFile(argv[2], opf);

	// uncomment this to print all entries
	// ff.print();

	// open root file
	TFile * file = TFile::Open(argv[1], "READ");

	
	// create root output filename
	size_t prlen = strlen(argv[1]);
	char * orf = new char[prlen+5];
	sprintf(orf, "%s%s", "out_", argv[1]);

	TFile * ofile = nullptr;
	if(argc >= 4)
	{
		ofile = TFile::Open(argv[3], "RECREATE");
		ofile->cd();
	}

	// do fitting for each TH1 object found
	TKey * key = nullptr;
	TIter nextkey(file->GetListOfKeys());
	while ((key = (TKey*)nextkey())) {
		const char * classname = key->GetClassName();
		TClass *cl = gROOT->GetClass(classname);
		if (!cl) continue;

		TObject *obj = key->ReadObj();
		printf("*** %s\n", obj->GetName());
		if (obj->InheritsFrom("TH1"))
		{
			TH1 * h = (TH1*)obj->Clone(obj->GetName());
			ff.fit(h);
			if (ofile)
				h->Write();
		}
	}

	ff.exportFactoryToFile();

	file->Close();

	if (ofile)
	{
		ofile->Write();
		ofile->Close();
	}

	return 0;
}