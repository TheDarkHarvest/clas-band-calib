#include <iostream>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <fstream>

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TImage.h"
#include "TStyle.h"

using namespace std;
void find_gain(int numRuns, TGraph * data, std::vector<double> * HVs, double &A, double &B, double &Aerr, double &Berr, double &HV_15000);
void find_size(int &step, int start, string pmt_number);


struct BarInfo{
	int id;
	int sector, layer, component;
	std::vector<double> meansL;
	std::vector<double> meansR;
	std::vector<double> meansLerr;
	std::vector<double> meansRerr;
};
struct find_bar{
	int uniqueID;
	find_bar(int id): uniqueID(id){ }
	bool operator()( BarInfo const& bar) const{
		return bar.id == uniqueID;
	}
};

int main(int argc, char ** argv)
{
	if (argc != 3)
	{
		cerr << "Wrong number of arguments. Instead use:\n"
			<< "\tpmt_test /path/to/gain/test/txt /path/to/output/file/txt\n";
		return -1;
	}

	ifstream inFile;
	inFile.open( argv[1] );
	char line[256];
	inFile.getline( line, 256 );
	double hv, leftMean, leftSigma, rightMean, rightSigma;
	int layer, sector, component;

	std::vector< BarInfo > Bars;
	std::vector< double > HVs;
	std::vector< double > HVerr;
	if (inFile.is_open() == true){
		while ( inFile >> hv >> sector >> layer >> component >> \
				leftMean >> leftSigma >> rightMean >> rightSigma ) {
	
			int ID = layer*5 + layer + sector*5*sector + component*7 + component;
			std::vector< BarInfo >::iterator it;
			it = std::find_if( Bars.begin(), Bars.end(), find_bar( ID ) );
			// If I've already started saving this bar
			if( it != Bars.end() ){
				int idx = std::distance( Bars.begin() , it );
				BarInfo bar = Bars.at(idx);
				Bars.at(idx).meansL.push_back( leftMean );
				Bars.at(idx).meansR.push_back( rightMean );
				Bars.at(idx).meansLerr.push_back( leftSigma );
				Bars.at(idx).meansRerr.push_back( rightSigma );
			}
			else{	// Otherwise create and add new bar
				BarInfo bar;
				bar.id = ID;
				bar.sector = sector;
				bar.layer = layer;
				bar.component = component;
				bar.meansL.push_back( leftMean );
				bar.meansR.push_back( rightMean );
				bar.meansLerr.push_back( leftSigma );
				bar.meansRerr.push_back( rightSigma );
				Bars.push_back( bar );
			}

			if( std::find( HVs.begin() , HVs.end() , hv ) == HVs.end() ){
				HVs.push_back( hv );
				HVerr.push_back( 0 );	
			}
			cout << ID << " " << hv << " " << sector << " " << layer << " " << component << " " << leftMean <<  " " << rightMean <<"\n";
		}
	}
	
	
	ofstream outFile;
	outFile.open( argv[2] );
	for( int i = 0 ; i < Bars.size() ; i++){
		BarInfo bar = Bars.at(i);
		
		//cout << bar.id << "\n";
		//for( int j = 0 ; j < bar.meansL.size() ; j++ ){
		//	cout << bar.meansL.at(j) << " " << bar.meansR.at(j) << "\n";
		//}
		
		std::vector<double> mL = bar.meansL;
		std::vector<double> mR = bar.meansR;
		std::vector<double> mLerr = bar.meansLerr;
		std::vector<double> mRerr = bar.meansRerr;
		
		TGraphErrors dataL( HVs.size() , &HVs[0] , &mL[0] , &HVerr[0] , &mLerr[0] );
		TGraphErrors dataR( HVs.size() , &HVs[0] , &mR[0] , &HVerr[0] , &mRerr[0] );

		double A_l, B_l, Aerr_l, Berr_l;
		double A_r, B_r, Aerr_r, Berr_r;
		double HV_l, HV_r;
		
		A_l = mL.at(0);
		A_r = mR.at(0);

		find_gain( HVs.size() , &dataL, &HVs, A_l, B_l, Aerr_l, Berr_l, HV_l );	
		find_gain( HVs.size() , &dataR, &HVs, A_r, B_r, Aerr_r, Berr_r, HV_r );	
		

		TCanvas *c = new TCanvas;
		c->Divide(2,1);
		TString name;

		gStyle->SetOptFit(1);
		gStyle->SetStatX(0.5);
		gStyle->SetStatY(0.9);
		gStyle->SetStatW(0.2);
		gStyle->SetStatH(0.2);
	
		c->cd(1);
		name = Form("Layer %i, Sector %i, Component %i, L PMT",bar.layer, bar.sector, bar.component);
		dataL.SetTitle(name);
		dataL.GetXaxis()->SetTitle("HV [kV]");
		dataL.GetYaxis()->SetTitle("ADC Channel");
		dataL.GetYaxis()->SetTitleOffset(1.6);
		dataL.SetMarkerStyle(20);
		dataL.SetMarkerColor(2);
		dataL.Draw("AP");

		c->cd(2);
		name = Form("Layer %i, Sector %i, Component %i, R PMT",bar.layer, bar.sector, bar.component);
		dataR.SetTitle(name);
		dataR.GetXaxis()->SetTitle("HV [kV]");
		dataR.GetYaxis()->SetTitle("ADC Channel");
		dataL.GetYaxis()->SetTitleOffset(1.6);
		dataR.SetMarkerStyle(20);
		dataR.SetMarkerColor(2);
		dataR.Draw("AP");
		
		c->Update();

		name = Form("layer%i_sector%i_comp%i",bar.layer,bar.sector,bar.component);
		c->SaveAs("/home/segarrae/data/band/gain_results/" + name + "_gainCurve.pdf");
	
		outFile << bar.layer << " " << bar.sector << " " << bar.component << " " << HV_l << " " << HV_r << "\n";
			// A_l << " " << B_l << " " << A_r << " " << B_r << "\n";
	}

	// Clean up
	outFile.close();
	
	return 0;
}

void find_gain(int numRuns, TGraph * data, std::vector<double> * HVs, double &A, double &B, double &Aerr, double &Berr, double &HV_15000)
{
	
	TF1 myFunc("myFunc","[0]*(x^[1])", HVs->at(0) , HVs->at(numRuns-1) );

	myFunc.SetParameter(0,A);
	myFunc.SetParameter(1,7);

	TFitResultPtr fitRes = data->Fit(&myFunc,"QES","",HVs->at(0)-200.,HVs->at(numRuns-1)+200.);
	cout << "\n\tGain Functional for PMT is:\tA = " << fitRes->Parameter(0) << "\t\tB [GAIN] = " << fitRes->Parameter(1) << "\n";	
	
	A = fitRes->Parameter(0);
	Aerr = fitRes->ParError(0);
	B = fitRes->Parameter(1);
	Berr = fitRes->ParError(1);

	HV_15000 = pow(15000./A,1./B) * 1000;
	cout << "\t\tHV at ADC 15000: " << HV_15000 << "\n";
	
}

