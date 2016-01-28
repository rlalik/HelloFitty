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
#include <getopt.h>
#include <sys/stat.h>

#include <TCanvas.h>
#include <TChain.h>
#include <TDirectory.h>
#include <TError.h>
#include <TGaxis.h>
#include <TH2.h>
#include <TImage.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TMath.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TVector.h>

#include <TVirtualFitter.h>
#include <TMatrixDSym.h>

// #include "RootTools.h"

#include "FitterFactory.h"

#define PR(x) std::cout << "++DEBUG: " << #x << " = |" << x << "| (" << __FILE__ << ", " << __LINE__ << ")\n";

bool FitterFactory::verbose_flag = true;

void ParamValues::print()
{
	printf("   %g ( %g, %g ), %d, %d\n", val, l, u, flag, has_limits);
}

long int HistFitParams::cnt_total = 0;
long int HistFitParams::cnt_owned = 0;

void HistFitParams::printStats(char * infotext)
{
	if (infotext)
		printf(" (%s) FitParams: total=%ld, owned=%ld\n", infotext, cnt_total, cnt_owned);
	else
		printf(" FitParams: total=%ld, owned=%ld\n", cnt_total, cnt_owned);
}

HistFitParams::HistFitParams() :
	allparnum(0), rebin(0),
	fit_disabled(kFALSE), pars(0),
	funSig(nullptr), funBkg(nullptr), funSum(nullptr),
	is_owner(true)
{
	++cnt_total;
	++cnt_owned;
}

HistFitParams::HistFitParams(const HistFitParams & hfp)
{
	histname = hfp.histname;
	funname = hfp.funname;
	f_sig = hfp.f_sig;
	f_bkg = hfp.f_bkg;

	allparnum = hfp.allparnum;
	rebin = hfp.rebin;
	fun_l = hfp.fun_l;
	fun_u = hfp.fun_u;
	fit_disabled = hfp.fit_disabled;

	funSig = new TF1();
	funBkg = new TF1();
	funSum = new TF1();

	if (hfp.funSig) hfp.funSig->Copy(*funSig);
	if (hfp.funBkg) hfp.funBkg->Copy(*funBkg);
	if (hfp.funSum) hfp.funSum->Copy(*funSum);

	if (funSum)
	{
		pars = new ParamValues[funSum->GetNpar()];
		memcpy(pars, hfp.pars, sizeof(ParamValues)*funSum->GetNpar());
	} else {
		pars = nullptr;
	}

	is_owner = true;

	++cnt_total;
	++cnt_owned;
}

HistFitParams& HistFitParams::operator=(const HistFitParams& hfp)
{
	// remove old structures
	--cnt_total;
	if (is_owner)
	{
		--cnt_owned;
		cleanup();
	}
	if (pars) delete [] pars;
	pars = nullptr;

	// create new structures
	histname = hfp.histname;
	funname = hfp.funname;
	f_sig = hfp.f_sig;
	f_bkg = hfp.f_bkg;

	allparnum = hfp.allparnum;
	rebin = hfp.rebin;
	fun_l = hfp.fun_l;
	fun_u = hfp.fun_u;
	fit_disabled = hfp.fit_disabled;

// 	funSig = hfp.funSig;
// 	funBkg = hfp.funBkg;
// 	funSum = hfp.funSum;

	if (!funSig)	funSig = new TF1();
	if (!funBkg)	funBkg = new TF1();
	if (!funSum)	funSum = new TF1();

	if (hfp.funSig) hfp.funSig->Copy(*funSig);
	if (hfp.funBkg) hfp.funBkg->Copy(*funBkg);
	if (hfp.funSum) hfp.funSum->Copy(*funSum);

	if (pars) delete [] pars;

	if (funSum)
	{
		pars = new ParamValues[funSum->GetNpar()];
		memcpy(pars, hfp.pars, sizeof(ParamValues)*funSum->GetNpar());
	} else {
		pars = NULL;
	}

	is_owner = true;

	++cnt_total;
	++cnt_owned;

	return *this;
}

HistFitParams::~HistFitParams()
{
	--cnt_total;
	if (is_owner)
	{
		--cnt_owned;
		cleanup();
	}

	delete [] pars;
}

