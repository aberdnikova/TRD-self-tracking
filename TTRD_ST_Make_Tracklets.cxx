
#include "TTRD_ST_Make_Tracklets.h"
//#include "TTRD_ST_Make_Tracklets_LinkDef.h"

ClassImp(TTRD_ST_Make_Tracklets);

//----------------------------------------------------------------------------------------
TTRD_ST_Make_Tracklets::TTRD_ST_Make_Tracklets(Int_t graphics)
{


    Init_QA();


    vec_TV3_Tracklet_pos.resize(540);
    vec_TV3_Tracklet_dir.resize(540);
    vec_h_diff_helix_line_impact_angle.resize(3); // [all,-,+]
    vec_h_diff_ref_loc_off_trkl.resize(3); // [all,-,+]
    for(Int_t i_charge = 0; i_charge < 3; i_charge++)
    {
        vec_h_diff_helix_line_impact_angle[i_charge].resize(547);
        vec_h_diff_ref_loc_off_trkl[i_charge].resize(547);
    }

    // Standard time bins
    vec_merge_time_bins.resize(24+1);
    for(Int_t i_time = 0; i_time < (24+1); i_time++)
    {
        vec_merge_time_bins[i_time] = i_time;
    }


    h_delta_angle_perp_impact = new TH1D("h_delta_angle_perp_impact","h_delta_angle_perp_impact",240,-30,30);
    h_detector_hit            = new TH1D("h_detector_hit","h_detector_hit",540,0,540);

    vec_layer_in_fit.resize(7);


    vec_digit_single_info.resize(14); // x,y,z,time,ADC,sector,stack,layer,row,column,dca,dca_x,dca_y,dca_z
    vec_track_single_info.resize(12); // dca,TPCdEdx,momentum,eta_track,pT_track,TOFsignal,Track_length,TRDsumADC,TRD_signal,nsigma_TPC_e,nsigma_TPC_pi,nsigma_TPC_p


    vec_tracklet_fit_points.resize(7); // layers 0-5, 6 = fit through first time cluster points of all layers
    for(Int_t i_layer = 0; i_layer < 7; i_layer++)
    {
        vec_tracklet_fit_points[i_layer].resize(2); // start, stop point
        for(Int_t i_start_stop = 0; i_start_stop < 2; i_start_stop++)
        {
            vec_tracklet_fit_points[i_layer][i_start_stop].resize(3); // x,y,z
        }
    }



    //---------------------------------------
    // Load TRD geometry, created by Create_TRD_geometry_files.cc
    TFile* file_TRD_geom = TFile::Open("./TRD_geometry_full.root");

    vec_TV3_TRD_center.resize(540);
    vec_TV3_TRD_center_offset.resize(540);
    for(Int_t i_det = 0; i_det < 540; i_det++)
    {
        vec_TV3_TRD_center[i_det].resize(3);
    }

    vector< vector<TH1D*> > vec_TH1D_TV3_TRD_center;
    vec_TH1D_TV3_TRD_center.resize(3); // x,y,z axes
    for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
    {
        vec_TH1D_TV3_TRD_center[i_xyz].resize(3); // vector direction
        for(Int_t i_dir_component = 0; i_dir_component < 3; i_dir_component++)
        {
            HistName = "vec_TH1D_TV3_TRD_center_";
            HistName += i_xyz;
            HistName += "_V";
            HistName += i_dir_component;
            vec_TH1D_TV3_TRD_center[i_xyz][i_dir_component] = (TH1D*)file_TRD_geom->Get(HistName.Data());

            for(Int_t i_det = 0; i_det < 540; i_det++)
            {
                Double_t value = vec_TH1D_TV3_TRD_center[i_xyz][i_dir_component] ->GetBinContent(i_det+1);
                vec_TV3_TRD_center[i_det][i_xyz][i_dir_component] = value;
                //printf("i_xyz: %d, i_dir_component: %d, i_det: %d, value: %4.3f \n",i_xyz,i_dir_component,i_det,value);
                //vec_TV3_TRD_center[i_det][i_xyz].Print();
            }
        }
    }

    vector<TH1D*> vec_TH1D_TV3_TRD_center_offset;
    vec_TH1D_TV3_TRD_center_offset.resize(3); // x,y,z axes
    for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
    {
        HistName = "vec_TH1D_TV3_TRD_center_offset_";
        HistName += i_xyz;
        vec_TH1D_TV3_TRD_center_offset[i_xyz] = (TH1D*)file_TRD_geom->Get(HistName.Data());

        for(Int_t i_det = 0; i_det < 540; i_det++)
        {
            vec_TV3_TRD_center_offset[i_det][i_xyz] = vec_TH1D_TV3_TRD_center_offset[i_xyz] ->GetBinContent(i_det+1);
        }
    }

    vector< vector<TH1D*> > vec_TH1D_TRD_geometry; // store for all 540 chambers the 8 corner vertices per detector
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
    //---------------------------------------


    //--------------------------
    // Open histogram which defines good and bad chambers
    //TFile* file_TRD_QA = TFile::Open("./Data/chamber_QC.root");
    //h_good_bad_TRD_chambers = (TH1D*)file_TRD_QA ->Get("all_defects_hist");

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
    //--------------------------


    vec_TV3_local_pos.resize(8);

    for(Int_t i_charge = 0; i_charge < 3; i_charge++)
    {
        for(Int_t TRD_detector = 0; TRD_detector < 547; TRD_detector++)
        {
            HistName = "vec_h_diff_helix_line_impact_angle_c_";
            HistName += i_charge;
            HistName += "_det_";
            HistName += TRD_detector;
            vec_h_diff_helix_line_impact_angle[i_charge][TRD_detector] = new TH1D(HistName.Data(),HistName.Data(),100,-10.0,10.0);

            HistName = "vec_h_diff_ref_loc_off_trkl_";
            HistName += i_charge;
            HistName += "_det_";
            HistName += TRD_detector;
            vec_h_diff_ref_loc_off_trkl[i_charge][TRD_detector] = new TH1D(HistName.Data(),HistName.Data(),400,-2.0,2.0);
        }
    }


    // OK

    vec_self_tracklet_points_matched.resize(540);
    vec_self_tracklet_fit_points_matched.resize(540);
    vec_self_tracklet_points_background.resize(540);
    vec_self_tracklet_fit_points_background.resize(540);
    trkl_min.resize(540);
    trkl_min_background.resize(540);
    vec_self_tracklet_points_bckg.resize(540);
    vec_self_tracklet_fit_points_bckg.resize(540);
    trkl_min_bckg.resize(540);

    vec_ADC_val.clear();
    vec_ADC_val.resize(540);
    radii_digits_initial = new TH1D("radii_digits_initial","radii_digits_initial",2000,0,2000);
    radii_tracklets_final = new TH1D("radii_tracklets_final","radii_tracklets_final",2000,0,2000);

    //TFile* file_TRD_geometry = TFile::Open("./TRD_Geometry.root");
    //TList* list_geom = (TList*)file_TRD_geometry->Get("TRD_Digits_output");
    //h2D_TRD_det_coordinates = (TH2D*)list_geom ->FindObject("h2D_TRD_det_coordinates");
    //h_layer_radii_det = new TH1D("h_layer_radii_det","h_layer_radii_det",540,0,540);

#if defined(USEEVE)
    if(graphics)
    {
        TEveP_TRD_det_origin = new TEvePointSet();
        TEveP_digits.resize(7); // ADC value -> color
        for(Int_t i_ADC = 0; i_ADC < 7; i_ADC++)
        {
            TEveP_digits[i_ADC] = new TEvePointSet();
        }
        TEveP_digits_flagged = new TEvePointSet();
        TEveLine_fitted_tracklets.resize(6); // layers
    }
#endif

    /*
    for(Int_t i_det = 0; i_det < 540; i_det++)
    {
        Double_t x_val = h2D_TRD_det_coordinates ->GetBinContent(i_det+1,1);
        Double_t y_val = h2D_TRD_det_coordinates ->GetBinContent(i_det+1,2);
        Double_t z_val = h2D_TRD_det_coordinates ->GetBinContent(i_det+1,3);

        printf("pos: {%4.3f, %4.3f, %4.3f}, posB: {%4.3f, %4.3f, %4.3f} \n",x_val,y_val,z_val,vec_TV3_TRD_center_offset[i_det][0],vec_TV3_TRD_center_offset[i_det][1],vec_TV3_TRD_center_offset[i_det][2]);

        Double_t radius_align = TMath::Sqrt(x_val*x_val + y_val*y_val);
        h_layer_radii_det ->SetBinContent(i_det+1,radius_align);

        TEveP_TRD_det_origin ->SetPoint(i_det,x_val,y_val,z_val);
    }
    */

    // constructor
#if defined(USEEVE)
    if(graphics)
    {
        TEveP_primary_vertex    = new TEvePointSet();
        TEveP_TRD_det_origin    = new TEvePointSet();
        TEve_clusters           = new TEvePointSet();
        TEve_connected_clusters = new TEvePointSet();


        TEveManager::Create();


        TPL3D_helix = new TEveLine();
        TEveLine_beam_axis = new TEveLine();
        TEveLine_beam_axis ->SetNextPoint(0.0,0.0,-300.0);
        TEveLine_beam_axis ->SetNextPoint(0.0,0.0,300.0);
        TEveLine_beam_axis ->SetName("beam axis");
        TEveLine_beam_axis ->SetLineStyle(1);
        TEveLine_beam_axis ->SetLineWidth(4);
        TEveLine_beam_axis ->SetMainColor(kBlue);
        gEve->AddElement(TEveLine_beam_axis);

        vec_TEveLine_tracklets.resize(6); // layers
        vec_TEveLine_tracklets_match.resize(6); // layers

        TEveP_TRD_det_origin ->SetMarkerSize(2);
        TEveP_TRD_det_origin ->SetMarkerStyle(20);
        TEveP_TRD_det_origin ->SetMainColor(kMagenta+1);
        //gEve->AddElement(TEveP_TRD_det_origin);
    }
#endif


#if defined(USEEVE)
    if(graphics)
    {
        vec_eve_TRD_detector_box.resize(540);
    }
#endif
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
    //= {kCyan,kPink,kMagenta,kMagenta+2,kOrange,kOrange+2,kRed,kRed+2};

#if defined(USEEVE)
    if(graphics)
    {
        for(Int_t TRD_detector = 0; TRD_detector < 540; TRD_detector++)
        {
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
            }

            gEve->AddElement(vec_eve_TRD_detector_box[TRD_detector]);
        }

        gEve->Redraw3D(kTRUE);
    }
