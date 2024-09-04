![HelloFitty: fitting tool](https://img.shields.io/badge/HelloFitty-fitting%20tool-orange)
[![Coverage Status](https://coveralls.io/repos/github/rlalik/HelloFitty/badge.svg)](https://coveralls.io/github/rlalik/HelloFitty)
[![Compiler Checks](https://github.com/rlalik/HelloFitty/actions/workflows/sys_checks.yml/badge.svg)](https://github.com/rlalik/HelloFitty/actions/workflows/sys_checks.yml)
[![clang-format Check](https://github.com/rlalik/HelloFitty/actions/workflows/clang-format-check.yml/badge.svg)](https://github.com/rlalik/HelloFitty/actions/workflows/clang-format-check.yml)
![GitHub](https://img.shields.io/github/license/rlalik/HelloFitty)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/rlalik/HelloFitty)
![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/rlalik/HelloFitty)

<p align="center"><img src="docs/HelloFitty.svg" width="100" alt="HelloFitty logo"></p>
HelloFitty allows for massive fitting of the histograms. It is a tool designed for use with CERN's ROOT framework.

# Concept
Assume you have hundreds of histograms to be fitted. The fit may fail, they may require to be iteratively fitted. Hard coding fit params in the macro will require to update them each time you want to refit.

Instead, create a text file with:
* histogram name
* one or more functions to be fitted, e.g. two signal peaks functions plus common background
* set of initial parameters

Load the file and allow HelloFitty to fit them and write the output. If it is needed to refit same histograms (you can tune some of the parameters in the text file!) HelloFitty will take the already optimized inputs for the next fitting iteration.

## Simple example
The data file `input_params.txt`:
```text
 k0_peak 450 550 0 gaus(0) expo(3) |  100  490  10  10 1
```
to fit K0s peak in the range 450-550 MeV/c^2 range with `gasu(0)` as a signal function and `exp(3)` as a background function, with `Scale=100`, `Mean=490` and `Sigma=10` as initial parameters for `gaus(0)`, `10` and `1` are parameters for `exp(3)`.

The (simplified) macro:
```c++
{
    auto hist = std::make_unique<TH1F>("k0_peak", "K0s signal", 200, 400, 600);
    hist.Fill(...);         // Fill the histogram with data

    auto hf = hf::fitter();	// create HelloFitty object
    hf.init_from_file("input_params.txt", "output_params.txt"); // specify input file and output file
    hf.fit(hist.get());     // will search for  `k0_peak` in the input file, and if found, will fit it
    hf.export_to_file();    // store new params in output file
}
```

## Data file format
In the data file each line corresponds to a single histogram. The line is identified by the histogram name. The name is followed by various properties of the fit, in order:
1. Lower and upper fitting range
2. Rebin parameter (applied before fitting)
3. List of functions to be fitted, separated by a white space
4. ```|``` - separator
5. List of parameters

## Functions
Each function is an independent entity, and can be a combinations of various generic functions. Any form of function accepted by `TFormula` is allowed, e.g.: `cos(x)+sin(x)`, `gaus(0)+exp(3)`, `[0]*x+[2]`, etc.
For all the functions belonging to a single histogram, a sum of functions is created and the sum is fit. For example, to fit a gausian signal and a polynomial background, one could define two functions: `gaus(0) pol3(3)`. From the fitting point of view it does not matter whether you define two partial functions `gaus(0) pol3(3)` or one larger `gaus(0)+pol3(3)`, however HelloFitty offers ways to access each partial function separately, which would not be possible with one grand function.

## Parameters
After the `|` separator which marks end of function definitions, the parameters definitions start. There should be as many parameters defined as expected by the functions, and more or less parameters will result in throw of `hf::invalid_format`.
Parameters can be free, fixed, or constrained by fitting limits. If `X` is a parameter value and `Y`, `Z` are parameter limits, then possible notations are:
* `X` - just `X` as initial value without constrains,
* `X : Y Z` - `X` is limited to `Y -- Z` range,
* `X F Y Z` - `X` is fixed, but preserve the limits (just in case you want to make it a free and constrained parameter later),
* `X f` - `X` is fixed and no limits specified.

## A function entry example
```text
 test_hist  -10  10  2  gaus(0) expo(3) | 10 : 0 20 1 f 1 F 0 2 1 -1
# just comment                            ========= === ======= = ==
# just comment                                1      2     3    4  5
```
* `test_hist` is a histogram name
* `-10 10` is fitting x-range
* `2` is a histogram rebinning factor
* `gaus(0)` is a first function
* `expo(3)` is a second function
* `10 : 0 20 1 f 1 F 0 2 1 -1` are input parameters for fit function `gaus(0)+expo(3)`, where:
  * first marker `=======` shows parameter #1 with value `10` and limits `0`--`20`
  * second marker shows parameter #2 with fixed value `1`
  * third marker shows parameter #3 with fixed value `1` and preserved limits `0`--`2`
  * parameters #4 and #5 are free without limits

The entry can be prep-ended with `@` which tells fitter that this function is found but the fitting is explicitly disabled:
```text
 test_hist  -10  10  2  gaus(0) expo(3) | 10 : 0 20 1 f 1 F 0 2 1 -1
@some_hist  -10  10  2  gaus(0) expo(3) | 10 : 0 20 1 f 1 F 0 2 1 -1
```
The fitter allows to set default function which will be used for histograms not found in the data file. If the histogram is disabled, it won't be fit. If one would comment out the line, the default function could be used.

# Features
HelloFitty provides following structures:
* `hf::fitter` -- the main fitting manager responsible to read/write data from files and fit histograms
* `hf::fit_entry` -- a structure holding full info about functions and parameters for a given histogram
* `hf::param` -- a single parameter info: values and boundaries
* `hf::draw_opts` -- partial and grand function drawing options

## `hf::fitter`
Only default constructor available. You can add histogram fit functions definitions either by importing from data file or adding definitions manually.

If you import data from file you can set auxiliary (output) file to store fitted data.
```c++
auto init_from_file(std::string input_file) -> bool;
auto init_from_file(std::string input_file, std::string aux_file, priority_mode mode = priority_mode::newer) -> bool;
```
The first function is intended to read the data without intention to store the fitted results. The second sets also auxiliary file which is the default target for exporting the fit updates. The `priority_mode` specifies how the sources are prioritized:
```c++
class hf::fitter {
    enum class priority_mode { reference, auxiliary, newer };
};
```
1. Read Reference (input)
2. Read Auxiliary (aux)
3. Read newer of them both (default behavior)

With the third option, you can repeat fitting multiple time, each time improving result of the previous fit as the input will be taken from auxiliary, not reference file, until the reference itself has not been updated (e.g. you need better function body to converge fit and start over).

If you decided to store fit results to data file, use:
```c++
auto export_to_file(bool update_reference = false) -> bool;
```
By default it will update the auxiliary file unless `update_reference` is set to `true`.

You can search whether given histogram is present in the fitter (after loading from file), either using the histogram object or histogram name:
```c++
auto find_fit(TH1* hist) const -> fit_entry*;
auto find_fit(const char* name) const -> fit_entry*;
```
You can finally fit the histogram using the histogram object with the fit entry stored in the fitter, or histogram object and providing additional fit entry object:
```c++
auto fit(TH1* hist, const char* pars = "BQ", const char* gpars = "") -> bool;
auto fit(fit_entry* hfp, TH1* hist, const char* pars = "BQ", const char* gpars = "") -> bool;
```
In addition to importing from file, you can add additional fit entries to the fitter:
```c++
auto insert_parameter(std::pair<std::string, fit_entry> hfp) -> void;
auto insert_parameter(const std::string& name, std::unique_ptr<fit_entry> hfp) -> void;
```
To set default fitting function for histograms not present in the histogram entries collection, use
```c++
auto set_generic_entry(fit_entry generic) -> void;
```
Use of generic function will create an entry in the auxiliary file, e.g.:
```c++
ff.set_generic_entry(&generic_hfp);
auto h = TH1F("missing_in_input_name", ...);
ff.fit(&h2); // will not fail
```
The new `missing_in_input_name` will show up in the output file:
```text
 test_hist                  0 10 0  gaus(0) expo(3) | 5005.69  3.0004  -0.501326  8.91282  -0.501843
 missing_in_input_name      0 10 0  gaus(0) expo(3) | 5005.69  3.0004  -0.501326  8.91282  -0.501843
```
You may also want to have multiple entries of fit functions for a single histogram. For that you can use histogram name decorator. An example parameter file:
```text
 test_hist     0 10 0 gaus(0) expo(3) | 10 : 0 20  1 f  1 F 0 2  1  -1
 test_hist_v2  0 10 0 gaus(0) sin(3)  | 10 : 0 20  1 f  1 F 0 2  1  -1
 test_hist_v3  0 10 0 gaus(0) cos(3)  | 10 : 0 20  1 f  1 F 0 2  1  -1
```
and its fitting:
```c++
hf::fitter ff;
ff.init_from_file("testpars.txt", "testpars.out");
auto h = std::make_unique<TH1F>("test_hist", ...);
ff.fit(h.get());                // will fit definition from `test_hist`
ff.set_name_decorator("*_v2");
ff.fit(h.get());                // will fit definition from `test_hist_v2`
ff.set_name_decorator("*_v3");
ff.fit(h.get());                // will fit definition from `test_hist_v3`
ff.clear_name_decorator();
ff.fit(h.get());                // will fit definition from `test_hist` again
```
The decorator could have form of text string with a character `*`, which will be replaced with the original histogram name:
* decorator `*_v1` on `hist_name` will give `hist_name_v1`
* but decorator `_v1` on `hist_name` will give `_v1`

## `hf::fit_entry`
The fit entry can be created by parsing the input file or created by user and provided to the fitter:
```c++
explicit fit_entry(Double_t range_lower, Double_t range_upper);
```
The fit entry can be read from file (as shown above) or created from the code level:
```c++
hf::fit_entry hfp(0, 10);
auto fid1 = hfp.add_function("gaus(0)");                    // function id = 0
auto fid2 = hfp.add_function("expo(3)");                    // function id = 1
hfp.set_param(0, 10, 0, 20, hf::param::fit_mode::free);     // gaus(0) par [0]
hfp.set_param(1, 1, hf::param::fit_mode::fixed);            // gaus(0) par [1]
hfp.set_param(2, 1, 0, 2, hf::param::fit_mode::fixed);      // gaus(0) par [2]
hfp.set_param(3, 1);                                        // exp(3) par [3]
hfp.set_param(4, -1);                                       // exp(3) par [4]
```
is the same like
```text
 h1 0 10 0 gaus(0) expo(3) | 10 : 0 20  1 f  1 F 0 2  1  -1
```
Each fit entry has own backup storage. You can copy and restore parameters from storage, and clear storage.
```c++
auto backup() -> void;
auto restore() -> void;
auto drop() -> void;
```
You can use this before fit and fit result is of worse quality then one can restore original parameters.
```c++
auto hfp = hf::fit_entry(0, 10);
hfp.add_function(...);
// and more...

auto fitter = hf::fitter();
hfp.backup();
fitter.fit(&hfp, hist, ...);
if (hist->GetChisquare() > 100) {   // very bad chi2
    hfp.restore();
} else {
    hfp.drop();
}
```

# Compilation/Installation
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install # optional, requires -DCMAKE_INSTALL_PREFIX=PATH for cmake call
```

# Testing
```bash
$ make test     # runs tests wia ctest
$ tests/gtests  # runs tests directly via gtests
```

# Builtin examples
Two examples are provided:
1. `example1` - creates histogram and input file with signal and background functions and then reads the input, fits histogram and stores output
   ```bash
   $ examples/example1
   ```
1. `example2` - opens root file, searches for histograms and tries to fit them using fits from input
   ```bash
   $ examples/example2 examples/testhist.root examples/testpars.txt # input files created by example1
   ```

# Usage with CMake projects
The cmake files provide target to link your targets against. The target is located in the `HelloFitty` namespace as `HelloFitty::HelloFitty`. Example of usage:
```cmake
find_package(HelloFitty)

target_link_libraries(your_target
    PUBLIC
        HelloFitty::HelloFitty
)
```

# Contributing
To contribute, pelase create pull request from your fork. Please compile the project in developer mode using preset:
```bash
cmake . --preset dev
```
Additional CMake options:
* `-DBUILD_MCSS_DOCS=ON` -- build Doxygen documentation
* `-DENABLE_COVERAGE=ON` -- built code coverage support

Useful `make` targets:
* `format-check` -- check code with clang-format
* `format-fix` -- fix formatting (required for pull request)
* `spell-check` -- check code for common spell errors, ignore words can be add to `.codespellrc`, see codespell documentation for details: https://github.com/codespell-project/codespell#ignoring-words
* `spell-fix` -- automatic fix spelling
* `docs` -- build documentation
* `coverage` -- build coverage report (only if `ENABLE_COVERAGE` is active)
* `help` -- full list of targets