void HistFitParams::cleanup()
{
	if (funSig) delete funSig;
	if (funBkg) delete funBkg;
	if (funSum) delete funSum;

	funSig = nullptr;
	funBkg = nullptr;
	funSum = nullptr;
}

inline void HistFitParams::setOwner(bool owner)
{
	if (is_owner and !owner)
	{
		--cnt_owned;
	}
	else if (!is_owner and owner)
	{
		++cnt_owned;
	}

	is_owner = owner;
}

void HistFitParams::init(const TString & h, const TString & fsig, const TString & fbg, Int_t rbn, Double_t f_l, Double_t f_u)
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
	f_bkg = fbg;
	TString funcSig = "f_" + histname + "_sig";
	TString funcBg = "f_" + histname + "_bkg";

	funSig = new TF1(funcSig, fsig, f_l, f_u);
	funBkg = new TF1(funcBg, fbg, f_l, f_u);

	funSum = new TF1("f_" + histname, fsig + "+" + fbg, f_l, f_u);
	allparnum = funSum->GetNpar();
	pars = new ParamValues[allparnum];
	fun_l = f_l;
	fun_u = f_u;
}

void HistFitParams::setParam(Int_t par, Double_t val, ParamValues::ParamFlags flag)
{
	if (!(par < allparnum)) return;
	pars[par].val = val;
	pars[par].flag = flag;
	pars[par].has_limits = false;

	if (flag == ParamValues::FIXED)
		funSum->FixParameter(par, val);
	else
		funSum->SetParameter(par, val);
}


void HistFitParams::setParam(Int_t par, Double_t val, Double_t l, Double_t u, ParamValues::ParamFlags flag)
{
	if (!(par < allparnum)) return;
	pars[par].val = val;
	pars[par].l = l;
	pars[par].u = u;
	pars[par].flag = flag;
	pars[par].has_limits = true;

	if (flag == ParamValues::FIXED)
		funSum->FixParameter(par, val);
	else
	{
		funSum->SetParameter(par, val);
		funSum->SetParLimits(par, l, u);
	}
}