#endif
    //--------------------------


}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
TTRD_ST_Make_Tracklets::~TTRD_ST_Make_Tracklets()
{

}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void TTRD_ST_Make_Tracklets::Init_QA()
{
    printf("TTRD_ST_Make_Tracklets::Init_QA() \n");
    //TFile* inputfile_QA = TFile::Open("/home/ceres/schmah/ALICE/TRD_Run3_calib/QA_out_year_2016_V2.root");
    //tg_HV_drift_vs_det = (TGraph*)inputfile_QA->Get("tg_HV_drift_vs_det");
    //tg_HV_anode_vs_det = (TGraph*)inputfile_QA->Get("tg_HV_anode_vs_det");;
    //tg_vdrift_vs_det   = (TGraph*)inputfile_QA->Get("tg_vdrift_vs_det");;
}
//----------------------------------------------------------------------------------------




//----------------------------------------------------------------------------------------
// Driver function to sort the 2D vector 
// on basis of a particular column 
bool sortcol_first( const vector<Double_t>& v1,
             const vector<Double_t>& v2 )
{
 return v1[3] > v2[3];  // second column
} 
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
Double_t TTRD_ST_Make_Tracklets::calculateMinimumDistanceStraightToPoint(TVector3 &base, TVector3 &dir,
									 TVector3 &point)
{
  // calculates the minimum distance of a point to a straight given as parametric straight x = base + n * dir

  if (!(dir.Mag()>0))
    {
      return -1000000.;
    }

    #if 0 //try 2D
base.SetXYZ(base.X(),base.Y(),0.0);
point.SetXYZ(point.X(),point.Y(),0.0);

    #endif
  
  TVector3 diff = base-point;

  TVector3 cross = dir.Cross(diff);
  
  return cross.Mag()/dir.Mag();
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
TVector3 TTRD_ST_Make_Tracklets::calculateDCA_vec_StraightToPoint(TVector3 &base, TVector3 &dir, TVector3 &point)
{
  // calculates the minimum distance vector of a point to a straight given as parametric straight x = base + n * dir

    TVector3 diff = base-point;
    TVector3 dir_norm = dir;
    dir_norm *= (1.0/dir.Mag());
    Double_t proj_val = diff.Dot(dir_norm);
    TVector3 proj_dir = dir_norm;
    proj_dir *= proj_val;

    TVector3 dist_vec = proj_dir - diff;

    return dist_vec;
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
TVector3 TTRD_ST_Make_Tracklets::calculate_point_on_Straight_dca_to_Point(TVector3 &base, TVector3 &dir, TVector3 &point)
{
  // calculates the TVector3 on the straight line which is closest to point

    TVector3 diff = point - base;
    TVector3 dir_norm = dir;
    dir_norm *= (1.0/dir.Mag());
    Double_t proj_val = diff.Dot(dir_norm);
    TVector3 proj_dir = dir_norm;
    proj_dir *= proj_val;

    TVector3 dist_vec = proj_dir + base;

    return dist_vec;
}
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
TVector3 TTRD_ST_Make_Tracklets::calculate_point_on_Straight_dca_to_Point_2D(TVector3 &base, TVector3 &dir, TVector3 &point)
{
  // calculates the TVector3 on the straight line which is closest to point

    
    TVector3 point2d;
    TVector3 base2d;
    TVector3 dir2d;

    point2d.SetXYZ(point[0],point[1],0.0);
    base2d.SetXYZ(base[0],base[1],0.0);
    dir2d.SetXYZ(dir[0],dir[1],0.0);

    TVector3 diff = point2d - base2d;
    TVector3 dir_norm = dir2d;
    dir_norm *= (1.0/dir2d.Mag());
    Double_t proj_val = diff.Dot(dir_norm);
    TVector3 proj_dir = dir_norm;
    proj_dir *= proj_val;

    TVector3 dist_vec = proj_dir + base2d;

    return dist_vec;
}
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
void TTRD_ST_Make_Tracklets::Reset()
{
    printf("TTRD_ST_Make_Tracklets::Reset() \n");
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void TTRD_ST_Make_Tracklets::Init_tree(TString SEList)
{
    cout << "Initialize tree" << endl;
    //TString pinputdir = "/misc/alidata120/alice_u/schmah/TRD_offline_calib/Data/";
    //TString inlistdir = "/home/ceres/schmah/ALICE/TRD_self_tracking/Lists/";
    //TString pinputdir = "/misc/alidata120/alice_u/schmah/TRD_self_tracking/Data/";
    //TString pinputdir = "/home/ceres/berdnikova/TRD-Run3-Calibration/";
    //TString inlistdir = "/home/ceres/hoppner/ALICE/TRD_self_tracking/TRD-self-tracking/";
    //TString pinputdir = "/home/ceres/hoppner/ALICE/TRD_self_tracking/TRD-self-tracking/Data/";

    TString inlistdir = "./Lists/";
    TString pinputdir = "./Data/";
    
    TString in_list_name = SEList;
    SEList = inlistdir + SEList;

    AS_Event = new Ali_AS_Event();
    AS_Track = new Ali_AS_Track();
    AS_Tracklet = new Ali_AS_Tracklet();
    AS_Digit = new Ali_AS_TRD_digit();

    // Same event input
    if (!SEList.IsNull())   // if input file is ok
    {
        cout << "Open same event file list " << SEList << endl;
        ifstream in(SEList);  // input stream
        if(in)
        {
            cout << "file list is ok" << endl;
            input_SE  = new TChain( JPsi_TREE.Data(), JPsi_TREE.Data() );
            char str[255];       // char array for each file name
            Long64_t entries_save = 0;
            while(in)
            {
                in.getline(str,255);  // take the lines of the file list
                if(str[0] != 0)
                {
                    TString addfile;
                    addfile = str;
                    addfile = pinputdir+addfile;
                    input_SE ->AddFile(addfile.Data(),-1, JPsi_TREE.Data() );
                    Long64_t file_entries = input_SE->GetEntries();
                    cout << "File added to data chain: " << addfile.Data() << " with " << (file_entries-entries_save) << " entries" << endl;
                    entries_save = file_entries;
                }
            }
            input_SE  ->SetBranchAddress( JPsi_BRANCH, &AS_Event );
        }
        else
        {
            cout << "WARNING: SE file input is problemtic" << endl;
        }
    }

    file_entries_total = input_SE->GetEntries();
    N_Events = file_entries_total;
    cout << "Total number of events in tree: " << file_entries_total << endl;


    //------------------------------------------------
    printf("Create output file \n");
    //TString outfile_name = "/misc/alidata120/alice_u/schmah/TRD_self_tracking/Calib_tracklets/" + in_list_name + "_out_V2.root";
    TString outfile_name = "./Data/" + in_list_name + "_out_V2.root";
    //TString outfile_name = "/home/ceres/hoppner/ALICE/TRD_self_tracking/TRD-self-tracking/Calib_tracklets/" + in_list_name + "_out_V2.root";
    //outputfile = new TFile("./Data/TRD_Calib_ADC_X1.root","RECREATE");
    outputfile = new TFile(outfile_name.Data(),"RECREATE");
    outputfile ->cd();
    // TRD self tracking output data containers
    TRD_ST_Tracklet   = new Ali_TRD_ST_Tracklets();
    TRD_ST_TPC_Track  = new Ali_TRD_ST_TPC_Track();
    TRD_ST_Event      = new Ali_TRD_ST_Event();

    Tree_TRD_ST_Event  = NULL;
    Tree_TRD_ST_Event  = new TTree("Tree_TRD_ST_Event" , "TRD_ST_Events" );
    Tree_TRD_ST_Event  ->Branch("Tree_TRD_ST_Event_branch"  , "TRD_ST_Event", TRD_ST_Event );
    Tree_TRD_ST_Event  ->SetAutoSave( 5000000 );
    //------------------------------------------------
}
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
Int_t TTRD_ST_Make_Tracklets::Loop_event(Long64_t event)
{
    printf("Loop event number: %lld \n",event);

    Event_active = event;

    if (!input_SE->GetEntry( event )) return 0; // take the event -> information is stored in event

    N_Digits = 0;


    //---------------------------------------------------------------------------
    UShort_t NumTracks            = AS_Event ->getNumTracks(); // number of tracks in this event
    NumTracks_event = NumTracks;
    Double_t EventVertexX         = AS_Event ->getx();
    Double_t EventVertexY         = AS_Event ->gety();
    Double_t EventVertexZ         = AS_Event ->getz();
    Int_t    N_tracks_event       = AS_Event ->getN_tracks();
    Int_t    N_TRD_tracklets      = AS_Event ->getN_TRD_tracklets();
    Int_t    N_TRD_tracklets_online = AS_Event ->getNumTracklets(); // online tracklet
    Float_t  V0MEq                = AS_Event ->getcent_class_V0MEq();

    printf("N_TRD_tracklets_online: %d \n",N_TRD_tracklets_online);

    N_Tracks = NumTracks;

    vec_track_info.clear();
    vec_digit_track_info.clear();
    for(Int_t i_det = 0; i_det < 540; i_det++)
    {
        vec_TV3_Tracklet_pos.clear();
        vec_TV3_Tracklet_dir.clear();
    }


    // Loop over all tracklets
    Double_t scale_factor_length = 2.0;
    for(UShort_t i_tracklet = 0; i_tracklet < N_TRD_tracklets_online; ++i_tracklet) // loop over all tracklets of the actual event
    {

        AS_Tracklet             = AS_Event    ->getTracklet( i_tracklet ); // take the track
        TVector3 TV3_offset     = AS_Tracklet ->get_TV3_offset(); // online tracklets
        //TV3_offset.Print();
        TVector3 TV3_dir        = AS_Tracklet ->get_TV3_dir();    // online tracklets
        //TV3_dir.Print();
        Short_t  i_det_tracklet = AS_Tracklet ->get_detector();

        vec_TV3_Tracklet_pos[i_det_tracklet].push_back(TV3_offset);
        vec_TV3_Tracklet_dir[i_det_tracklet].push_back(TV3_dir);
        //vec_TV3_TRD_center[i_det_tracklet][2].Print();

        Double_t impact_angle = TV3_dir.Angle(vec_TV3_TRD_center[i_det_tracklet][2]);
        if(impact_angle > TMath::Pi()*0.5) impact_angle -= TMath::Pi();

        Float_t points[6] =
        {
            (Float_t)(TV3_offset[0]),(Float_t)(TV3_offset[1]),(Float_t)(TV3_offset[2]),
            (Float_t)(TV3_offset[0] + scale_factor_length*TV3_dir[0]),(Float_t)(TV3_offset[1] + scale_factor_length*TV3_dir[1]),(Float_t)(TV3_offset[2] + scale_factor_length*TV3_dir[2])
        };
        //printf("i_tracklet: %d, out of %d, impact_angle: %4.3f, offset: {%4.3f, %4.3f, %4.3f}, end: {%4.3f, %4.3f, %4.3f} \n",i_tracklet,N_TRD_tracklets_online,impact_angle*TMath::RadToDeg(),TV3_offset[0],TV3_offset[1],TV3_offset[2],TV3_offset[0] + TV3_dir[0],TV3_offset[1] + TV3_dir[1],TV3_offset[2] + TV3_dir[2]);
    }


    // Loop over all tracks
    for(UShort_t i_track = 0; i_track < NumTracks; ++i_track) // loop over all tracks of the actual event
    {
        //cout << "i_track: " << i_track << ", of " << NumTracks << endl;
        AS_Track      = AS_Event ->getTrack( i_track ); // take the track
        Double_t nsigma_TPC_e   = AS_Track ->getnsigma_e_TPC();
        Double_t nsigma_TPC_pi  = AS_Track ->getnsigma_pi_TPC();
        Double_t nsigma_TPC_p   = AS_Track ->getnsigma_p_TPC();
        Double_t nsigma_TOF_e   = AS_Track ->getnsigma_e_TOF();
        Double_t nsigma_TOF_pi  = AS_Track ->getnsigma_pi_TOF();
        Double_t TRD_signal     = AS_Track ->getTRDSignal();
        Double_t TRDsumADC      = AS_Track ->getTRDsumADC();
        Double_t dca            = AS_Track ->getdca();  // charge * distance of closest approach to the primary vertex
        TLorentzVector TLV_part = AS_Track ->get_TLV_part();
        UShort_t NTPCcls        = AS_Track ->getNTPCcls();
        UShort_t NTRDcls        = AS_Track ->getNTRDcls();
        UShort_t NITScls        = AS_Track ->getNITScls();
        Float_t TPCchi2         = AS_Track ->getTPCchi2();
        Float_t TPCdEdx         = AS_Track ->getTPCdEdx();
        Float_t TOFsignal       = AS_Track ->getTOFsignal(); // in ps (1E-12 s)
        Float_t Track_length    = AS_Track ->getTrack_length();

        Float_t momentum        = TLV_part.P();
        Float_t eta_track       = TLV_part.Eta();
        Float_t pT_track        = TLV_part.Pt();
        Float_t theta_track     = TLV_part.Theta();
        Float_t phi_track       = TLV_part.Phi();

        vec_track_single_info[0]  = dca;
        vec_track_single_info[1]  = TPCdEdx;
        vec_track_single_info[2]  = momentum;
        vec_track_single_info[3]  = eta_track;
        vec_track_single_info[4]  = pT_track;
        vec_track_single_info[5]  = TOFsignal;
        vec_track_single_info[6]  = Track_length;
        vec_track_single_info[7]  = TRDsumADC;
        vec_track_single_info[8]  = TRD_signal;
        vec_track_single_info[9]  = nsigma_TPC_e;
        vec_track_single_info[10] = nsigma_TPC_pi;
        vec_track_single_info[11] = nsigma_TPC_p;
        vec_track_info.push_back(vec_track_single_info);

        //----------------------------------------------
        // TRD digit information
        UShort_t  fNumTRDdigits        = AS_Track ->getNumTRD_digits();
        UShort_t  fNumOfflineTracklets = AS_Track ->getNumOfflineTracklets();
        //--------------------------


        //--------------------------
        // Offline tracklet loop
        for(Int_t i_tracklet = 0; i_tracklet < fNumOfflineTracklets; i_tracklet++) // layers
        {
            AS_offline_Tracklet     = AS_Track            ->getOfflineTracklet( i_tracklet ); // take the track
            TVector3 TV3_offset     = AS_offline_Tracklet ->get_TV3_offset(); // offline tracklets
            TVector3 TV3_dir        = AS_offline_Tracklet ->get_TV3_dir();    // offline tracklets
            Short_t  i_det_tracklet = AS_offline_Tracklet ->get_detector();

            //printf("offline, i_tracklet: %d, offset: {%4.3f, %4.3f, %4.3f}, dir: {%4.3f, %4.3f, %4.3f} \n",i_tracklet,(Float_t)(TV3_offset[0]),(Float_t)(TV3_offset[1]),(Float_t)(TV3_offset[2]),(Float_t)TV3_dir[0],(Float_t)TV3_dir[1],(Float_t)TV3_dir[2]);
        }
        //--------------------------


        //printf("i_track: %d, pT: %4.3f, phi: %4.3f, eta: %4.3f, N_off_trkl: %d \n",i_track,pT_track,phi_track,eta_track,fNumOfflineTracklets);

        //printf("i_track: %d, fNumTRDdigits: %d \n",i_track,fNumTRDdigits);

        vec_digit_info.clear();
        for(UShort_t i_digits = 0; i_digits < fNumTRDdigits; i_digits++)
        {
            //cout << "i_digits: " << i_digits << ", of " << fNumTRDdigits << endl;
            AS_Digit              = AS_Track ->getTRD_digit(i_digits);
            Int_t    layer        = AS_Digit ->get_layer();
            Int_t    sector       = AS_Digit ->get_sector();
            Int_t    column       = AS_Digit ->get_column();
            Int_t    stack        = AS_Digit ->get_stack();
            Int_t    row          = AS_Digit ->get_row();
            Int_t    detector     = AS_Digit ->get_detector(layer,stack,sector);
            Float_t  dca_to_track = AS_Digit ->getdca_to_track();
            Float_t  dca_x        = AS_Digit ->getdca_x();
            Float_t  dca_y        = AS_Digit ->getdca_y();
            Float_t  dca_z        = AS_Digit ->getdca_z();
            Float_t  ImpactAngle  = AS_Digit ->getImpactAngle();

            for(Int_t i_time = 0; i_time < 24; i_time++)
            {
                for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                {
                    digit_pos[i_xyz] = AS_Digit ->get_pos(i_time,i_xyz);
                }
                //printf("track: %d/%d, digit: %d/%d \n",i_track,NumTracks,i_digits,fNumTRDdigits);
                //printf("pos: {%4.3f, %4.3f, %4.3f} \n,",digit_pos[0],digit_pos[1],digit_pos[2]);

                Float_t ADC = (Float_t)AS_Digit ->getADC_time_value(i_time);


                // x,y,z,time,ADC,sector,stack,layer,row,column,dca
                vec_digit_single_info[0]  = digit_pos[0];
                vec_digit_single_info[1]  = digit_pos[1];
                vec_digit_single_info[2]  = digit_pos[2];
                vec_digit_single_info[3]  = i_time;
                vec_digit_single_info[4]  = ADC;
                vec_digit_single_info[5]  = sector;
                vec_digit_single_info[6]  = stack;
                vec_digit_single_info[7]  = layer;
                vec_digit_single_info[8]  = row;
                vec_digit_single_info[9]  = column;
                vec_digit_single_info[10] = dca_to_track;
                vec_digit_single_info[11] = dca_x;
                vec_digit_single_info[12] = dca_y;
                vec_digit_single_info[13] = dca_z;

                //printf("dca_full: %4.3f, dca: {%4.3f, %4.3f, %4.3f} \n",dca_to_track,dca_x,dca_y,dca_z);
                vec_digit_info.push_back(vec_digit_single_info);

                N_Digits ++;
            }

        } // end of digits loop

        vec_digit_track_info.push_back(vec_digit_info);

    } // end of track loop


    return 1;
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
//void TTRD_ST_Make_Tracklets::Calc_SVD_tracklet(Int_t i_det, Int_t i_trkl)
Double_t TTRD_ST_Make_Tracklets::Calc_SVD_tracklet(Int_t i_det, Int_t i_trkl)
{
    // https://www.codefull.net/2015/06/3d-line-fitting/ -> MATLAB code

    TVector3 TV3_point;
    TVector3 TV3_mean(0.0,0.0,0.0);
    vector<TVector3> arr_TV3_points;
    vector<TVector3> arr_TV3_points_mean;

    Double_t SVD_chi2 = 0.0;  //chi2 of the tracklet

    Int_t number_ok_clusters = 0;
    for(Int_t i_time = 0; i_time < (Int_t)vec_connected_clusters[i_det][i_trkl].size(); i_time++)
    //for(Int_t i_time = 0; i_time < (Int_t)vec_self_tracklet_points[i_det][i_trkl].size(); i_time++)
    {
        //if(vec_self_tracklet_points[i_det][i_trkl][i_time][0] == -999.0 && vec_self_tracklet_points[i_det][i_trkl][i_time][1] == -999.0 && vec_self_tracklet_points[i_det][i_trkl][i_time][2] == -999.0) continue;
        //else
        {
            //TV3_point.SetXYZ(vec_self_tracklet_points[i_det][i_trkl][i_time][0],vec_self_tracklet_points[i_det][i_trkl][i_time][1],vec_self_tracklet_points[i_det][i_trkl][i_time][2]);
            TV3_point.SetXYZ(vec_connected_clusters[i_det][i_trkl][i_time][0],vec_connected_clusters[i_det][i_trkl][i_time][1],vec_connected_clusters[i_det][i_trkl][i_time][2]);
            TV3_mean += TV3_point;
            arr_TV3_points.push_back(TV3_point);
            number_ok_clusters++;

            //if(i_det == 244 && i_trkl == 4)
            //{
            //    printf("pos: {%4.3f, %4.3f, %4.3f} \n",vec_self_tracklet_points[i_det][i_trkl][i_time][0],vec_self_tracklet_points[i_det][i_trkl][i_time][1],vec_self_tracklet_points[i_det][i_trkl][i_time][2]);
            //}
        }
    }

    //printf("number_ok_clusters: %d \n",number_ok_clusters);
    TArrayD arr_data_points(number_ok_clusters*3);

    // Calculate mean and subtract it
    if(number_ok_clusters > 0) TV3_mean *= 1.0/((Double_t)number_ok_clusters);

    //if(i_det == 244 && i_trkl == 4)
    //{
    //    printf(" --> number_ok_clusters: %d, mean vec: {%4.3f, %4.3f, %4.3f} \n",number_ok_clusters,TV3_mean.X(),TV3_mean.Y(),TV3_mean.Z());
    //}

    for(Int_t i_point = 0; i_point < number_ok_clusters; i_point++)
    {
        arr_TV3_points_mean.push_back(arr_TV3_points[i_point] - TV3_mean);
        for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
        {
            Int_t i_data = i_xyz + 3*i_point;
            arr_data_points[i_data] = arr_TV3_points_mean[i_point][i_xyz];
        }
    }

    TMatrixD A_Matrix(number_ok_clusters,3);
    A_Matrix.SetMatrixArray(arr_data_points.GetArray());
    TDecompSVD tSVD(A_Matrix);
    tSVD.Decompose();
    TMatrixD V_Matrix = tSVD.GetV();

    TVector3 TV3_fit_dir(V_Matrix[0][0],V_Matrix[1][0],V_Matrix[2][0]);
    TV3_fit_dir *= 1.0/TV3_fit_dir.Mag();

    TV3_SVD_tracklet_offset = TV3_mean;
    TV3_SVD_tracklet_dir    = TV3_fit_dir;

    for(Int_t i_point = 0; i_point < number_ok_clusters; i_point++)
    {
        //Double_t dist = calculateMinimumDistanceStraightToPoint(TV3_SVD_tracklet_offset, TV3_SVD_tracklet_dir,arr_TV3_points[i_point]);
        //SVD_chi2 += abs(dist);

        TVector3 testpoint = calculate_point_on_Straight_dca_to_Point_2D(TV3_SVD_tracklet_offset, TV3_SVD_tracklet_dir, arr_TV3_points[i_point]);

        Double_t dist_DCA_XY = TMath::Sqrt(TMath::Power(arr_TV3_points[i_point][0] - testpoint[0],2) + TMath::Power(arr_TV3_points[i_point][1] - testpoint[1],2));
        //Double_t dist_DCA_Z  = fabs(arr_TV3_points[i_point][2] - testpoint[2]);
        Double_t dist_weighted = TMath::Sqrt(TMath::Power(dist_DCA_XY/0.7,2.0));// + TMath::Power(dist_DCA_Z/7.5,2.0));
        SVD_chi2 += abs(dist_weighted);

#if 0
        if (i_det == 185 && (i_trkl == 1 || i_trkl == 4))
        {
            printf("datapoint: {%4.3f, %4.3f, %4.3f}, testpoint: {%4.3f, %4.3f, %4.3f} \n",arr_TV3_points[i_point][0],arr_TV3_points[i_point][1],arr_TV3_points[i_point][2],testpoint[0],testpoint[1],testpoint[2]);
            printf("TV3_SVD_tracklet_offset: {%4.3f, %4.3f, %4.3f}, TV3_SVD_tracklet_dir: {%4.3f, %4.3f, %4.3f} \n",TV3_SVD_tracklet_offset[0],TV3_SVD_tracklet_offset[1],TV3_SVD_tracklet_offset[2],TV3_SVD_tracklet_dir[0],TV3_SVD_tracklet_dir[1],TV3_SVD_tracklet_dir[2]);
            //printf("offset point: {%4.3f, %4.3f, %4.3f}");
            printf("dist_DCA_XY: %4.3f, dist_DCA_Z: %4.3f, dist_weighted: %4.3f, dist: %4.3f \n",dist_DCA_XY,dist_DCA_Z,dist_weighted,dist);
            printf("SVD_chi2: %4.3f \n",SVD_chi2);
        }

        testpoint.Delete();
#endif
    }

    SVD_chi2 = SVD_chi2/number_ok_clusters;
    return SVD_chi2;
    //printf("SVD_chi2 = %4.3f \n",SVD_chi2);

}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void TTRD_ST_Make_Tracklets::Make_clusters_and_get_tracklets_fit(Double_t Delta_x, Double_t Delta_z, Double_t factor_missing, Int_t graphics)
{
    //printf("TTRD_ST_Make_Tracklets::Make_clusters_and_get_tracklets_fit() \n");

    //Reset();

    vector< vector< vector< vector<Double_t> > > > vec_all_TRD_digits;
    vector< vector< vector< vector<Double_t> > > > vec_all_TRD_digits_clusters;
    vector< vector< vector<Int_t> > >              vec_used_clusters;
    //vector< vector< vector< vector<Double_t> > > > vec_self_tracklet_points;


    vec_all_TRD_digits.resize(540);
    vec_all_TRD_digits_clusters.resize(540);
    vec_used_clusters.resize(540);

    for(Int_t i_det = 0; i_det < 540; i_det++)
    {
        vec_all_TRD_digits[i_det].resize(24); // time bins
        vec_all_TRD_digits_clusters[i_det].resize(24); // time bins
    	vec_used_clusters[i_det].resize(24); // time bins
    }
    vector<Double_t> vec_digit_data; // x,y,z,ADC
    vector<Double_t> vec_digit_cluster_data; // x,y,z,ADC,N_digits
    vec_digit_data.resize(4);
    vec_digit_cluster_data.resize(5);

    // Fill all the information in the hierachy of detectors and time bins
    Int_t    N_TRD_digits  = AS_Event ->getNumTRD_digits();
    printf(" --> N_TRD_digits: %d \n",N_TRD_digits);
    for(Int_t i_digit = 0; i_digit < N_TRD_digits; i_digit++)
    {
        AS_Digit              = AS_Event ->getTRD_digit(i_digit);
        Int_t    layer        = AS_Digit ->get_layer();
        Int_t    sector       = AS_Digit ->get_sector();
        Int_t    column       = AS_Digit ->get_column();
        Int_t    stack        = AS_Digit ->get_stack();
        Int_t    row          = AS_Digit ->get_row();
        Int_t    detector     = AS_Digit ->get_detector(layer,stack,sector);

        Double_t radius_prev = 0.0;
        for(Int_t i_time = 0; i_time < 24; i_time++)
        {
            Float_t ADC = (Float_t)AS_Digit ->getADC_time_value(i_time);
            if(ADC < 0.0) continue;
            for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
            {
                digit_pos[i_xyz] = AS_Digit ->get_pos(i_time,i_xyz);
                vec_digit_data[i_xyz] = digit_pos[i_xyz];
            }
            vec_digit_data[3] = ADC;

#if defined(USEEVE)
            if(graphics)
            {
                for(Int_t i_ADC = 0; i_ADC < 7; i_ADC++)
                {
                    if(ADC > arr_ADC_color_digits[i_ADC] && ADC <= arr_ADC_color_digits[i_ADC+1])
                    {
                        TEveP_digits[i_ADC] ->SetNextPoint(digit_pos[0],digit_pos[1],digit_pos[2]);
                        break;
                    }
                }
            }
#endif


            vec_all_TRD_digits[detector][i_time].push_back(vec_digit_data);

            Double_t radius = TMath::Sqrt(TMath::Power(digit_pos[0] ,2) + TMath::Power(digit_pos[1] ,2));
            //if(i_time > 0 && radius_prev < radius) printf("det: %d, layer: %d, sector: %d, stack: %d, i_time: %d, radius: %4.3f \n",detector,layer,sector,stack,i_time,radius);
            radius_prev = radius;

        }
    }



    //-------------------------------------------------------
    // Make clusters for each detector and time bin
    // Individial digits -> clusters/time bin
    for(Int_t i_det = 0; i_det < 540; i_det++)
    {
        Int_t sector = (Int_t)(i_det/30);
        Int_t stack  = (Int_t)(i_det%30/6);
        Int_t layer  = i_det%6;
        //Int_t i_det = layer + 6*stack + 30*sector;

        //printf("i_det: %d \n",i_det);
        for(Int_t i_time = 0; i_time < 24; i_time++)
        {
            //if(!(i_det == 0 && i_time == 0)) continue;
            //for(Int_t i_digit = 0; i_digit < (Int_t)vec_all_TRD_digits[i_det][i_time].size(); i_digit++)
            //{
            //    printf("i_digit: %d, pos: {%4.3f, %4.3f, %4.3f}, ADC: %4.3f \n",i_digit,vec_all_TRD_digits[i_det][i_time][i_digit][0],vec_all_TRD_digits[i_det][i_time][i_digit][1],vec_all_TRD_digits[i_det][i_time][i_digit][2],vec_all_TRD_digits[i_det][i_time][i_digit][3]);
            //}

            //cout << "Sorting vector" << endl;
            // First order the vector from high to low ADC values

            // vec_all_TRD_digits // 540 chambers, 24 time bins, (x,y,z,ADC)
            std::sort(vec_all_TRD_digits[i_det][i_time].begin(),vec_all_TRD_digits[i_det][i_time].end(),sortcol_first); // large values to small values, last column sorted via function sortcol

            //for(Int_t i_digit = 0; i_digit < (Int_t)vec_all_TRD_digits[i_det][i_time].size(); i_digit++)
            //{
            //    printf("i_digit: %d, pos: {%4.3f, %4.3f, %4.3f}, ADC: %4.3f \n",i_digit,vec_all_TRD_digits[i_det][i_time][i_digit][0],vec_all_TRD_digits[i_det][i_time][i_digit][1],vec_all_TRD_digits[i_det][i_time][i_digit][2],vec_all_TRD_digits[i_det][i_time][i_digit][3]);
            //}


            vector<Int_t> arr_used_digits;
            arr_used_digits.clear();
            arr_used_digits.resize((Int_t)vec_all_TRD_digits[i_det][i_time].size());
            for(Int_t i_digit_max = 0; i_digit_max < (Int_t)vec_all_TRD_digits[i_det][i_time].size(); i_digit_max++)
            {
                arr_used_digits[i_digit_max] = 0;
            }


            //-------------------------------
            // Pre cleaning procedure -> flag digits which are alone, no other digits around in same time window

            // Start from the maximum ADC value(s)
            for(Int_t i_digit_max = 0; i_digit_max < ((Int_t)vec_all_TRD_digits[i_det][i_time].size()); i_digit_max++)
            {
                //if(arr_used_digits[i_digit_max]) continue;
                //arr_used_digits[i_digit_max] = 1;

                Double_t pos_ADC_max[4] = {vec_all_TRD_digits[i_det][i_time][i_digit_max][0],vec_all_TRD_digits[i_det][i_time][i_digit_max][1],vec_all_TRD_digits[i_det][i_time][i_digit_max][2],vec_all_TRD_digits[i_det][i_time][i_digit_max][3]};
                Int_t N_digits_added = 1;

                // Get all other digits within a certain radius
                for(Int_t i_digit_sub = 0; i_digit_sub < (Int_t)vec_all_TRD_digits[i_det][i_time].size(); i_digit_sub++)
                {
                    if(i_digit_sub == i_digit_max) continue;
                    //if(arr_used_digits[i_digit_sub]) continue;

                    Double_t pos_ADC_sub[4] = {vec_all_TRD_digits[i_det][i_time][i_digit_sub][0],vec_all_TRD_digits[i_det][i_time][i_digit_sub][1],vec_all_TRD_digits[i_det][i_time][i_digit_sub][2],vec_all_TRD_digits[i_det][i_time][i_digit_sub][3]};
                    Double_t dist_digits_XY = TMath::Sqrt(TMath::Power(pos_ADC_max[0] - pos_ADC_sub[0],2) + TMath::Power(pos_ADC_max[1] - pos_ADC_sub[1],2));
                    Double_t dist_digits_Z  = fabs(pos_ADC_max[2] - pos_ADC_sub[2]);
                    if(dist_digits_XY < 1.0 && dist_digits_Z  < 5.0)
                    {
                        N_digits_added++;
                        break;
                    }

                }

                if(N_digits_added == 1) arr_used_digits[i_digit_max] = 1;

                //if(i_det == 390 && i_time == 0 && pos_ADC_max[0] < -44.0)  printf("i_digit_max: %d, pos: {%4.3f, %4.3f, %4.3f} \n",i_digit_max,pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);

#if defined(USEEVE)
                if(graphics)
                {
                    if(N_digits_added == 1)
                    {
                        //if(i_det == 390 && i_time == 0)
                        {
                            TEveP_digits_flagged ->SetNextPoint(pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);
                            //printf("  --> flagged, i_digit_max: %d, pos: {%4.3f, %4.3f, %4.3f} \n",i_digit_max,pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);
                        }
                    }
                }
#endif
            } // end of digit max loop
            //-------------------------------


            // Start from the maximum ADC value(s)
            Double_t baseline = 10.0;
            for(Int_t i_digit_max = 0; i_digit_max < ((Int_t)vec_all_TRD_digits[i_det][i_time].size() - 1); i_digit_max++)
            {
                if(arr_used_digits[i_digit_max]) continue;
                arr_used_digits[i_digit_max] = 1;

                Double_t pos_ADC_max[4] = {vec_all_TRD_digits[i_det][i_time][i_digit_max][0],vec_all_TRD_digits[i_det][i_time][i_digit_max][1],vec_all_TRD_digits[i_det][i_time][i_digit_max][2],vec_all_TRD_digits[i_det][i_time][i_digit_max][3] - baseline};
                Int_t N_digits_added = 1;
                Double_t Sum_ADC_weight = pos_ADC_max[3]; // ADC as weight
                if(Sum_ADC_weight < 0.0) continue;
                Double_t pos_ADC_sum[4] = {pos_ADC_max[0]*pos_ADC_max[3],pos_ADC_max[1]*pos_ADC_max[3],pos_ADC_max[2]*pos_ADC_max[3],pos_ADC_max[3]}; // positions times ADC value [3]

                //printf("i_time: %d, i_digit_max: %d, ADC: %4.3f \n",i_time,i_digit_max,pos_ADC_max[3]);

                // Get all other digits within a certain radius
                for(Int_t i_digit_sub = (i_digit_max + 1); i_digit_sub < (Int_t)vec_all_TRD_digits[i_det][i_time].size(); i_digit_sub++)
                {
                    if(arr_used_digits[i_digit_sub]) continue;

                    Double_t pos_ADC_sub[4] = {vec_all_TRD_digits[i_det][i_time][i_digit_sub][0],vec_all_TRD_digits[i_det][i_time][i_digit_sub][1],vec_all_TRD_digits[i_det][i_time][i_digit_sub][2],vec_all_TRD_digits[i_det][i_time][i_digit_sub][3] - baseline};

                    Double_t ADC_sub = pos_ADC_sub[3];
                    if(ADC_sub < 0.0) continue;

                    Double_t dist_digits_XY = TMath::Sqrt(TMath::Power(pos_ADC_max[0] - pos_ADC_sub[0],2) + TMath::Power(pos_ADC_max[1] - pos_ADC_sub[1],2));
                    Double_t dist_digits_Z  = fabs(pos_ADC_max[2] - pos_ADC_sub[2]);
                    if(dist_digits_XY > 2.5)  continue; // 2.5
                    if(dist_digits_Z  > 10.0) continue; // 15.0

                    for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                    {
                        pos_ADC_sum[i_xyz] += ADC_sub*pos_ADC_sub[i_xyz];
                    }
                    pos_ADC_sum[3] += pos_ADC_sub[3];

                    arr_used_digits[i_digit_sub] = 1;
                    Sum_ADC_weight += ADC_sub;
                    N_digits_added++;
                }

                if(Sum_ADC_weight <= 0.0) continue;
                // Calculate average position
                for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                {
                    pos_ADC_sum[i_xyz] /= Sum_ADC_weight;
                }



#if defined(USEEVE)
                if(graphics)
                {
                    //if(N_digits_added > 1)
                    {
                        TEve_clusters ->SetNextPoint(pos_ADC_sum[0],pos_ADC_sum[1],pos_ADC_sum[2]);
                    }
                }
#endif



                for(Int_t i_xyzADC = 0; i_xyzADC < 4; i_xyzADC++)
                {
                    vec_digit_cluster_data[i_xyzADC] = pos_ADC_sum[i_xyzADC];
                }
                vec_digit_cluster_data[4] = (Double_t)N_digits_added;

                //printf("i_time: %d, cluster pos: {%4.3f, %4.3f, %4.3f} \n",i_time,vec_digit_cluster_data[0],vec_digit_cluster_data[1],vec_digit_cluster_data[2]);

                vec_all_TRD_digits_clusters[i_det][i_time].push_back(vec_digit_cluster_data);
                vec_used_clusters[i_det][i_time].push_back(0);
            } // end of digit max loop
        } // end of timebin loop
    }
    //printf("arrangement of clusters done \n");
    //-------------------------------------------------------


    //create "tracklets"
    //Int_t sector = (Int_t)(i_det/30);
    //Int_t stack  = (Int_t)(i_det%30/6);
    //Int_t layer  = i_det%6;
    //Int_t i_det = layer + 6*stack + 30*sector;


    //-------------------------------------------------------
    // Connect clusters within each chamber
    // Clusters/time bin -> connect them time bin wise
    vec_self_tracklet_points.clear(); // defined in Ana_Digits_functions.h as static
    vec_self_tracklet_points.resize(540);

    vec_connected_clusters.clear(); // defined in Ana_Digits_functions.h as static
    vec_connected_clusters.resize(540); // i_det i_trkl i_point (up to 24 time bins, can be less) i_xyz

    vector< vector<Double_t> > vec_single_connected_clusters; // for one tracklet, i_point (up to 24 time bins, can be less) i_xyz
    vector<Double_t> vec_single_point; // x,y,z,ADC
    vec_single_point.resize(4); // x,y,z,ADC

    vector< vector<Int_t> > vec_N_clusters_self_tracklet_points;
    vec_N_clusters_self_tracklet_points.resize(540);

    Int_t min_nbr_cls = 10;

    for(Int_t i_det = 0; i_det < 540; i_det++) // is done chamber wise
    {

        for(Int_t i_time = 0; i_time < 24 - min_nbr_cls; i_time++) // is done chamber wise
        //for(Int_t i_time = 0; i_time < 1; i_time++) // is done chamber wise  ALEX
        {
            Int_t N_clusters = (Int_t)vec_all_TRD_digits_clusters[i_det][i_time].size();
            vec_self_tracklet_points[i_det].resize(N_clusters);

            std::sort(vec_all_TRD_digits_clusters[i_det][i_time].begin(),vec_all_TRD_digits_clusters[i_det][i_time].end(),sortcol_first); // large values to small values, last column sorted via function sortcol


            //printf("i_det: %d, N_clusters: %d \n",i_det,N_clusters);

            vec_N_clusters_self_tracklet_points[i_det].resize(N_clusters);

            vector< vector<Int_t> > vec_cls_shared;
            vector<Int_t> vec_single_trkl_cls;

            for(Int_t i_cls = 0; i_cls < N_clusters; i_cls++) // loop over all clusters in one detector and for time bin 0
            {
                vec_single_connected_clusters.clear();

                vec_self_tracklet_points[i_det][i_cls].resize(24);
                for(Int_t i_timebin = 0; i_timebin < 24; i_timebin++) // ALEX why do we set it for all i_time to -999? -> changed
                {
                    vec_self_tracklet_points[i_det][i_cls][i_timebin].resize(4);
                    for (Int_t i_xyzADC = 0; i_xyzADC < 4; i_xyzADC++)
                    {
                        vec_self_tracklet_points[i_det][i_cls][i_timebin][i_xyzADC] = -999.0;
                    }
                }


                vec_single_trkl_cls.push_back(i_cls);
                if(vec_used_clusters[i_det][i_time][i_cls]) continue;
                Int_t n_clusters_attached = 0;
                Double_t pos_ADC_max[4] = {vec_all_TRD_digits_clusters[i_det][i_time][i_cls][0],vec_all_TRD_digits_clusters[i_det][i_time][i_cls][1],vec_all_TRD_digits_clusters[i_det][i_time][i_cls][2],vec_all_TRD_digits_clusters[i_det][i_time][i_cls][3]};

                Int_t N_digits_used = vec_all_TRD_digits_clusters[i_det][i_time][i_cls][4];
                if(N_digits_used <= 1) continue;

#if defined(USEEVE)
                    if(graphics)
                    {
                        //if(i_det == 185 && i_cls == 4)
                        {
                            TEve_connected_clusters ->SetNextPoint(pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);
                            //printf(" i_cls: %d, TEve pos: {%4.3f, %4.3f, %4.3f} \n",i_cls,pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);
                        }
                    }
#endif

                Double_t radius = TMath::Sqrt(TMath::Power(vec_all_TRD_digits_clusters[i_det][i_time][i_cls][0],2) + TMath::Power(vec_all_TRD_digits_clusters[i_det][i_time][i_cls][1],2));

                //if(i_time == 0) printf("---> i_det: %d, i_cls: %d, time 0, pos: {%4.3f, %4.3f, %4.3f}, radius: %4.3f, ADC: %4.3f, N_digits_used: %d \n",i_det,i_cls,vec_all_TRD_digits_clusters[i_det][i_time][i_cls][0],vec_all_TRD_digits_clusters[i_det][i_time][i_cls][1],vec_all_TRD_digits_clusters[i_det][i_time][i_cls][2],radius,vec_all_TRD_digits_clusters[i_det][i_time][i_cls][3],(Int_t)vec_all_TRD_digits_clusters[i_det][i_time][i_cls][4]);

                for(Int_t i_xyzADC = 0; i_xyzADC < 4; i_xyzADC++)
                {
                    vec_self_tracklet_points[i_det][i_cls][i_time][i_xyzADC] = pos_ADC_max[i_xyzADC];
                }

                for(Int_t i_xyzADC = 0; i_xyzADC < 4; i_xyzADC++)
                {
                    vec_single_point[i_xyzADC] = pos_ADC_max[i_xyzADC];
                }
                vec_single_connected_clusters.push_back(vec_single_point);


                Int_t    i_time_start    = i_time + 1;
                Double_t scale_fac_add   = 1.0;
                Int_t    missed_time_bin = 0;
                Double_t radius_prev     = 0.0;

                Double_t sum_cluster_quality = 0.0;
                for(Int_t i_time_sub = i_time_start; i_time_sub < 24; i_time_sub++)
                {
                    Double_t scale_fac = 1.0*scale_fac_add;
                    //printf("i_time_sub: %d, scale_fac: %4.3f \n",i_time_sub,scale_fac);
                    //if(i_time_sub == 0) scale_fac = factor_layer*scale_fac_add;
                    Int_t N_clusters_sub = (Int_t)vec_all_TRD_digits_clusters[i_det][i_time_sub].size();

                    Int_t best_sub_cluster = -1;
                    Double_t best_cluster_quality = 1000000.0;

                    for(Int_t i_cls_sub = 0; i_cls_sub < N_clusters_sub; i_cls_sub++)
                    {
                        if(vec_used_clusters[i_det][i_time_sub][i_cls_sub]) continue;

                        Double_t pos_ADC_sub[4] = {vec_all_TRD_digits_clusters[i_det][i_time_sub][i_cls_sub][0],vec_all_TRD_digits_clusters[i_det][i_time_sub][i_cls_sub][1],vec_all_TRD_digits_clusters[i_det][i_time_sub][i_cls_sub][2],vec_all_TRD_digits_clusters[i_det][i_time_sub][i_cls_sub][3]};

                        Double_t dist_clusters_XY = TMath::Sqrt(TMath::Power(pos_ADC_max[0] - pos_ADC_sub[0],2) + TMath::Power(pos_ADC_max[1] - pos_ADC_sub[1],2));
                        Double_t dist_clusters_Z  = fabs(pos_ADC_max[2] - pos_ADC_sub[2]);

                        if(dist_clusters_XY > scale_fac*Delta_x)  continue;
                        if(dist_clusters_Z  > Delta_z) continue;
                        //if(dist_clusters_XY > scale_fac*3.0)  continue;
                        //if(dist_clusters_Z  > 10.0) continue;

                        // Matching quality - chi2 like
                        Double_t sub_cluster_quality = TMath::Power(dist_clusters_XY/0.7,2.0) + TMath::Power(dist_clusters_Z/7.5,2.0);


                        if(sub_cluster_quality < best_cluster_quality)
                        {
                            best_cluster_quality = sub_cluster_quality;
                            best_sub_cluster     = i_cls_sub;
                        }
                    }

                    sum_cluster_quality += best_cluster_quality;
                    vec_single_trkl_cls.push_back(best_sub_cluster);

                    //cout << "best_sub_cluster: " << best_sub_cluster << ", i_time_sub: " << i_time_sub << ", i_layer_sub: " << i_layer_sub << endl;
                    if(best_sub_cluster < 0)
                    {
                        scale_fac_add *= factor_missing; // one time bin was missing, increase matching window
                        missed_time_bin++;
                        continue;
                    }

                    if(missed_time_bin > 3) break;
                    scale_fac_add = 1.0; // reset additional matching window factor once a match was found

                    //if(missed_time_bin > 3) break;
                    //scale_fac_add = 1.0; 

                    // Define new pos_ADC_max
                    for(Int_t i_xyzADC = 0; i_xyzADC < 4; i_xyzADC++)
                    {
                        pos_ADC_max[i_xyzADC] = vec_all_TRD_digits_clusters[i_det][i_time_sub][best_sub_cluster][i_xyzADC]; // The one connected is now the new "first" one for the next search step in time
                        vec_self_tracklet_points[i_det][i_cls][i_time_sub][i_xyzADC] = pos_ADC_max[i_xyzADC];
                        vec_single_point[i_xyzADC] = pos_ADC_max[i_xyzADC];

                        // To do: define array with number of clusters connected
                        // -> cut on those for fit
                    }

                    vec_single_connected_clusters.push_back(vec_single_point);

#if defined(USEEVE)
                    if(graphics)
                    {
                        //if(i_det == 185)// && i_cls == 0)
                        {
                            TEve_connected_clusters ->SetNextPoint(pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);
                            //if (i_det == 37)printf(" i_ det = %d, i_cls: %d, TEve pos: {%4.3f, %4.3f, %4.3f} \n",i_det,i_cls,pos_ADC_max[0],pos_ADC_max[1],pos_ADC_max[2]);
                        }
                    }
#endif

                    vec_used_clusters[i_det][i_time_sub][best_sub_cluster] = 1;
                    Double_t radius_sub = TMath::Sqrt(TMath::Power(vec_self_tracklet_points[i_det][i_cls][i_time_sub][0],2) + TMath::Power(vec_self_tracklet_points[i_det][i_cls][i_time_sub][1],2));
                    //radii_tracklets_final -> Fill(radius_sub);
                    radii_digits_initial -> Fill(radius_sub);

                    // Already sometimes wrong radius-time ordering
                    //if(i_time_sub > 1 && radius_prev < radius_sub) printf("---> i_det: %d, i_cls: %d, time %d, pos: {%4.3f, %4.3f, %4.3f}, radius_sub: %4.3f, radius_prev: %4.3f \n",i_det,i_cls,i_time_sub,vec_self_tracklet_points[i_det][i_cls][i_time_sub][0],vec_self_tracklet_points[i_det][i_cls][i_time_sub][1],vec_self_tracklet_points[i_det][i_cls][i_time_sub][2],radius_sub,radius_prev);
                    radius_prev = radius_sub;

                    //vec_TEveLine_cluster_tracks[(Int_t)vec_TEveLine_cluster_tracks.size()-1] ->SetNextPoint((Float_t)pos_ADC_max[0],(Float_t)pos_ADC_max[1],(Float_t)pos_ADC_max[2]);
                    //vec_TPL_cluster_tracks[i_sector][i_stack][(Int_t)vec_TPL_cluster_tracks[i_sector][i_stack].size()-1] ->SetNextPoint((Float_t)pos_ADC_max[0],(Float_t)pos_ADC_max[1]);

                    n_clusters_attached++;
                } // end of time sub loop

                vec_N_clusters_self_tracklet_points[i_det][i_cls] = n_clusters_attached;


                vec_connected_clusters[i_det].push_back(vec_single_connected_clusters);
            } // end of cluster loop


        } // end of time loop
    }

    //printf("connection of clusters within detector done \n");
    //printf("tracklets fit starts now \n");
    //-------------------------------------------------------



    //-------------------------------------------------------
    // Fit the time bin wise connected clusters

    //ready to fit vec_self_tracklet_points[i_det][i_cls][i_time_sub][i_xyzADC]

    // Is fitting the tracklets and doing the global fit through all first cluster points of all available layers
    //printf("TTRD_ST_Make_Tracklets::get_tracklets_fit((%d) \n",i_track);

    //fit merged digits with a straight line
    vec_self_tracklet_fit_points.clear();
    vec_self_tracklet_fit_points.resize(540);         //[i_det][i_trkl][i_start_stop][i_xyz]

    for(Int_t i_detector = 0; i_detector < 540; i_detector++)
    {
        vec_self_tracklet_fit_points[i_detector].resize((Int_t)vec_connected_clusters[i_detector].size());

        vec_ADC_val[i_detector].resize((Int_t)vec_connected_clusters[i_detector].size());

        for (Int_t i_trkl = 0; i_trkl < (Int_t)vec_connected_clusters[i_detector].size(); i_trkl++)
        {
            vec_self_tracklet_fit_points[i_detector][i_trkl].resize(2);
            for(Int_t i_start_stop = 0; i_start_stop < 2; i_start_stop++)
            {
                vec_self_tracklet_fit_points[i_detector][i_trkl][i_start_stop].resize(3);
                for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                {
                    vec_self_tracklet_fit_points[i_detector][i_trkl][i_start_stop][i_xyz] = -999.0;
                }
            }
        }
    }


    Int_t trkl_index_layer[6] = {0};
    Int_t trkl_index=0;
    for(Int_t i_det = 0; i_det < 540; i_det++)
    {
        //if(i_det == 0) printf("fitting detector %d  \n",i_det);
        //if(i_det % 40 == 0) printf("fitting detector %d  \n",i_det);

        Int_t i_layer  = i_det%6;


        //printf("i_det: %d, N_tracklets: %d \n",i_det,(Int_t)vec_self_tracklet_points[i_det].size());

        //for(Int_t i_trkl = 0; i_trkl < (Int_t)vec_self_tracklet_points[i_det].size(); i_trkl++) // ALEX
        for(Int_t i_trkl = 0; i_trkl < (Int_t)vec_connected_clusters[i_det].size(); i_trkl++)
        {
            //printf("i_det: %d, i_trkl: %d, N_clusters: %d \n",i_det,i_trkl,vec_N_clusters_self_tracklet_points[i_det][i_trkl]);
            if((Int_t)vec_connected_clusters[i_det][i_trkl].size() < 10) continue; // ALEX

            //------------------------------
            Double_t SVD_chi2 = Calc_SVD_tracklet(i_det,i_trkl);

            if(SVD_chi2 > 0.5) continue; // 0.5 ALEX


            //TV3_SVD_tracklet_offset.Print();
            //TV3_SVD_tracklet_dir.Print();

            //vec_ADC_val[i_det][i_trkl].resize((Int_t)vec_self_tracklet_points[i_det][i_trkl].size());
            vec_ADC_val[i_det][i_trkl].resize((Int_t)vec_connected_clusters[i_det][i_trkl].size()); // ALEX


#if defined(USEEVE)
            if(graphics && 1 == 0) // use for test purposes to check SVD calculation directly
            {
                TEveLine_fitted_tracklets[i_layer].resize(trkl_index_layer[i_layer]+1);
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]] = new TEveLine();
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]]->SetNextPoint(TV3_SVD_tracklet_offset[0],TV3_SVD_tracklet_offset[1],TV3_SVD_tracklet_offset[2]);
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]]->SetNextPoint(TV3_SVD_tracklet_offset[0] + scale_length_vec*TV3_SVD_tracklet_dir[0],TV3_SVD_tracklet_offset[1] + scale_length_vec*TV3_SVD_tracklet_dir[1],TV3_SVD_tracklet_offset[2] + scale_length_vec*TV3_SVD_tracklet_dir[2]);

                //printf("tracklets start: {%4.3f, %4.3f, %4.3f}, tracklets stop: {%4.3f, %4.3f, %4.3f} \n", TV3_SVD_tracklet_offset[0],TV3_SVD_tracklet_offset[1],TV3_SVD_tracklet_offset[2],
                    //TV3_SVD_tracklet_offset[0] + scale_length_vec*TV3_SVD_tracklet_dir[0],TV3_SVD_tracklet_offset[1] + scale_length_vec*TV3_SVD_tracklet_dir[1],TV3_SVD_tracklet_offset[2] + scale_length_vec*TV3_SVD_tracklet_dir[2]);


                HistName = "trkl ";
                HistName += trkl_index_layer[i_layer];
                HistName += " gbl ";
                HistName += i_trkl;
                HistName += " det ";
                HistName += i_det;
                HistName += " layer ";
                HistName += i_layer;
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]]    ->SetName(HistName.Data());
            }
