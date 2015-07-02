/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fstream>
#include <string>
#include "getopt.h"

#include "TCanvas.h"
#include "TChain.h"
#include "TDirectory.h"
#include "TError.h"
#include "TGaxis.h"
#include "TH2.h"
#include "TImage.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TMath.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TVector.h"

#include <sys/stat.h>

// #include "RootTools.h"

#include "FitterFactory.h"

#define PR(x) std::cout << "++DEBUG: " << #x << " = |" << x << "| (" << __FILE__ << ", " << __LINE__ << ")\n";

HistFitParams::HistFitParams() :
	allparnum(0), rebin(0),
	fit_disabled(kFALSE), pars(0),
	funSig(0), funBg(0), funSum(0) {
}

HistFitParams::HistFitParams(const HistFitParams & hfp)
{
	histname = hfp.histname;
	funname = hfp.funname;
	f_sig = hfp.f_sig;
	f_bg = hfp.f_bg;

	allparnum = hfp.allparnum;
	rebin = hfp.rebin;
	fun_l = hfp.fun_l;
	fun_u = hfp.fun_u;
	fit_disabled = hfp.fit_disabled;

	funSig = hfp.funSig;
	funBg = hfp.funBg;
	funSum = hfp.funSum;

	if (funSum)
	{
		pars = new ParamValues[funSum->GetNpar()];
		memcpy(pars, hfp.pars, sizeof(ParamValues)*funSum->GetNpar());
	} else {
		pars = NULL;
	}
}

HistFitParams& HistFitParams::operator=(const HistFitParams& hfp) {
	histname = hfp.histname;
	funname = hfp.funname;
	f_sig = hfp.f_sig;
	f_bg = hfp.f_bg;

	allparnum = hfp.allparnum;
	rebin = hfp.rebin;
	fun_l = hfp.fun_l;
	fun_u = hfp.fun_u;
	fit_disabled = hfp.fit_disabled;


	funSig = hfp.funSig;
	funBg = hfp.funBg;
	funSum = hfp.funSum;

	if (pars) delete [] pars;

	if (funSum)
	{
		pars = new ParamValues[funSum->GetNpar()];
		memcpy(pars, hfp.pars, sizeof(ParamValues)*funSum->GetNpar());
	} else {
		pars = NULL;
	}

	return *this;
}

HistFitParams::~HistFitParams()
{
	delete [] pars;
}

void HistFitParams::Init(const TString & h, const TString & fsig, const TString & fbg, Int_t rbn, Double_t f_l, Double_t f_u)
{
	histname = h;
	if (h[0] == '@') {
		fit_disabled = kTRUE;
		histname.Remove(0, 1);
	}
// 	func = f;
	rebin = rbn;
	funname = "f_" + histname;
	f_sig = fsig;
	f_bg = fbg;
	TString funcSig = "f_" + histname + "_sig";
	TString funcBg = "f_" + histname + "_bg";

	funSig = new TF1(funcSig, fsig, f_l, f_u);
	funBg = new TF1(funcBg, fbg, f_l, f_u);

	funSum = new TF1("f_" + histname, fsig + "+" + fbg, f_l, f_u);
	allparnum = funSum->GetNpar();
	pars = new ParamValues[allparnum];
	fun_l = f_l;
	fun_u = f_u;
}

void HistFitParams::SetParam(Int_t par, Double_t val) {
	if (!(par < allparnum)) return;
	pars[par].val = val;
	funSum->SetParameter(par, val);
}

void HistFitParams::SetParam(Int_t par, Double_t val, Double_t l, Double_t u) {
	if (!(par < allparnum)) return;
	pars[par].val = val;
	pars[par].l = l;
	pars[par].u = u;
	funSum->SetParameter(par, val);
	funSum->SetParLimits(par, l, u);
}

