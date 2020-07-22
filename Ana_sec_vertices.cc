
#include "./functions_BW.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

void Ana_sec_vertices()
{



    //--------------------------
    printf("Ana_sec_vertices started \n");

    //gStyle->SetPalette(kDarkBodyRadiator); // https://root.cern.ch/doc/master/classTColor.html
    //gStyle->SetPalette(kInvertedDarkBodyRadiator); // https://root.cern.ch/doc/master/classTColor.html
    gStyle->SetPalette(kTemperatureMap); // https://root.cern.ch/doc/master/classTColor.html
    //--------------------------



    //--------------------------
    // Load TRD geometry
    TEveManager::Create();

    TH1I* h_good_bad_TRD_chambers;
    h_good_bad_TRD_chambers = new TH1I("h_good_bad_TRD_chambers","h_good_bad_TRD_chambers",540,0,540);
    TFile* file_TRD_QA_flags = TFile::Open("./Data/chamber_QC_flags.root");
    vector<int> *t_flags;
    file_TRD_QA_flags ->GetObject("QC_flags", t_flags);

    // Its a 3 digit binary number. LSB is ADC good = 0 or bad = 1, next bit is anode HV good = 0, or bad = 1, and last bit is drift HV
    // so a 3 means that the ADC and the anode HV was bad, but the drift HV was okay

    // LSB = official QA, bit 1 = no fit, bit 2 = anode HV defect, bit 3 = drift HV defect, bit 4 = adc defect

    // number   adc defect   drift HV defect   anode HD defect    no fit   official QA
    //   0          0               0                0               0          0         --> all good
    //   1          0               0                0               0          1         --> official QA bad, rest good
    //  ...
    //   31         1               1                1               1          1         --> all bad

    Int_t i_chamber = 0;
    for(vector<int>::iterator it = t_flags->begin(); it != t_flags->end(); ++it)
    {
        //cout << "chamber: " << i_chamber << ", it: "  << *it << ", " << t_flags->at(i_chamber) << endl;
        h_good_bad_TRD_chambers ->SetBinContent(i_chamber+1,t_flags->at(i_chamber));
        i_chamber++;
    }

    Int_t color_flag_QC[32];
    for(Int_t i_QC_flag = 0; i_QC_flag < 32; i_QC_flag++)
    {
        color_flag_QC[i_QC_flag] = kCyan;

        Int_t k_bit = 1; // fit
        Int_t bit_value = (i_QC_flag & ( 1 << k_bit )) >> k_bit;
        if(bit_value == 1) // no fit
        {
            color_flag_QC[i_QC_flag] = kPink;
        }

        k_bit = 4; // ADC value
        bit_value = (i_QC_flag & ( 1 << k_bit )) >> k_bit;
        if(bit_value == 1) // ADC low
        {
            color_flag_QC[i_QC_flag] = kMagenta;
        }

        k_bit = 2; // anode HV
        bit_value = (i_QC_flag & ( 1 << k_bit )) >> k_bit;
        if(bit_value == 1) // anode HV low
        {
            color_flag_QC[i_QC_flag] = kYellow;
        }

        k_bit = 3; // drift HV bit
        bit_value = (i_QC_flag & ( 1 << k_bit )) >> k_bit;
        if(bit_value == 1) // drift HV defect
        {
            color_flag_QC[i_QC_flag] = kOrange;
        }

        k_bit = 0; // official QA
        bit_value = (i_QC_flag & ( 1 << k_bit )) >> k_bit;
        if(bit_value == 1) // official QA bad
        {
            color_flag_QC[i_QC_flag] = kRed;
        }
    }
    color_flag_QC[31] = kRed;

    vector< vector<TH1D*> > vec_TH1D_TRD_geometry; // store for all 540 chambers the 8 corner vertices per detector
    TFile* file_TRD_geom = TFile::Open("./Data/TRD_Geom.root");
    vec_TH1D_TRD_geometry.resize(3); // x,y,z
    for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
    {
        vec_TH1D_TRD_geometry[i_xyz].resize(8); // 8 vertices
        for(Int_t i_vertex = 0; i_vertex < 8; i_vertex++)
        {
            HistName = "vec_TH1D_TRD_geometry_xyz_";
            HistName += i_xyz;
            HistName += "_V";
            HistName += i_vertex;
            vec_TH1D_TRD_geometry[i_xyz][i_vertex] = (TH1D*)file_TRD_geom->Get(HistName.Data());
        }

    }


    vector<TEveBox*> vec_eve_TRD_detector_box;
    vec_eve_TRD_detector_box.resize(540);
    vector< vector<TPolyLine*> > vec_PL_TRD_det_2D;
    vec_PL_TRD_det_2D.resize(18); // sectors
    for(Int_t i_sector = 0; i_sector < 18; i_sector++)
    {
        vec_PL_TRD_det_2D[i_sector].resize(6); // layers
        for(Int_t i_layer = 0; i_layer < 6; i_layer++)
        {
            vec_PL_TRD_det_2D[i_sector][i_layer] = new TPolyLine();
        }
    }
    for(Int_t TRD_detector = 0; TRD_detector < 540; TRD_detector++)
    {
        Int_t i_sector = (Int_t)(TRD_detector/30);
        Int_t i_stack  = (Int_t)(TRD_detector%30/6);
        Int_t i_layer  = TRD_detector%6;
        //Int_t i_det = layer + 6*stack + 30*sector;


        vec_eve_TRD_detector_box[TRD_detector] = new TEveBox;

        HistName = "TRD_box_";
        HistName += TRD_detector;
        vec_eve_TRD_detector_box[TRD_detector] ->SetName(HistName.Data());
        Int_t flag_QC = h_good_bad_TRD_chambers ->GetBinContent(TRD_detector+1);
        if(!flag_QC) // chamber is OK flagged by QA
        {
            vec_eve_TRD_detector_box[TRD_detector]->SetMainColor(kCyan);
            vec_eve_TRD_detector_box[TRD_detector]->SetMainTransparency(95); // the higher the value the more transparent
        }
        else // bad chamber
        {
            vec_eve_TRD_detector_box[TRD_detector]->SetMainColor(color_flag_QC[flag_QC]);
            vec_eve_TRD_detector_box[TRD_detector]->SetMainTransparency(85); // the higher the value the more transparent
        }

        for(Int_t i_vertex = 0; i_vertex < 8; i_vertex++)
        {
            Double_t arr_pos_glb[3] = {vec_TH1D_TRD_geometry[0][i_vertex]->GetBinContent(TRD_detector),vec_TH1D_TRD_geometry[1][i_vertex]->GetBinContent(TRD_detector),vec_TH1D_TRD_geometry[2][i_vertex]->GetBinContent(TRD_detector)};
            vec_eve_TRD_detector_box[TRD_detector]->SetVertex(i_vertex,arr_pos_glb[0],arr_pos_glb[1],arr_pos_glb[2]);

            if(i_stack == 0 && i_vertex < 4)
            {
                vec_PL_TRD_det_2D[i_sector][i_layer] ->SetPoint(i_vertex,arr_pos_glb[0],arr_pos_glb[1]);
                if(i_vertex == 0) vec_PL_TRD_det_2D[i_sector][i_layer] ->SetPoint(4,arr_pos_glb[0],arr_pos_glb[1]);
            }
        }

        gEve->AddElement(vec_eve_TRD_detector_box[TRD_detector]);
    }
    gEve->Redraw3D(kTRUE);
    //--------------------------



    //--------------------------
    // Open Ntuple
    TFile* inputfile = TFile::Open("./ST_out/Merge_ST_sec_ver_NT.root");
    TNtuple* NT_sec_vertices = (TNtuple*)inputfile->Get("NT_secondary_vertices");
    Float_t x_sec,y_sec,z_sec,ntracks_sec;
    NT_sec_vertices ->SetBranchAddress("x",&x_sec);
    NT_sec_vertices ->SetBranchAddress("y",&y_sec);
    NT_sec_vertices ->SetBranchAddress("z",&z_sec);
    NT_sec_vertices ->SetBranchAddress("ntracks",&ntracks_sec);
    //--------------------------


    TH2D* h2d_vertex_pos_xy = new TH2D("h2d_vertex_pos_xy","h2d_vertex_pos_xy",500,-400,400,500,-400,400);
    TEvePointSet* TEveP_offset_points = new TEvePointSet();


    //--------------------------
    // Loop over data
    Long64_t N_entries = NT_sec_vertices->GetEntries();
    for(Long64_t i_entry = 0; i_entry < N_entries; i_entry++)
        //for(Long64_t i_entry = 0; i_entry < 50; i_entry++)
    {
        if (i_entry != 0  &&  i_entry % 50 == 0)
            cout << "." << flush;
        if (i_entry != 0  &&  i_entry % 500 == 0)
        {
            printf("i_entry: %lld out of %lld, %4.2f%% total done \n",i_entry,N_entries,((Double_t)i_entry/(Double_t)N_entries)*100.0);
        }

        NT_sec_vertices ->GetEntry(i_entry);
        h2d_vertex_pos_xy ->Fill(x_sec,y_sec);
        TEveP_offset_points  ->SetPoint(i_entry,x_sec,y_sec,z_sec);

        //printf("i_entry: %d, pos: {%4.3f, %4.3f, %4.3f} \n",i_entry,x_sec,y_sec,z_sec);
    }
    //--------------------------



    //--------------------------
    gEve->AddElement(TEveP_offset_points);
    TEveP_offset_points  ->SetMarkerSize(1);
    TEveP_offset_points  ->SetMarkerStyle(20);
    TEveP_offset_points  ->SetMarkerColor(kBlue+1);
    gEve->Redraw3D(kTRUE);
    //--------------------------




    //--------------------------
    // Plot y vs x
    h2d_vertex_pos_xy ->GetXaxis()->SetTitle("x (cm)");
    h2d_vertex_pos_xy ->GetYaxis()->SetTitle("y (cm)");
    TCanvas* can_vertex_pos_xy = Draw_2D_histo_and_canvas(h2d_vertex_pos_xy,"can_vertex_pos_xy",1010,820,0.0,-1.0,"colz"); // TH2D* hist, TString name, Int_t x_size, Int_t y_size, Double_t min_val, Double_t max_val, TString option
    can_vertex_pos_xy->cd()->SetRightMargin(0.20);
    can_vertex_pos_xy->cd()->SetTopMargin(0.08);
    can_vertex_pos_xy->cd()->SetLogz(0);
    can_vertex_pos_xy->cd();

    for(Int_t i_sector = 0; i_sector < 18; i_sector++)
    {
        vec_PL_TRD_det_2D[i_sector].resize(6); // layers
        for(Int_t i_layer = 0; i_layer < 6; i_layer++)
        {
            vec_PL_TRD_det_2D[i_sector][i_layer] ->SetLineColor(kBlack);
            vec_PL_TRD_det_2D[i_sector][i_layer] ->SetLineWidth(1);
            vec_PL_TRD_det_2D[i_sector][i_layer] ->SetLineStyle(1);
            vec_PL_TRD_det_2D[i_sector][i_layer] ->Draw("l");
        }
    }
    //--------------------------




}