#endif
            //------------------------------

            //-------------------------------------------------------
            // Calculate tracklet base and direction vectors

            // Space point on straight line which is closes to first space point of fitted clusters
            TVector3 TV3_base_plane = vec_TV3_TRD_center_offset[i_det];
            TVector3 TV3_norm_plane = vec_TV3_TRD_center[i_det][2];
            TVector3 TV3_base_fit_t0 = intersect_line_plane(TV3_SVD_tracklet_offset,TV3_SVD_tracklet_dir,TV3_base_plane,TV3_norm_plane);

            for(Int_t a=0;a<(Int_t)vec_connected_clusters[i_det][i_trkl].size();a++)
            {
                Double_t radius_sub = TMath::Sqrt(TMath::Power(vec_connected_clusters[i_det][i_trkl][a][0],2) + TMath::Power(vec_connected_clusters[i_det][i_trkl][a][1],2));
                //radii_tracklets_final -> Fill(radius_sub);
                radii_tracklets_final -> Fill(radius_sub);


            }
            Double_t TV3_base_fit_t0_radius = TMath::Sqrt(TMath::Power(TV3_base_fit_t0[0],2) + TMath::Power(TV3_base_fit_t0[1],2));

            //TV3_base_fit_t0.Print();
            //TV3_SVD_tracklet_dir.Print();


            TVector3 vec_AB[2];
            vec_AB[0] = TV3_base_fit_t0;
            vec_AB[1] = TV3_base_fit_t0 + TV3_SVD_tracklet_dir;
            //if(vec_AB[1].Mag() > vec_AB[0].Mag())  // changed sign -> correct physical direction, pointing inwards
            if(vec_AB[1].Perp() < vec_AB[0].Perp())  // changed sign -> "uncorrect" direction, pointing outwards
            {
                TV3_SVD_tracklet_dir *= -1.0;
                vec_AB[1] = TV3_base_fit_t0 + TV3_SVD_tracklet_dir;
            }


            for(Int_t i_AB = 0; i_AB < 2; i_AB++)
            {
                for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                {
                    vec_self_tracklet_fit_points[i_det][i_trkl][i_AB][i_xyz] = vec_AB[i_AB][i_xyz];
                }
            }


            // fill ADC val vector
            Int_t tbn_max = (Int_t)vec_connected_clusters[i_det][i_trkl].size();
            //Double_t radius_in  = TMath::Sqrt( TMath::Power(vec_connected_clusters[i_det][i_trkl][tbn_max-1][0],2) + TMath::Power(vec_connected_clusters[i_det][i_trkl][tbn_max-1][1],2) );
            //Double_t radius_out = TMath::Sqrt( TMath::Power(vec_connected_clusters[i_det][i_trkl][0][0],2) + TMath::Power(vec_connected_clusters[i_det][i_trkl][0][1],2) );

            for(Int_t i_timebin = 0; i_timebin < tbn_max; i_timebin++)
            {
                //printf("i_timebin: %d, value: %4.3f \n",i_timebin,vec_connected_clusters[i_det][i_trkl][i_timebin][3]);
                vec_ADC_val[i_det][i_trkl][i_timebin] = vec_connected_clusters[i_det][i_trkl][i_timebin][3]; // ALEX
            }

            //-------------------------------------------------------

            Double_t radius = TMath::Sqrt( TMath::Power(TV3_base_fit_t0[0],2) + TMath::Power(TV3_base_fit_t0[1],2) );
            //printf("amin: %4.3f, par: {%4.3f, %4.3f, %4.3f, %4.3f} \n",amin,parFit[0],parFit[1],parFit[2],parFit[3]);
            //printf("   --> radius: %4.3f, point first cluster: {%4.3f, %4.3f, %4.3f}, point line: {%4.3f, %4.3f, %4.3f} \n",radius,TV3_t0_point[0],TV3_t0_point[1],TV3_t0_point[2],TV3_base_fit_t0[0],TV3_base_fit_t0[1],TV3_base_fit_t0[2]);