const HistFitParams HistFitParams::parseEntryFromFile(const TString & line) {
	TString line_ = line;
	line_.ReplaceAll("\t", " ");
	TObjArray * arr = line_.Tokenize(" ");

	HistFitParams hfp;

	if (arr->GetEntries() < 5)
		return hfp;

	hfp.Init(
		((TObjString *)arr->At(0))->String(),			// hist name
		((TObjString *)arr->At(1))->String(),			// func val
		((TObjString *)arr->At(2))->String(),			// func val
		((TObjString *)arr->At(3))->String().Atoi(),	// bg par offset
		((TObjString *)arr->At(4))->String().Atof(),	// low range
		((TObjString *)arr->At(5))->String().Atof()		// upper range
	);

	Double_t par_, l_, u_;
	Int_t step = 0;
	Int_t parnum = 0;

	for (int i = 6; i < arr->GetEntries(); /*++i*/i += step, ++parnum) {
		TString val = ((TObjString *)arr->At(i))->String();
		TString nval = ((i+1) < arr->GetEntries()) ? ((TObjString *)arr->At(i+1))->String() : TString();

		par_ = val.Atof();
		if (nval == ":") {
			l_ = (i+2) < arr->GetEntries() ? ((TObjString *)arr->At(i+2))->String().Atof() : 0;
			u_ = (i+2) < arr->GetEntries() ? ((TObjString *)arr->At(i+3))->String().Atof() : 0;
			step = 4;
		} else {
			l_ = 0;
			u_ = 0;
			step = 1;
		}
// std::cout << parnum << ", " << par_ << ", " << l_ << ", " << u_ << "\n";
		hfp.SetParam(parnum, par_, l_, u_);
	}

	return hfp;
}

TString HistFitParams::exportEntry() const
{
	TString out;
	if (fit_disabled)
		out += "@";
	else
		out += " ";

	out = TString::Format(
		"%s%s\t%s %s %d %.0f %.0f",
		out.Data(), histname.Data(), f_sig.Data(), f_bg.Data(), rebin, fun_l, fun_u);

// 	PR(allparnum);
	for (int i = 0; i < allparnum; ++i) {
		Double_t val = funSum->GetParameter(i);
// 		TString v = TString::Format("%g", val).Strip(TString::kTrailing, '0');
// 		TString l = TString::Format("%g", pars[i].l).Strip(TString::kTrailing, '0');
// 		TString u = TString::Format("%g", pars[i].u).Strip(TString::kTrailing, '0');
		TString v = TString::Format("%g", val);
		TString l = TString::Format("%g", pars[i].l);
		TString u = TString::Format("%g", pars[i].u);

		if (pars[i].l or pars[i].u)
			out += TString::Format(" %s : %s %s", v.Data(), l.Data(), u.Data());
		else
			out += TString::Format(" %s", v.Data());
	}

	return out;
}

void HistFitParams::Print() const {
	std::cout << "@ hist name = " << histname.Data() << std::endl;
	std::cout << "  func name = " << funname.Data() << std::endl;
	std::cout << " | rebin = " << rebin << std::endl;
	std::cout << "  range = " << fun_l << " -- " << fun_u << std::endl;
	std::cout << "  param num = " << allparnum << std::endl;
	std::cout << "  params list:" << std::endl;
	PR(pars);
	for (int i = 0; i < allparnum; ++i) {
		std::cout << "  * " << i << ": " << pars[i].val << " " << pars[i].l << " " << pars[i].u << std::endl;
	}
}

void HistFitParams::PrintInline() const {
	std::cout << "  hn=" << histname.Data() << std::endl;
	if (rebin > 0)
		std::cout << " R=" << rebin;
}

FitterFactory::FLAGS FitterFactory::setFlags(FitterFactory::FLAGS new_flags)
{
	FitterFactory::FLAGS old = flags;
	flags = new_flags;
	return old;
}

void FitterFactory::setDefaultParameters(HistFitParams defs)
{
	has_defaults = true;
	defpars = defs;
}