const HistFitParams HistFitParams::parseEntryFromFile(const TString & line)
{
	TString line_ = line;
	line_.ReplaceAll("\t", " ");
	TObjArray * arr = line_.Tokenize(" ");

	HistFitParams hfp;

	if (arr->GetEntries() < 5)
		return hfp;

	hfp.init(
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
	ParamValues::ParamFlags flag_;
	bool has_limits_ = false;

	for (int i = 6; i < arr->GetEntries(); /*++i*/i += step, ++parnum)
	{
		TString val = ((TObjString *)arr->At(i))->String();
		TString nval = ((i+1) < arr->GetEntries()) ? ((TObjString *)arr->At(i+1))->String() : TString();

		par_ = val.Atof();
		if (nval == ":")
		{
			l_ = (i+2) < arr->GetEntries() ? ((TObjString *)arr->At(i+2))->String().Atof() : 0;
			u_ = (i+2) < arr->GetEntries() ? ((TObjString *)arr->At(i+3))->String().Atof() : 0;
			step = 4;
			flag_ = ParamValues::FREE;
			has_limits_ = true;
		}
		else if (nval == "F")
		{
			l_ = (i+2) < arr->GetEntries() ? ((TObjString *)arr->At(i+2))->String().Atof() : 0;
			u_ = (i+2) < arr->GetEntries() ? ((TObjString *)arr->At(i+3))->String().Atof() : 0;
			step = 4;
			flag_ = ParamValues::FIXED;
			has_limits_ = true;
		}
		else if (nval == "f")
		{
			l_ = 0;
			u_ = 0;
			step = 2;
			flag_ = ParamValues::FIXED;
			has_limits_ = false;
		}
		else
		{
			l_ = 0;
			u_ = 0;
			step = 1;
			flag_ = ParamValues::FREE;
			has_limits_ = false;
		}

// 		std::cout << parnum << ", " << par_ << ", " << l_ << ", " << u_ << ", " << flag_ << "\n";
		if (has_limits_)
			hfp.setParam(parnum, par_, l_, u_, flag_);
		else
			hfp.setParam(parnum, par_, flag_);
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

	char sep;

	out = TString::Format(
		"%s%s\t%s %s %d %.0f %.0f",
		out.Data(), histname.Data(), f_sig.Data(), f_bkg.Data(), rebin, fun_l, fun_u);

	for (int i = 0; i < allparnum; ++i)
	{
		// BUG why val from funSum?
// 		Double_t val = funSum->GetParameter(i);
// 		TString v = TString::Format("%g", val);

		TString v = TString::Format("%g", pars[i].val);
		TString l = TString::Format("%g", pars[i].l);
		TString u = TString::Format("%g", pars[i].u);

		switch (pars[i].flag)
		{
			case ParamValues::FREE:
				if (pars[i].has_limits)
					sep = ':';
				else
					sep = ' ';
				break;
			case ParamValues::FIXED:
				if (pars[i].has_limits)
					sep = 'F';
				else
					sep = 'f';
				break;
			default:
				sep = ' ';
				break;
		}

//		pars[i].print();
		if (pars[i].flag == ParamValues::FREE and pars[i].has_limits == 0)
			out += TString::Format(" %s", v.Data());
		else if (pars[i].flag == ParamValues::FIXED and pars[i].has_limits == 0)
			out += TString::Format(" %s %c", v.Data(), sep);
		else
			out += TString::Format(" %s %c %s %s", v.Data(), sep, l.Data(), u.Data());
	}

	return out;
}

void HistFitParams::print() const
{
	std::cout << "@ hist name = " << histname.Data() << std::endl;
	std::cout << "  func name = " << funname.Data() << std::endl;
	std::cout << " | rebin = " << rebin << std::endl;
	std::cout << "  range = " << fun_l << " -- " << fun_u << std::endl;
	std::cout << "  param num = " << allparnum << std::endl;
	std::cout << "  params list:" << std::endl;

	for (int i = 0; i < allparnum; ++i)
	{
		std::cout << "  * " << i << ": " << pars[i].val << " " << pars[i].l << " " << pars[i].u << std::endl;
	}
}

void HistFitParams::printInline() const
{
	std::cout << "  hn=" << histname.Data() << std::endl;
	if (rebin > 0)
		std::cout << " R=" << rebin;
}

bool HistFitParams::update(TF1 * f)
{
	if (allparnum == f->GetNpar())
	{
		for (int i = 0; i < allparnum; ++i)
			pars[i].val = f->GetParameter(i);
	}
	else
			return false;

	return true;
}

FitterFactory::FitterFactory(FLAGS flags) :
	flags(flags), has_defaults(false),
	par_ref(nullptr), par_aux(nullptr), min_entries(0),
	ps_prefix(PS_IGNORE), ps_suffix(PS_IGNORE)
{
}

FitterFactory::~FitterFactory()
{
	std::map<TString, HistFitParams>::iterator it;
	for (it = hfpmap.begin(); it != hfpmap.end(); ++it)
		it->second.cleanup();

	hfpmap.clear();
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
		std::pair<TString,HistFitParams> hfp(imfit.histname, imfit);
		hfpmap.insert(hfp);
		++cnt;
	}

	printf("Imported %lu entries, total entries %lu\n", cnt, hfpmap.size());
	return true;
}

bool FitterFactory::export_parameters(const char* filename)
{
	if (!filename)
		return false;

	std::ofstream fparfile(filename);
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

bool FitterFactory::updateParams(TH1 * hist, HistFitParams & hfp)
{
	std::map<TString, HistFitParams>::iterator it = hfpmap.find(hist->GetName());
	if (it != hfpmap.end())
	{
		it->second = hfp;
	}
	else
		return false;

	return true;
}

bool FitterFactory::updateParams(TH1 * hist, TF1 * f)
{
	std::map<TString, HistFitParams>::iterator it = hfpmap.find(hist->GetName());
	if (it != hfpmap.end())
	{
		HistFitParams hfp = it->second;

		return hfp.update(f);
	}
	else
		return false;

	return true;
}

FitterFactory::FIND_FLAGS FitterFactory::findParams(TH1 * hist, HistFitParams & hfp, bool use_defaults) const
{
	return findParams(hist->GetName(), hfp, use_defaults);
}

FitterFactory::FIND_FLAGS FitterFactory::findParams(const char * name, HistFitParams & hfp, bool use_defaults) const
{
	std::map<TString, HistFitParams>::const_iterator it = hfpmap.find(format_name(name));
	if (it != hfpmap.end())
	{std::cout << "     Found:  " << prefix + name + suffix << std::endl;
		hfp = (*it).second;

// 		printf(" + Fitting Invariant Mass with custom function");
// 		hfp.PrintInline();
		return USE_FOUND;
	}
	else
	{std::cout << " Not Found:  " << prefix + name + suffix << std::endl;
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
// 	HistFitParams::printStats();
	HistFitParams hfp;
	int res = findParams(hist->GetName(), hfp, true);

// 	HistFitParams::printStats();
	if (res == NOT_FOUND)
		return false;

	bool status = fit(hfp, hist, pars, gpars, min_entries);
// 	HistFitParams::printStats();

	if (status)
		updateParams(hist, hfp);

	return status;
}

bool FitterFactory::fit(HistFitParams & hfp, TH1* hist, const char* pars, const char* gpars, double min_entries)
{
// 	std::pair<Double_t, Double_t> res;

// 	bool was_rebin = false;

	Int_t bin_l = hist->FindBin(hfp.fun_l);
	Int_t bin_u = hist->FindBin(hfp.fun_u);

	if (hfp.rebin != 0)
	{
// 		was_rebin = true;
		hist->Rebin(hfp.rebin);
	}

	if (hist->GetEntries() < min_entries)
		return false;

	if (hist->Integral(bin_l, bin_u) == 0)
		return false;

	TF1 * tfLambdaSig = new TF1();
	TF1 * tfLambdaBkg = new TF1();
	TF1 * tfLambdaSum = new TF1();

	if (hfp.funSig) hfp.funSig->Copy(*tfLambdaSig);
	if (hfp.funBkg) hfp.funBkg->Copy(*tfLambdaBkg);
	if (hfp.funSum) hfp.funSum->Copy(*tfLambdaSum);

// 	PR(hist->GetListOfFunctions()->GetEntries());
	hist->GetListOfFunctions()->Clear();
	hist->GetListOfFunctions()->SetOwner(kTRUE);

	// FIXME ??? why this?
// 	tfLambdaSum->Draw();

	tfLambdaSig->SetBit(TF1::kNotDraw);
	tfLambdaBkg->SetBit(TF1::kNotDraw);
// 	tfLambdaSum->SetBit(TF1::kNotDraw);

	const size_t par_num = tfLambdaSum->GetNpar();

	// backup old parameters
	double * pars_backup_old = new double[par_num];
	tfLambdaSum->GetParameters(pars_backup_old);
	double chi2_backup_old = hist->Chisquare(tfLambdaSum, "R");

// 	tfLambdaSum->SetRange(hfp.fun_l, hfp.fun_u);

	// TODO remove it at some point
	// this trick differences amplitudes, having two amplitudes of the same value is unprobable
	if ( fabs(pars_backup_old[0] - pars_backup_old[3]) < (pars_backup_old[0] * 0.01) )
	{
		if (verbose_flag) printf(" + applying trick\n");
		tfLambdaSum->SetParameter(0, pars_backup_old[0] * 1.5);
		tfLambdaSum->SetParameter(3, pars_backup_old[3] * 0.5);

		chi2_backup_old *= 2.;
	}

	if (verbose_flag)
	{
		// print them
		printf("* old: ");
		for (uint i = 0; i < par_num; ++i)
			printf("%g ", pars_backup_old[i]);
		printf(" --> chi2:  %f -- *\n", chi2_backup_old);
	}

	hist->Fit(tfLambdaSum, pars, gpars, hfp.fun_l, hfp.fun_u);

	TF1 * new_sig_func = ((TF1*)hist->GetListOfFunctions()->At(0));

// 	TVirtualFitter * fitter = TVirtualFitter::GetFitter();
// 	TMatrixDSym cov;
// 	fitter->GetCovarianceMatrix()
// 	cov.Use(fitter->GetNumberTotalParameters(), fitter->GetCovarianceMatrix());
// 	cov.Print();

	// backup new parameters
	double * pars_backup_new = new double[par_num];
	tfLambdaSum->GetParameters(pars_backup_new);
	double chi2_backup_new = hist->Chisquare(tfLambdaSum, "R");

	if (verbose_flag)
	{
		printf("  new: ");
		for (uint i = 0; i < par_num; ++i)
			printf("%g ", pars_backup_new[i]);
		printf(" --> chi2:  %f -- *", chi2_backup_new);
	}

	if (chi2_backup_new > chi2_backup_old)
	{
		tfLambdaSum->SetParameters(pars_backup_old);
		new_sig_func->SetParameters(pars_backup_old);
		printf("\n\tFIT-ERROR: Fit got worse -> restoring params for chi2 = %g", hist->Chisquare(tfLambdaSum, "R"));
	}
	else if (tfLambdaSum->GetMaximum() > 2.0 * hist->GetMaximum())
	{
		tfLambdaSum->SetParameters(pars_backup_old);
		new_sig_func->SetParameters(pars_backup_old);
		printf("\n\tMAX-ERROR: %g vs. %g -> %f (entries=%g)", tfLambdaSum->GetMaximum(), hist->GetMaximum(),
			   hist->Chisquare(tfLambdaSum, "R"), hist->GetEntries() );
		printf("\n");

		printf("  old: ");
		for (uint i = 0; i < par_num; ++i)
			printf("%g ", pars_backup_old[i]);
		printf(" --> chi2:  %f -- *\n", chi2_backup_old);

		printf("  new: ");
		for (uint i = 0; i < par_num; ++i)
			printf("%g ", pars_backup_new[i]);
		printf(" --> chi2:  %f -- *\n", chi2_backup_new);

		for (int i = 0; i < hist->GetNbinsX(); ++i)
			printf(" %g", hist->GetBinContent(i+1));
		printf("\n");
	}
	else
	{
// 		printf("\n\tIS-OK: %g vs. %g -> %f", tfLambdaSum->GetMaximum(), hist->GetMaximum(),
// 			   hist->Chisquare(tfLambdaSum, "R") );

		if (verbose_flag) printf("\t [ OK ]\n");

		hfp.update(tfLambdaSum);
	}

	tfLambdaSum->SetChisquare(hist->Chisquare(tfLambdaSum, "R"));

	new_sig_func->SetChisquare(hist->Chisquare(tfLambdaSum, "R"));

	const size_t npar = tfLambdaSum->GetNpar();

	for (uint i = 0; i < npar; ++i)
	{
		double par = tfLambdaSum->GetParameter(i);
		double err = tfLambdaSum->GetParError(i);

		tfLambdaSig->SetParameter(i, par);
		tfLambdaBkg->SetParameter(i, par);

		tfLambdaSig->SetParError(i, err);
		tfLambdaBkg->SetParError(i, err);
	}

	hist->GetListOfFunctions()->Add(tfLambdaSig);
	hist->GetListOfFunctions()->Add(tfLambdaBkg);

// 	hfp.SetOwner(false);
	delete tfLambdaSum;
	delete [] pars_backup_old;
	delete [] pars_backup_new;

	return true;
}

void FitterFactory::print() const
{
	std::map<TString, HistFitParams>::const_iterator it;
	for (it = hfpmap.begin(); it != hfpmap.end(); ++it)
		it->second.print();
}

void FitterFactory::cleanup()
{
	std::map<TString, HistFitParams>::iterator it;
	for (it = hfpmap.begin(); it != hfpmap.end(); ++it)
		it->second.cleanup();

	hfpmap.clear();
}

std::string FitterFactory::format_name(const std::string & name) const
{
	if (ps_prefix == PS_IGNORE and ps_suffix == PS_IGNORE and !rep_src.length())
		return name;

	std::string formatted = name;

	if (ps_prefix == PS_APPEND and ps_suffix == PS_APPEND)
		return prefix + name + suffix;

	if (ps_prefix == PS_APPEND)
		formatted = prefix + formatted;

	if (ps_suffix == PS_APPEND)
		formatted = formatted + suffix;

	if (ps_prefix == PS_SUBSTRACT)
	{
		size_t pos = formatted.find(prefix);
		if (pos == 0)
			formatted = formatted.substr(prefix.length(), std::string::npos);
	}

	if (ps_suffix == PS_SUBSTRACT)
	{
		size_t pos = formatted.find(suffix, formatted.length() - suffix.length());
		if (pos != std::string::npos)
			formatted = formatted.substr(0, pos);
	}

	if (rep_src.length())
	{
		size_t pos = formatted.find(rep_src);
		if (pos != std::string::npos)
			formatted = formatted.replace(pos, rep_src.length(), rep_dst);
	}

	return formatted;
}