#if defined(USEEVE)
            if(graphics)
            {
                TEveLine_fitted_tracklets[i_layer].resize(trkl_index_layer[i_layer]+1);
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]] = new TEveLine();
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]]->SetNextPoint(TV3_base_fit_t0[0],TV3_base_fit_t0[1],TV3_base_fit_t0[2]);
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]]->SetNextPoint(TV3_base_fit_t0[0] + scale_length_vec*TV3_SVD_tracklet_dir[0],TV3_base_fit_t0[1] + scale_length_vec*TV3_SVD_tracklet_dir[1],TV3_base_fit_t0[2] + scale_length_vec*TV3_SVD_tracklet_dir[2]);

                //printf("tracklets start: {%4.3f, %4.3f, %4.3f}, tracklets stop: {%4.3f, %4.3f, %4.3f} \n", TV3_base_fit_t0[0],TV3_base_fit_t0[1],TV3_base_fit_t0[2],
                    //TV3_base_fit_t0[0] + scale_length_vec*TV3_SVD_tracklet_dir[0],TV3_base_fit_t0[1] + scale_length_vec*TV3_SVD_tracklet_dir[1],TV3_base_fit_t0[2] + scale_length_vec*TV3_SVD_tracklet_dir[2]);


                HistName = "trkl ";
                HistName += trkl_index_layer[i_layer];
                HistName += " gbl ";
                HistName += i_trkl;
                HistName += " det ";
                HistName += i_det;
                HistName += " layer ";
                HistName += i_layer;
                TEveLine_fitted_tracklets[i_layer][trkl_index_layer[i_layer]]    ->SetName(HistName.Data());
            }