bool FitterFactory::initFactoryFromFile(const char * filename, const char * auxname)
{
	struct stat st_ref;
	struct stat st_aux;

	TSystem sys;

	par_ref = filename;
	par_aux = auxname;

	if (!par_ref)
	{
		fprintf(stderr, "No reference input file given\n");
	}
	if (!par_aux)
	{
		fprintf(stderr, "No output file given\n");
	}

	long long int mod_ref = 0;
	long long int mod_aux = 0;

//	#ifdef HAVE_ST_MTIM
	if (stat(par_ref, &st_ref))
	{
		perror(par_ref);
	}
	else
	{
		mod_ref = (long long)st_ref.st_mtim.tv_sec;
	}

	if (stat(par_aux, &st_aux))
	{
		perror(par_aux);
	}
	else
	{
		mod_aux = (long long)st_aux.st_mtim.tv_sec;
	}

	bool aux_newer = mod_aux > mod_ref;

	std::cout << "Parameter files:";
	if (!par_ref)
		std::cout << " [x] REF";
	else if (!aux_newer)
		std::cout << " [*] REF";
	else
		std::cout << " [ ] REF";

	if (!par_aux)
		std::cout << " [x] AUX";
	else if (aux_newer)
		std::cout << " [*] AUX";
	else
		std::cout << " [ ] AUX";
	std::cout << std::endl;

	if (flags == ALWAYS_REF)
		return import_parameters(par_ref);

	if (flags == ALWAYS_AUX)
		return import_parameters(par_ref);

	if (flags == ALWAYS_NEWER)
	{
		if (aux_newer)
			return import_parameters(par_aux);
		else
			return import_parameters(par_ref);
	}

	return false;
}

bool FitterFactory::exportFactoryToFile()
{
	return export_parameters(par_aux);
}

bool FitterFactory::import_parameters(const char * filename)
{
	std::ifstream ifs(filename);
	if (!ifs.is_open()) {
		std::cerr << "No file " << filename << " to open." << std::endl;
		return false;
	}

	size_t cnt = 0;
	std::string line;
	while (std::getline(ifs, line))
	{
		HistFitParams imfit = HistFitParams::parseEntryFromFile(line);
		hfpmap.insert(std::pair<TString,HistFitParams>(imfit.histname, imfit));
		++cnt;
	}

	printf("Imported %u entries\n", cnt);
	return true;
}

bool FitterFactory::export_parameters(const char* filename)
{
	if (!par_aux)
		return false;

	std::ofstream fparfile(par_aux);
	if (!fparfile.is_open())
	{
		std::cerr << "Can't create AUX file " << filename << ". Skipping..." << std::endl;
	}
	else
	{
		std::cout << "AUX file " << filename << " opened..." << std::endl;
		for (std::map<TString, HistFitParams>::const_iterator it = hfpmap.begin(); it != hfpmap.end(); ++it)
		{
			fparfile << it->second.exportEntry().Data() << std::endl;
		}
	}
	fparfile.close();
	return true;
}

FitterFactory::FIND_FLAGS FitterFactory::findParams(TH1 * hist, HistFitParams & hfp, bool use_defaults) const
{
	return findParams(hist->GetName(), hfp, use_defaults);
}

FitterFactory::FIND_FLAGS FitterFactory::findParams(const char * name, HistFitParams & hfp, bool use_defaults) const
{
	std::map<TString, HistFitParams>::const_iterator it = hfpmap.find(name);
	if (it != hfpmap.end())
	{
		hfp = (*it).second;

// 		printf(" + Fitting Invariant Mass with custom function");
// 		hfp.PrintInline();
		return USE_FOUND;
	}
	else
	{
		if (use_defaults and has_defaults)
		{
			hfp = defpars;

// 			printf(" + Fitting Invariant Mass with standard function");
// 			hfp.PrintInline();
			return USE_DEFAULT;
		}
		else
		{
			return NOT_FOUND;
		}
	}

}

bool FitterFactory::fit(TH1* hist, const char* pars, const char* gpars)
{
	HistFitParams hfp;
	int res = findParams(hist->GetName(), hfp, true);
	if (res == NOT_FOUND)
		return false;

	return fit(hfp, hist, pars, gpars);
}

