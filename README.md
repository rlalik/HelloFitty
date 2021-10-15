FitterFactory allows for massive fitting of the histograms. It is a tool designed for use with CERN's ROOT framework.

See example below for usage case.

## Features
1. Read parameters from file and store to file
You cans specify input (reference) and output (auxiliary) file and one of three priority access modes:
 1. Read Reference
 2. Read Auxiliary
 3. Read newer of both
With the third option, you can repeat fitting multiple time, each time improving result of the previous fit as the input will be taken from auxiliary, not reference file, until the reference has not updated timestamp.
```c++
class FitterFactory {
    enum class PriorityMode { Reference, Auxilary, Newer };
}
```
1. Fit functions stored in a text file in ascii format:
```text
 test_hist gaus(0) expo(3)  0 0 10 10 1 1 1 -1
```
where:
 * `test_hist` is a histogram name
 * `gaus(0)` is a signal function
 * `expo(3)` is a background function
 *`0` is a histogram rebinning factor
 * `0 10` is fitting x-range
 * `10 1 1 1 -1` are input parameters for fit function `gaus(0)+expo(3)`
The fit function is a sum of signal and background: `gaus(0)+expo(3)`; if you do not need both, use `0` instead, example `gaus(0)+0`.

1. Parameters can be free, fixed, or constrained, if `X` is a param and `Y`, `Z` are parameter limits, then possible notations are:
  * `X` - just `X`
  * `X : Y Z` - `X` is limited to `Y -- Z` range
  * `X F Y Z` - `X`is fixed, but preserve the limits (just in case you want to make it a free parameter)
  * `X f` - `X` is foxed and no limit specified.
An example:
```text
 test_hist gaus(0) expo(3)  0 0 10 10 : 0 20 1 f 1 F 0 2 1 -1
                                                      =======  == ======
```
  * first marker shows parameter #1 `10` with limits `0 -- 20`
  * second marker shows parameter #2 with fixed value `1`
  * third marker shows parameter #3 with fixed value `1` and preserved limits `0 -- 2`
  * parameters #4 and #5 are free without limits

1. Fitting functions can be stored in a file (as shown above) or created fro mthe code level:
```c++
HistogramFitParams hfp("h1", "gaus(0)", "expo(3)", 0, 10);
hfp.setParam(0, 10, 0, 20, ParamValue::FitMode::Free);
hfp.setParam(1, 1, ParamValue::FitMode::Fixed);
hfp.setParam(2, 1, 0, 2, ParamValue::FitMode::Fixed);
hfp.setParam(3, 1);
hfp.setParam(4, -1);
```
is a equivalent of example above.

1. Create clone of an existing functions and parameters set (to avoid duplication of histogram names, copying is forbidden).
```c++
HistogramFitParams hfp("h1", "gaus(0)", "expo(3)", 0, 10);
auto hfp2 = hfp.clone("h2");
```

1. If a histogram name is prepended with `@` then the histogram will not be fit.
```text
 test_hist gaus(0) expo(3)  0 0 10 10 : 0 20 1 f 1 F 0 2 1 -1
@test_hist gaus(0) expo(3)  0 0 10 10 : 0 20 1 f 1 F 0 2 1 -1
```

1. You may want to have multiple sets of fit functions for a single histogram. For that you can use histogram name decorator. An example parameter file:
```text
 test_hist gaus(0) expo(3)  0 0 10 10 : 0 20 1 f 1 F 0 2 1 -1
 test_hist_v2 gaus(0) sin(3)  0 0 10 10 : 0 20 1 f 1 F 0 2 1 -1
 test_hist_v3 gaus(0) cos(3)  0 0 10 10 : 0 20 1 f 1 F 0 2 1 -1
```
and its fitting:
```c++
FitterFactory ff;	// create FitterFactory object
ff.initFactoryFromFile("testpars.txt", "testpars.out"); // specify input file and output file
TH1 * h = new TH1F("test_hist", ...);
ff.fit(h);   // will fit `test_hist`
ff.setNameDecorator("*_v2");
ff.fit(h);   // will fit `test_hist_v2`
ff.setNameDecorator("*_v3");
ff.fit(h);   // will fit `test_hist_v3`
ff.clearNameDecorator();
ff.fit(h);   // will fit `test_hist` again
```
The decorator must have form of text string with a character `*`, which will be replaced with the original histogram name:
  * decorator `*_v1` on `hist_name` will give `hist_name_v1`
  * but decorator `_v1` on `hist_name` will give `_v1`

### Example usage

1. Assume there is a histogram `test_hist`
2. Create file `test_parameters.txt` containing fitting function and parameters.
3. Run code
```c++
    FitterFactory ff;	// create FitterFactory object
    ff.initFactoryFromFile("testpars.txt", "testpars.out"); // specify input file and output file

    HistogramFitParams hfp = ff.findParams("test_hist"); // try to find parameters for histogram `test_hist`
    if (hfp)
    {
        hfp->push();                        // preserve default parameters, just in case, optional
        if (!ff.fit(hfp, unnamed))     // try to fit
            hfp->pop();                     // if fit failed, you can restore previous parameters
    }
    else
    {
        std::cerr << "No function found" << std::endl;
    }
    ff.exportFactoryToFile();        // save parameters to output file
```
4. The output file contains:
```text
 test_hist      gaus(0) expo(3) 0 0 10 5005.69 3.0004 -0.501326 8.91282 -0.501843
```

## Compilation/Installation

$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install # optional

## Testing

$ make test