#endif

            trkl_index_layer[i_layer]++;
            trkl_index++;
        }
    }
    //-------------------------------------------------------

#if defined(USEEVE)
    if(graphics)
    {
        for(Int_t i_ADC = 0; i_ADC < 7; i_ADC++)
        {
            TEveP_digits[i_ADC]  ->SetMarkerSize(0.3);
            TEveP_digits[i_ADC]  ->SetMarkerStyle(20);
            TEveP_digits[i_ADC]  ->SetMarkerColor(arr_color_ADC_digits[i_ADC]);
            gEve->AddElement(TEveP_digits[i_ADC]);
        }

        TEveP_digits_flagged  ->SetMarkerSize(1.5);
        TEveP_digits_flagged  ->SetMarkerStyle(20);
        TEveP_digits_flagged  ->SetMarkerColor(kOrange+7);
        gEve->AddElement(TEveP_digits_flagged);

        TEve_clusters  ->SetMarkerSize(1.0);
        TEve_clusters  ->SetMarkerStyle(20);
        TEve_clusters  ->SetMarkerColor(kMagenta+1);
        //gEve->AddElement(TEve_clusters);  // -->

        TEve_connected_clusters  ->SetMarkerSize(1.2);
        TEve_connected_clusters  ->SetMarkerStyle(20);
        TEve_connected_clusters  ->SetMarkerColor(kAzure-2);
        gEve->AddElement(TEve_connected_clusters);


        for(Int_t i_layer = 0; i_layer < 6; i_layer++)
        {
            for(Int_t i_tracklet = 0; i_tracklet < (Int_t)TEveLine_fitted_tracklets[i_layer].size(); i_tracklet++)
            {
                TEveLine_fitted_tracklets[i_layer][i_tracklet]    ->SetLineStyle(1);
                TEveLine_fitted_tracklets[i_layer][i_tracklet]    ->SetLineWidth(3);
                TEveLine_fitted_tracklets[i_layer][i_tracklet]    ->SetMainColor(color_layer[i_layer]);
                gEve->AddElement(TEveLine_fitted_tracklets[i_layer][i_tracklet]);
            }
        }


        gEve->Redraw3D(kTRUE);
    }