bool FitterFactory::fit(HistFitParams & hfp, TH1* hist, const char* pars, const char* gpars)
{
	std::pair<Double_t, Double_t> res;

	bool was_rebin = false;

	Int_t bin_l = hist->FindBin(hfp.fun_l);
	Int_t bin_u = hist->FindBin(hfp.fun_u);

	if (hfp.rebin != 0)
	{
		was_rebin = true;
		hist->Rebin(hfp.rebin);
	}

	if (hist->Integral(bin_l, bin_u) == 0)
		return true;

	TF1 * tfLambdaSig = hfp.funSig;
	TF1 * tfLambdaBg = hfp.funBg;
	TF1 * tfLambdaSum = hfp.funSum;

	hist->GetListOfFunctions()->Clear();

	// FIXME ??? why this?
// 	tfLambdaSum->Draw();

	tfLambdaSig->SetBit(TF1::kNotDraw);
	tfLambdaBg->SetBit(TF1::kNotDraw);
	tfLambdaSum->SetBit(TF1::kNotDraw);

	const size_t par_num = tfLambdaSum->GetNpar();

	// backup old parameters
	double * pars_backup_old = new double[par_num];
	tfLambdaSum->GetParameters(pars_backup_old);
	double chi2_backup_old = hist->Chisquare(tfLambdaSum, "R");

// 	tfLambdaSum->SetRange(hfp.fun_l, hfp.fun_u);

	// TODO remove it at some point
	// this trick differences amplitudes, having two amplitudes of the same value is unprobable
	if ( fabs(pars_backup_old[0] - pars_backup_old[3]) < 0.1 )
	{
		printf(" + applying trick\n");
		tfLambdaSum->SetParameter(0, pars_backup_old[0] * 1.5);
		tfLambdaSum->SetParameter(3, pars_backup_old[3] * 0.5);

		chi2_backup_old *= 2.;
	}

	// print them
	printf("* %s old: ",  hist->GetName());
	for (uint i = 0; i < par_num; ++i)
		printf("%f ", pars_backup_old[i]);
	printf(" --> chi2:  %f -- *\n", chi2_backup_old);

// 	RootTools::Smooth(hist, 50);
	hist->Fit(tfLambdaSum, pars, gpars, hfp.fun_l, hfp.fun_u);

	// backup new parameters
	double * pars_backup_new = new double[par_num];
	tfLambdaSum->GetParameters(pars_backup_new);
	double chi2_backup_new = hist->Chisquare(tfLambdaSum, "R");

	printf("  %s new: ",  hist->GetName());
	for (uint i = 0; i < par_num; ++i)
		printf("%f ", pars_backup_new[i]);
	printf(" --> chi2:  %f -- *\n", chi2_backup_new);

// 	double chi2_new = tfLambdaSum->GetChisquare();
// 	printf(" %s Chi2 -- old:   %12.2g\t new:   %12.2g\n", hist->GetName(), chi2_backup, chi2_new);
	if (chi2_backup_new >= chi2_backup_old)
	{
		tfLambdaSum->SetParameters(pars_backup_old);
		((TF1*)hist->GetListOfFunctions()->At(0))->SetParameters(pars_backup_old);
		printf("\tFIT-ERROR: Fit get worst -> %f\n", hist->Chisquare(tfLambdaSum, "R"));

// 		double * pars_backup2 = new double[par_num];
// 		tfLambdaSum->GetParameters(pars_backup2);
// 		// print them
// 		printf("   restored: ");
// 		for (uint i = 0; i < par_num; ++i)
// 			printf("%f ", ((TF1*)hist->GetListOfFunctions()->At(0))->GetParameter(i));
// 		printf("\n");
	}
	else if (tfLambdaSum->GetMaximum() > 2.0 * hist->GetMaximum())
	{
		tfLambdaSum->SetParameters(pars_backup_old);
		((TF1*)hist->GetListOfFunctions()->At(0))->SetParameters(pars_backup_old);
		printf("\tMAX-ERROR: %g vs. %g -> %f\n", tfLambdaSum->GetMaximum(), hist->GetMaximum(), hist->Chisquare(tfLambdaSum, "R") );

// 		double * pars_backup2 = new double[par_num];
// 		tfLambdaSum->GetParameters(pars_backup2);
// 		// print them
// 		printf("   restored: ");
// 		for (uint i = 0; i < par_num; ++i)
// 			printf("%f ", ((TF1*)hist->GetListOfFunctions()->At(0))->GetParameter(i));
// 		printf("\n");
	}
	else
	{
		printf("\taccepted\n");
	}
// 	printf("   %f\n", hist->Chisquare(tfLambdaSum, "R"));

	tfLambdaSig->SetParameters(tfLambdaSum->GetParameters());
	tfLambdaBg->SetParameters(tfLambdaSum->GetParameters());

	hist->GetListOfFunctions()->Add(tfLambdaSig);
	hist->GetListOfFunctions()->Add(tfLambdaBg);

	return true;
}

void FitterFactory::print() const
{
	std::map<TString, HistFitParams>::const_iterator it;
	for (it = hfpmap.begin(); it != hfpmap.end(); ++it)
		it->second.Print();
}
