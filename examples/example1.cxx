#include <FitterFactory.h>

#include "ffconfig.h"

#include <TFile.h>
#include <TH1.h>

#include <fstream>
#include <iostream>

void create_input_file(const TString& filename)
{
    std::ifstream parfile(filename, std::ifstream::in);
    if (!parfile.is_open())
    {
        std::cout << "Creating parameter file\n";
        std::ofstream parfile2(filename, std::ofstream::out);
        if (parfile2.is_open())
        {
            parfile2 << " test_hist gaus(0) expo(3)  0 0 10 10 1 1 1 -1" << std::endl;
            parfile2.close();
        }
        else
        {
            std::cerr << "Parameter file can't be created, unable to execute example." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    else
    {
        std::cout << "Good, parameter file " << filename << " exists." << std::endl;
    }
};

TH1I* create_root_file(const TString& filename)
{
    TH1I* unnamed = new TH1I("test_hist", "", 100, 0, 10);
    unnamed->SetBinContent(1, 7290);
    unnamed->SetBinContent(2, 6750);
    unnamed->SetBinContent(3, 6383);
    unnamed->SetBinContent(4, 6255);
    unnamed->SetBinContent(5, 6037);
    unnamed->SetBinContent(6, 5577);
    unnamed->SetBinContent(7, 5327);
    unnamed->SetBinContent(8, 5070);
    unnamed->SetBinContent(9, 4779);
    unnamed->SetBinContent(10, 4695);
    unnamed->SetBinContent(11, 4444);
    unnamed->SetBinContent(12, 4276);
    unnamed->SetBinContent(13, 3976);
    unnamed->SetBinContent(14, 3939);
    unnamed->SetBinContent(15, 3582);
    unnamed->SetBinContent(16, 3434);
    unnamed->SetBinContent(17, 3419);
    unnamed->SetBinContent(18, 3366);
    unnamed->SetBinContent(19, 3302);
    unnamed->SetBinContent(20, 3376);
    unnamed->SetBinContent(21, 3581);
    unnamed->SetBinContent(22, 3623);
    unnamed->SetBinContent(23, 3999);
    unnamed->SetBinContent(24, 4424);
    unnamed->SetBinContent(25, 4999);
    unnamed->SetBinContent(26, 5350);
    unnamed->SetBinContent(27, 5976);
    unnamed->SetBinContent(28, 6264);
    unnamed->SetBinContent(29, 6523);
    unnamed->SetBinContent(30, 6633);
    unnamed->SetBinContent(31, 6544);
    unnamed->SetBinContent(32, 6289);
    unnamed->SetBinContent(33, 5859);
    unnamed->SetBinContent(34, 5367);
    unnamed->SetBinContent(35, 4723);
    unnamed->SetBinContent(36, 4021);
    unnamed->SetBinContent(37, 3350);
    unnamed->SetBinContent(38, 2801);
    unnamed->SetBinContent(39, 2279);
    unnamed->SetBinContent(40, 1833);
    unnamed->SetBinContent(41, 1502);
    unnamed->SetBinContent(42, 1245);
    unnamed->SetBinContent(43, 1069);
    unnamed->SetBinContent(44, 958);
    unnamed->SetBinContent(45, 953);
    unnamed->SetBinContent(46, 747);
    unnamed->SetBinContent(47, 726);
    unnamed->SetBinContent(48, 704);
    unnamed->SetBinContent(49, 656);
    unnamed->SetBinContent(50, 594);
    unnamed->SetBinContent(51, 585);
    unnamed->SetBinContent(52, 586);
    unnamed->SetBinContent(53, 518);
    unnamed->SetBinContent(54, 468);
    unnamed->SetBinContent(55, 490);
    unnamed->SetBinContent(56, 476);
    unnamed->SetBinContent(57, 423);
    unnamed->SetBinContent(58, 436);
    unnamed->SetBinContent(59, 407);
    unnamed->SetBinContent(60, 364);
    unnamed->SetBinContent(61, 380);
    unnamed->SetBinContent(62, 345);
    unnamed->SetBinContent(63, 322);
    unnamed->SetBinContent(64, 321);
    unnamed->SetBinContent(65, 319);
    unnamed->SetBinContent(66, 277);
    unnamed->SetBinContent(67, 264);
    unnamed->SetBinContent(68, 228);
    unnamed->SetBinContent(69, 238);
    unnamed->SetBinContent(70, 218);
    unnamed->SetBinContent(71, 236);
    unnamed->SetBinContent(72, 221);
    unnamed->SetBinContent(73, 201);
    unnamed->SetBinContent(74, 196);
    unnamed->SetBinContent(75, 210);
    unnamed->SetBinContent(76, 153);
    unnamed->SetBinContent(77, 146);
    unnamed->SetBinContent(78, 154);
    unnamed->SetBinContent(79, 130);
    unnamed->SetBinContent(80, 131);
    unnamed->SetBinContent(81, 135);
    unnamed->SetBinContent(82, 115);
    unnamed->SetBinContent(83, 117);
    unnamed->SetBinContent(84, 126);
    unnamed->SetBinContent(85, 100);
    unnamed->SetBinContent(86, 107);
    unnamed->SetBinContent(87, 78);
    unnamed->SetBinContent(88, 73);
    unnamed->SetBinContent(89, 93);
    unnamed->SetBinContent(90, 72);
    unnamed->SetBinContent(91, 78);
    unnamed->SetBinContent(92, 73);
    unnamed->SetBinContent(93, 90);
    unnamed->SetBinContent(94, 65);
    unnamed->SetBinContent(95, 55);
    unnamed->SetBinContent(96, 64);
    unnamed->SetBinContent(97, 63);
    unnamed->SetBinContent(98, 67);
    unnamed->SetBinContent(99, 62);
    unnamed->SetBinContent(100, 55);
    unnamed->SetEntries(210000);

    TFile* fp = TFile::Open(filename, "RECREATE");
    if (fp)
        unnamed->Write();
    else
    {
        std::cerr << "File " << filename << " not open\n";
        abort();
    }

    fp->Close();

    return unnamed;
}

int main()
{
    auto root_file_name = TString(examples_bin_path) + "test_hist.root";
    auto hist = create_root_file(root_file_name);
    auto root_outout_name = TString(examples_bin_path) + "test_output.root";

    auto input_name = TString(examples_bin_path) + "test_input.txt";
    create_input_file(input_name);

    auto output1_name = TString(examples_bin_path) + "test_output1.txt";
    auto output2_name = TString(examples_bin_path) + "test_output2.txt";
    auto output3_name = TString(examples_bin_path) + "test_output3.txt";

    FitterFactory ff;
    ff.setDrawBits(true, true, true);
    ff.propBkg().setLineColor(1).setLineWidth(2).setLineStyle(9);
    ff.propSig().setLineColor(1).setLineWidth(1).setLineStyle(2);

    /** First usage using HFP object **/
    printf("\n ---- FIRST USAGE ---\n\n");
    ff.initFactoryFromFile(input_name, output1_name);

    auto hfp = ff.findFit("test_hist");
    if (hfp)
    {
        printf("\nBefore fitting:\n");
        hfp->print();
        printf("\n");

        hfp->push();
        if (!ff.fit(hfp, hist)) hfp->pop();

        printf("\nAfter fitting:\n");
        hfp->print(true);
        printf("\n");
    }
    else
    {
        std::cerr << "No function found" << std::endl;
    }
    ff.exportFactoryToFile();

    TFile* fp = TFile::Open(root_outout_name, "RECREATE");
    if (fp)
        hist->Write();
    else
    {
        std::cerr << "File " << root_outout_name << " not open\n";
        abort();
    }
    fp->Close();

    /** Second usage using histogram object **/
    printf("\n ---- SECOND USAGE ---\n\n");
    ff.initFactoryFromFile(input_name, output2_name);

    printf("\nBefore fitting:\n");
    ff.print();

    if (!ff.fit(hist)) { std::cerr << "No function found" << std::endl; }

    printf("\nAfter fitting:\n");
    ff.print();

    ff.exportFactoryToFile();

    FitterFactory ff2(FitterFactory::PriorityMode::Reference);

    /** Third usage using histogram object **/
    printf("\n ---- THIRD USAGE ---\n\n");
    ff.initFactoryFromFile(input_name, output3_name);

    printf("\nBefore fitting:\n");
    ff.print();

    if (!ff.fit(hist)) { std::cerr << "No function found" << std::endl; }

    printf("\nAfter fitting:\n");
    ff.print();

    ff.exportFactoryToFile();

    return 0;
}