#endif


}

//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
Int_t TTRD_ST_Make_Tracklets::Calibrate(Double_t Delta_x, Double_t Delta_z, Double_t factor_missing, Int_t graphics)
{
    //printf("TTRD_ST_Make_Tracklets::Loop_full_self_tracking() \n");

    TVector3 TV3_trkl_offset;
    TVector3 TV3_trkl_dir;
    Float_t  helix_par[9];
    Double_t ADC_val[24];

    //for(Long64_t i_event = 0; i_event < file_entries_total; i_event++)
    for(Long64_t i_event = 0; i_event < 1; i_event++)
    {
        if (!input_SE->GetEntry( i_event )) return 0; // take the event -> information is stored in event


        //------------------------------------------
        // Fill event information
        TRD_ST_Event ->clearTrackList();
        TRD_ST_Event ->clearTrackletList();

        TRD_ST_Event ->setTriggerWord( AS_Event->getTriggerWord() );
        TRD_ST_Event ->setx( AS_Event->getx() );
        TRD_ST_Event ->sety( AS_Event->gety() );
        TRD_ST_Event ->setz( AS_Event->getz() );
        TRD_ST_Event ->setid( AS_Event->getid() );
        TRD_ST_Event ->setN_tracks( AS_Event->getN_tracks() );
        TRD_ST_Event ->setN_TRD_tracklets( AS_Event->getN_TRD_tracklets() );
        TRD_ST_Event ->setBeamIntAA( AS_Event->getBeamIntAA() );
        TRD_ST_Event ->setT0zVertex( AS_Event->getT0zVertex() );
        TRD_ST_Event ->setcent_class_ZNA( AS_Event->getcent_class_ZNA() );
        TRD_ST_Event ->setcent_class_ZNC( AS_Event->getcent_class_ZNC() );
        TRD_ST_Event ->setcent_class_V0A( AS_Event->getcent_class_V0A() );
        TRD_ST_Event ->setcent_class_V0C( AS_Event->getcent_class_V0C() );
        TRD_ST_Event ->setcent_class_V0M( AS_Event->getcent_class_V0M() );
        TRD_ST_Event ->setcent_class_CL0( AS_Event->getcent_class_CL0() );
        TRD_ST_Event ->setcent_class_CL1( AS_Event->getcent_class_CL1() );
        TRD_ST_Event ->setcent_class_SPD( AS_Event->getcent_class_SPD() );
        TRD_ST_Event ->setcent_class_V0MEq( AS_Event->getcent_class_V0MEq() );
        TRD_ST_Event ->setcent_class_V0AEq( AS_Event->getcent_class_V0AEq() );
        TRD_ST_Event ->setcent_class_V0CEq( AS_Event->getcent_class_V0CEq() );

        //printf("Event information filled \n");
        //------------------------------------------



        //------------------------------------------
        // Loop over all tracks
        UShort_t NumTracks = AS_Event ->getNumTracks(); // number of tracks in this event

        if(i_event % 1 == 0) printf("i_event: %lld out of %lld, NumTracks: %d \n",i_event,file_entries_total,NumTracks);
        for(UShort_t i_track = 0; i_track < NumTracks; ++i_track) // loop over all tracks of the actual event
        {
            TRD_ST_TPC_Track = TRD_ST_Event ->createTrack(); // TPC track
            //cout << "i_track: " << i_track << ", of " << NumTracks << endl;
            AS_Track      = AS_Event ->getTrack( i_track ); // take the track
            TRD_ST_TPC_Track ->setnsigma_e_TPC( AS_Track ->getnsigma_e_TPC() );
            TRD_ST_TPC_Track ->setnsigma_pi_TPC( AS_Track ->getnsigma_pi_TPC() );
            TRD_ST_TPC_Track ->setnsigma_p_TPC( AS_Track ->getnsigma_p_TPC() );
            TRD_ST_TPC_Track ->setnsigma_e_TOF( AS_Track ->getnsigma_e_TOF() );
            TRD_ST_TPC_Track ->setnsigma_pi_TOF( AS_Track ->getnsigma_pi_TOF() );
            TRD_ST_TPC_Track ->setTRDSignal( AS_Track ->getTRDSignal() );
            TRD_ST_TPC_Track ->setTRDsumADC( AS_Track ->getTRDsumADC() );
            TRD_ST_TPC_Track ->setdca( AS_Track ->getdca() );  // charge * distance of closest approach to the primary vertex
            TRD_ST_TPC_Track ->set_TLV_part( AS_Track ->get_TLV_part() );
            TRD_ST_TPC_Track ->setNTPCcls( AS_Track ->getNTPCcls() );
            TRD_ST_TPC_Track ->setNTRDcls( AS_Track ->getNTRDcls() );
            TRD_ST_TPC_Track ->setNITScls( AS_Track ->getNITScls() );
            TRD_ST_TPC_Track ->setTPCchi2( AS_Track ->getTPCchi2() );
            TRD_ST_TPC_Track ->setTPCdEdx( AS_Track ->getTPCdEdx() );
            TRD_ST_TPC_Track ->setTOFsignal( AS_Track ->getTOFsignal() ); // in ps (1E-12 s)
            TRD_ST_TPC_Track ->setTrack_length( AS_Track ->getTrack_length() );

            for(Int_t i_helix_par = 0; i_helix_par < 9; i_helix_par++)
            {
                helix_par[i_helix_par] =  AS_Track ->getHelix_param(i_helix_par);
            }

            TRD_ST_TPC_Track ->setHelix(helix_par[0],helix_par[1],helix_par[2],helix_par[3],helix_par[4],helix_par[5],helix_par[6],helix_par[7],helix_par[8]);
        }

        //printf("Track information filled \n");
        //------------------------------------------




        //------------------------------------------
        // Loop over all TRD tracklets
        Make_clusters_and_get_tracklets_fit(Delta_x,Delta_z,factor_missing,graphics);
        for(Int_t i_det = 0; i_det < 540; i_det++)
        {
            //printf("i_det: %d \n",i_det);
            for(Int_t i_trkl = 0; i_trkl < (Int_t)vec_self_tracklet_fit_points[i_det].size(); i_trkl++)
            {
                for (Int_t i_tbn = 0; i_tbn < 24; i_tbn++)
                {
                    ADC_val[i_tbn] = {-999.0};
                }

                //printf("i_trkl: %d \n",i_trkl);
                for(Int_t i_AB = 0; i_AB < 2; i_AB++)
                {
                    for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                    {
                        //printf("i_AB: %d, i_xyz: %d \n",i_AB,i_xyz);
                        if(i_AB == 0) TV3_trkl_offset[i_xyz] = vec_self_tracklet_fit_points[i_det][i_trkl][i_AB][i_xyz];
                        if(i_AB == 1) TV3_trkl_dir[i_xyz]    = vec_self_tracklet_fit_points[i_det][i_trkl][i_AB][i_xyz];
                    }
                }

                TV3_trkl_dir -= TV3_trkl_offset;

                TRD_ST_Tracklet = TRD_ST_Event ->createTracklet(); // online tracklet
                TRD_ST_Tracklet  ->set_TRD_det(i_det);
                TRD_ST_Tracklet  ->set_TV3_offset(TV3_trkl_offset);
                TRD_ST_Tracklet  ->set_TV3_dir(TV3_trkl_dir);
                for(Int_t i_timebin = 0; i_timebin < (Int_t)vec_ADC_val[i_det][i_trkl].size(); i_timebin++) // ALEX
                {
                    ADC_val[i_timebin] = vec_ADC_val[i_det][i_trkl][i_timebin];
                    TRD_ST_Tracklet  ->set_ADC_val(i_timebin,ADC_val[i_timebin]);
                }

            } // end tracklet loop
        } // end detector loop

        //printf("Tracklet information filled \n");
        //------------------------------------------


        //printf("Fill tree \n");
        Tree_TRD_ST_Event ->Fill();
        //printf("Tree filled for event %lld \n",i_event);
    }

    printf("Write data to file \n");
    outputfile ->cd();
    Tree_TRD_ST_Event ->Write();

#if 0
    for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
    {
        for(Int_t i_vertex = 0; i_vertex < 8; i_vertex++)
        {
            vec_TH1D_TRD_geometry[i_xyz][i_vertex] ->Write();
        }

    }
#endif

    printf("All data written \n");

    return 1;
}
//----------------------------------------------------------------------------------------



void TTRD_ST_Make_Tracklets::plot_dem_histos1()
{
	TCanvas *can_radii_digits_initial = new TCanvas("can_radii_digits_initial", "can_radii_digits_initial",10,10,500,500);
    can_radii_digits_initial->cd();
    radii_digits_initial -> Draw();
	
	//TCanvas *can_radii_tracklets_final = new TCanvas("can_radii_tracklets_final", "can_radii_tracklets_final",10,10,500,500);
    //can_radii_digits_initial->cd();
    //radii_tracklets_final -> Draw("same");
	

}	
void TTRD_ST_Make_Tracklets::plot_dem_histos2()
{
	//TCanvas *can_radii_digits_initial = new TCanvas("can_radii_digits_initial", "can_radii_digits_initial",10,10,500,500);
    //can_radii_digits_initial->cd();
    //radii_digits_initial -> Draw();
	
	TCanvas *can_radii_tracklets_final = new TCanvas("can_radii_tracklets_final", "can_radii_tracklets_final",10,10,500,500);
    can_radii_tracklets_final->cd();
    radii_tracklets_final -> Draw();
	

}	




