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


#ifndef FITTERFACTORY_H
#define FITTERFACTORY_H

#include <map>

#include <TF1.h>
#include <TH1.h>

struct ParamValues
{
	double val;		// value
	double l;			// lower limit
	double u;			// upper limit
	enum ParamFlags { FREE, FIXED } flag;
	bool has_limits;

	ParamValues() : val(0), l(0), u(0), flag(FREE), has_limits(false) {}
	void print();
};

class HistFitParams
{
public:
	TString histname;	// keeps hist name
	TString funname;	// keeps func name
	TString f_sig, f_bkg;
// 	TString func;		// fit function
// 	TString func;		// fit function
	Int_t allparnum;	// num of pars
	Int_t rebin;		// rebin, 0 == no rebin
	Double_t fun_l;		// function range
	Double_t fun_u;		// function range
	Bool_t fit_disabled;

	ParamValues * pars;
	TF1 * funSig;
	TF1 * funBkg;
	TF1 * funSum;

	HistFitParams();
	HistFitParams(const HistFitParams & hfp);
	~HistFitParams();
	HistFitParams & operator=(const HistFitParams & hfp);
	void init(const TString & h, const TString & fsig, const TString & fbg, Int_t bgn, Double_t f_l, Double_t f_u);
	void setParam(Int_t par, Double_t val, ParamValues::ParamFlags flag);
	void setParam(Int_t par, Double_t val, Double_t l, Double_t u, ParamValues::ParamFlags flag);
	void setNewName(const TString & new_name);
	void print() const;
	void printInline() const;
	bool update(TF1 * f);

	void cleanup();
	inline void setOwner(bool owner);

	TString exportEntry() const;

	static const HistFitParams parseEntryFromFile(const TString & line);

	static void printStats(char * infotext = nullptr);
// 	TF1 *fitV;
private:
	bool is_owner;

	static long int cnt_total;
	static long int cnt_owned;
};

typedef std::pair<TString, HistFitParams> HfpEntry;

class FitterFactory
{
public:
	enum FLAGS { ALWAYS_REF, ALWAYS_AUX, ALWAYS_NEWER };
	enum FIND_FLAGS { NOT_FOUND, USE_FOUND, USE_DEFAULT };
	enum PSFIX { PS_APPEND, PS_IGNORE, PS_SUBSTRACT };

	FitterFactory(FLAGS flags = ALWAYS_NEWER);
	virtual ~FitterFactory();

	void cleanup();

	FLAGS setFlags(FLAGS new_flags);
	void setDefaultParameters(HistFitParams defs);

	bool initFactoryFromFile(const char * filename, const char * auxname = 0);
	bool exportFactoryToFile();

	FIND_FLAGS findParams(TH1 * hist, HistFitParams & hfp, bool use_defaults = true) const;
	FIND_FLAGS findParams(const char * name, HistFitParams & hfp, bool use_defaults = true) const;
	bool updateParams(TH1 * hist, HistFitParams & hfp);
	bool updateParams(TH1 * hist, TF1 * f);

	bool fit(TH1 * hist, const char * pars = "B,Q", const char * gpars = "");
	static bool fit(HistFitParams & hfp, TH1 * hist, const char * pars = "B,Q", const char * gpars = "", double min_entries = 0);

	void print() const;

	static void setVerbose(bool verbose) { verbose_flag = verbose; }

	inline void setEntriesLowLimit(double min) { min_entries = min; }

	inline void setPrefix(const std::string & str) { prefix = str; }
	inline void setSuffix(const std::string & str) { suffix = str; }
	inline void setReplacement(const std::string & src, const std::string & dst) { rep_src = src; rep_dst = dst; }

	inline void setPrefixManipulator(PSFIX manip = PS_IGNORE) { ps_prefix = manip; }
	inline void setSuffixManipulator(PSFIX manip = PS_IGNORE) { ps_suffix = manip; }

	std::string format_name(const std::string & name) const;

	void insertParameters(const HistFitParams & hfp);
	void insertParameters(const TString & name, const HistFitParams & hfp);
	void insertParameters(const HfpEntry & par);

private:
	bool import_parameters(const char * filename);
	bool export_parameters(const char * filename);

	FitterFactory::FLAGS flags;

	bool has_defaults;
	static bool verbose_flag;

	const char * par_ref;
	const char * par_aux;

	HistFitParams defpars;
	std::map<TString, HistFitParams> hfpmap;

	double min_entries;

	std::string prefix;
	std::string suffix;

	PSFIX ps_prefix;
	PSFIX ps_suffix;

	std::string rep_src;
	std::string rep_dst;
};

#endif // FITTERFACTORY_H
