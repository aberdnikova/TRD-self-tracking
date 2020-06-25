
#include "Ali_TRD_ST.h"
#include "Ali_TRD_ST_LinkDef.h"


//----------------------------------------------------------------------------------------
class Ali_TRD_ST_Analyze
{
private:
    TChain* input_SE;
    TString TRD_ST_TREE   = "Tree_TRD_ST_Event";
    TString TRD_ST_BRANCH = "Tree_TRD_ST_Event_branch";
    Long64_t file_entries_total;
    Long64_t N_Events;
    Ali_TRD_ST_Tracklets* TRD_ST_Tracklet;
    Ali_TRD_ST_TPC_Track* TRD_ST_TPC_Track;
    Ali_TRD_ST_Event*     TRD_ST_Event;

    Ali_TRD_ST_Tracklets* TRD_ST_Tracklet_out;
    Ali_TRD_ST_TPC_Track* TRD_ST_TPC_Track_out;
    Ali_TRD_ST_Event*     TRD_ST_Event_out;
    TTree* Tree_TRD_ST_Event_out;

    TFile* outputfile;
    Double_t test;

    TString HistName;
    TH1D* th1d_TRD_layer_radii;
    TH1D* th1d_offset_diff;
    TH1D* th1d_angle_diff;

    TEveLine* TEveLine_beam_axis = NULL;
    TEveLine* TPL3D_helix = NULL;
    vector<TEveLine*> vec_TPL3D_helix;
    vector<TEveLine*> vec_TPL3D_helix_inner;
    vector<TEveLine*> vec_TPL3D_helix_hull;

    vector<TEveLine*> vec_TPL3D_helix_kalman;
    vector<TEveLine*> vec_TPL3D_helix_kalman_inner;
    vector<TEveLine*> vec_TPL3D_helix_kalman_hull;

    vector< vector<TEveLine*> > vec_TEveLine_tracklets;
    vector< vector<TEveLine*> > vec_TEveLine_tracklets_match;
    vector< vector<TEveLine*> > vec_TEveLine_self_matched_tracklets;
    Int_t N_tracklets_layers[6] = {0};
    Double_t scale_length = 10.0;
    Int_t track_color    = kAzure-2;
    Int_t color_layer_match[6] = {kRed,kGreen,kCyan,kYellow,kPink-3,kOrange+8};
    Int_t color_layer[6] = {kGray,kGray,kGray,kGray,kGray,kGray};
    Double_t TRD_layer_radii[6][2] =
    {
        {297.5,306.5},
        {310.0,320.0},
        {323.0,333.0},
        {336.0,345.5},
        {348.0,357.0},
        {361.0,371.0}
    };

    Int_t Not_installed_TRD_detectors[19] = {402,403,404,405,406,407,432,433,434,435,436,437,462,463,464,465,466,467,538};
    TH1I* h_good_bad_TRD_chambers;

    // TRD 3D graphics
    vector< vector<TH1D*> > vec_TH1D_TRD_geometry; // store for all 540 chambers the 8 corner vertices per detector
    vector<TEveBox*> vec_eve_TRD_detector_box;

    vector<vector<Double_t>> mHelices_kalman; // Kalman helix parameters, based on AliHelix
    Double_t aliHelix_params[6];
    vector<Ali_Helix*> vec_helices;
    TEvePointSet* TEveP_sec_vertices;

public:
    Ali_TRD_ST_Analyze();
    ~Ali_TRD_ST_Analyze();
    void Init_tree(TString SEList);
    void Loop_event(Long64_t i_event);
    void Draw_event(Long64_t i_event);
    void Do_TPC_TRD_matching(Long64_t i_event, Double_t xy_matching_window, Double_t z_matching_window);
    void Do_TPC_TRD_matching_allEvents(Double_t xy_matching_window, Double_t z_matching_window);
    void Do_TRD_self_matching(Long64_t i_event, Double_t xy_matching_window, Double_t z_matching_window);
    void Draw_hist_TPC_tracklet_diffs();
	TH1I* get_h_good_bad_TRD_chambers();

	Ali_TRD_ST_Tracklets** Tracklets;
	vector< vector<Ali_TRD_ST_Tracklets*> > matched_tracks;
    Int_t Number_Tracklets;
    void Draw_Kalman_Tracks(vector< vector<Ali_TRD_ST_Tracklets*> > found_tracks);
    void set_Kalman_helix_params(vector<vector<Double_t>> mHelices_kalman_in);
    void Draw_Kalman_Helix_Tracks();
    void set_single_helix_params(vector<Double_t> vec_params);
    void Evaluate(Double_t t, // helix evaluation, taken from AliHelix
                  Double_t r[3]);  //radius vector
    void fHelixAtoPointdca(TVector3 space_vec, Ali_Helix* helixA, Float_t &pathA, Float_t &dcaAB);
    void fHelixABdca(Ali_Helix* helixA, Ali_Helix* helixB, Float_t &pathA, Float_t &pathB, Float_t &dcaAB);
    Int_t fCross_points_Circles(Double_t x1, Double_t y1, Double_t r1, Double_t x2, Double_t y2, Double_t r2,Double_t &x1_c, Double_t &y1_c, Double_t &x2_c, Double_t &y2_c);
    Int_t fDCA_Helix_Estimate(Ali_Helix* helixA, Ali_Helix* helixB, Float_t &pathA, Float_t &pathB, Float_t &dcaAB);
    void Calculate_secondary_vertices(Int_t graphics);
	pair<Double_t,Double_t>fpathLength(Double_t r) const;
	Int_t fCircle_Interception(Double_t x1, Double_t y1, Double_t r1, Double_t x2, Double_t y2, Double_t r2);

    ClassDef(Ali_TRD_ST_Analyze, 1)
};
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
Ali_TRD_ST_Analyze::Ali_TRD_ST_Analyze()
{
    outputfile = new TFile("./TRD_Calib_matched.root","RECREATE");
    //------------------------------------------------
    outputfile ->cd();
    // TRD self tracking output data containers
    TRD_ST_Tracklet_out   = new Ali_TRD_ST_Tracklets();
    TRD_ST_TPC_Track_out  = new Ali_TRD_ST_TPC_Track();
    TRD_ST_Event_out      = new Ali_TRD_ST_Event();

    Tree_TRD_ST_Event_out  = NULL;
    Tree_TRD_ST_Event_out  = new TTree("Tree_TRD_ST_Event_out" , "TRD_ST_Events_out" );
    Tree_TRD_ST_Event_out  ->Branch("Tree_TRD_ST_Event_branch_out"  , "TRD_ST_Event_out", TRD_ST_Event_out );
    Tree_TRD_ST_Event_out  ->SetAutoSave( 5000000 );
    //------------------------------------------------
    // constructor
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

    th1d_TRD_layer_radii = new TH1D("th1d_TRD_layer_radii","th1d_TRD_layer_radii",900,250,400.0);



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
        cout << "chamber: " << i_chamber << ", it: "  << *it << ", " << t_flags->at(i_chamber) << endl;
        h_good_bad_TRD_chambers ->SetBinContent(i_chamber+1,t_flags->at(i_chamber));
        i_chamber++;
    }
    //--------------------------

    //--------------------------
    // Load TRD geometry
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
    vec_eve_TRD_detector_box.resize(540);
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
    //--------------------------



}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::fHelixAtoPointdca(TVector3 space_vec, Ali_Helix* helixA, Float_t &pathA, Float_t &dcaAB)
{
    // V1.1
    Float_t pA[2] = {0.0,-100.0}; // the two start values for pathB, 0.0 is the origin of the helix at the first measured point
    Float_t distarray[2];
    TVector3 testA;
    Double_t helix_point[3];
    for(Int_t r = 0; r < 2; r++)
    {
        helixA ->Evaluate(pA[r],helix_point);  // 3D-vector of helixB point at path pB[r]
        testA.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

        distarray[r] = (testA-space_vec).Mag(); // dca between helixA and helixB
    }
    Int_t loopcounter = 0;
    Float_t scale = 1.0;
    Float_t flip  = 1.0; // checks if the minimization direction changed
    Float_t scale_length = 100.0;
    while(fabs(scale_length) > 0.01 && loopcounter < 100) // stops when the length is too small
    {
        //cout << "n = " << loopcounter << ", pA[0] = " << pA[0]
        //    << ", pA[1] = " << pA[1] << ", d[0] = " << distarray[0]
        //    << ", d[1] = " << distarray[1] << ", flip = " << flip
        //    << ", scale_length = " << scale_length << endl;
        if(distarray[0] > distarray[1])
        {
            if(loopcounter != 0)
            {
                if(flip == 1.0) scale = 0.4; // if minimization direction changes -> go back, but only the way * 0.4
                else scale = 0.7; // go on in this direction but only by the way * 0.7
            }
            scale_length = (pA[1]-pA[0])*scale; // the next length interval
            pA[0]     = pA[1] + scale_length; // the new path

            helixA ->Evaluate(pA[0],helix_point);  // 3D-vector of helixB point at path pB[r]
            testA.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

            distarray[0] = (testA-space_vec).Mag(); // new dca
            flip = 1.0;
        }
        else
        {
            if(loopcounter != 0)
            {
                if(flip == -1.0) scale = 0.4;
                else scale = 0.7;
            }
            scale_length = (pA[0]-pA[1])*scale;
            pA[1]     = pA[0] + scale_length;
            helixA ->Evaluate(pA[1],helix_point);  // 3D-vector of helixB point at path pB[r]
            testA.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
            distarray[1] = (testA-space_vec).Mag();
            flip = -1.0;
        }
        loopcounter++;
    }
    if(distarray[0] < distarray[1])
    {
        pathA = pA[0];
        dcaAB = distarray[0];
    }
    else
    {
        pathA = pA[1];
        dcaAB = distarray[1];
    }
    //cout << "pathA = " << pathA << ", dcaAB = " << dcaAB << endl;
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
Int_t Ali_TRD_ST_Analyze::fCross_points_Circles(Double_t x1, Double_t y1, Double_t r1, Double_t x2, Double_t y2, Double_t r2,
                           Double_t &x1_c, Double_t &y1_c, Double_t &x2_c, Double_t &y2_c)
{
    // (x1,y1) -> center of circle 1, r1 -> radius of circle 1
    // (x2,y2) -> center of circle 2, r2 -> radius of circle 2
    // (x1_c,y1_c) -> crossing point 1
    // (x2_c,y2_c) -> crossing point 2
    // Solution see Wikipedia

    Double_t s = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));  // distance between the center of the circles

    x1_c = 0;
    y1_c = 0;
    x2_c = 0;
    y2_c = 0;

    if(x1 != x2 && y1 != y2 && s < (r1+r2))
    {
        Double_t m  = (x1-x2)/(y2-y1);
        Double_t n  = (-r2*r2 + r1*r1 + y2*y2 - y1*y1 + x2*x2 - x1*x1)/(2.0*(y2-y1));
        Double_t p  = (2.0*(-x1 + m*(n-y1)))/(1.0 + m*m);
        Double_t q  = (x1*x1 + (n-y1)*(n-y1) -r1*r1)/(1.0 + m*m);
        Double_t p2 = (p/2.0)*(p/2.0);

        if(p2 >= q)
        {
            x1_c = (-p/2.0) + sqrt(p2 - q);
            x2_c = (-p/2.0) - sqrt(p2 - q);
            y1_c = m*x1_c + n;
            y2_c = m*x2_c + n;
            return 1;
        }
        else return 0;
    }
    else return 0;

} 
//----------------------------------------------------------------------------------------

Int_t Ali_TRD_ST_Analyze::fCircle_Interception(Double_t x1, Double_t y1, Double_t r1, Double_t x2, Double_t y2, Double_t r2)
{
	Double_t dif_x=(x1-x2);
	Double_t dif_y=(y1-y2);
	Double_t dist=TMath::Sqrt( (dif_x*dif_x) +(dif_y*dif_y) );
	Double_t dist_inv =1/dist;
	if (dist==0){};
	
	if (dist<(r1+r2)){
	//2 intersections
		Double_t x_loc 		=((dist*dist +r1*r1 -r2*r2)*(0.5*dist_inv));
		Double_t y_loc		=TMath::Sqrt(r1*r1 -x_loc*x_loc);
		
		Double_t x_glob1	=(x1*dist+x_loc*dif_x -y_loc*dif_y)*dist_inv;
		Double_t y_glob1	=(y1*dist+x_loc*dif_y +y_loc*dif_x)*dist_inv;
		
		Double_t x_glob2	=(x1*dist+x_loc*dif_x +y_loc*dif_y)*dist_inv;
		Double_t y_glob2	=(y1*dist-x_loc*dif_y +y_loc*dif_x)*dist_inv;
		
	}
	else{
	//no intersection (maybe 1)
		Double_t normrad	=r1*dist_inv;
		Double_t px1		=x1+ normrad*dif_x;
		Double_t py1		=y1+ normrad*dif_y;
		
		normrad				=(dist-r2)/dist;
		Double_t px2		=x1+ normrad*dif_x;
		Double_t py2		=y1+ normrad*dif_y;
		
	}	
	
	return 1;
}	

//---------------------------------------------------------------------------------------------------------
pair<Double_t,Double_t> Ali_TRD_ST_Analyze::fpathLength(Double_t r) const
{
	pair<Double_t,Double_t> value;
	pair<Double_t,Double_t> VALUE(999999999.,999999999.);
	Double_t curvature=aliHelix_params[4];
	Double_t radius=1/curvature;
	Double_t x0=aliHelix_params[5] +radius*TMath::Sin(aliHelix_params[2]);
	Double_t y0=aliHelix_params[0] -radius*TMath::Cos(aliHelix_params[2]);
	Double_t z0=aliHelix_params[1];

	Double_t phase=aliHelix_params[2] -TMath::Pi()/2;
	if(phase<0) phase=2*TMath::Pi() -	aliHelix_params[2];
	Double_t cosphase=TMath::Cos(phase);	
	Double_t sinphase=TMath::Sin(phase);
	Double_t dipangle=TMath::ATan(aliHelix_params[3]);
	Double_t cosdipangle=TMath::Cos(dipangle);
	Double_t sindipangle=TMath::Sin(dipangle);
	Double_t h= TMath::Sign(1,aliHelix_params[4]);
	
	Double_t t1 = y0*curvature;
  	
	Double_t t2 = sinphase;
  	Double_t t3 = curvature*curvature;
  	Double_t t4  =y0*t2;
	Double_t t5 = cosphase;
	Double_t t6 = x0*t5;
	Double_t t8 = x0*x0;
	Double_t t11 = x0*x0;
	Double_t t14 = r*r;
	Double_t t15 = t14*curvature;
	Double_t t17 = t8*t8;
	Double_t t19 = t11*t11;
	Double_t t21 = t11*t3;
	Double_t t23 = t5*t5;
	Double_t t32 = t14*t14;
	Double_t t35 = t14*t3;
	Double_t t38 = 8.0*t4*t6 - 4.0*t1*t2*t8 - 4.0*t11*curvature*t6 +
				4.0*t15*t6 + t17*t3 + t19*t3 + 2.0*t21*t8 + 4.0*t8*t23 -
				4.0*t8*x0*curvature*t5 - 4.0*t11*t23 -
				4.0*t11*x0*curvature*t2 + 4.0*t11 - 4.0*t14 +
				t32*t3 + 4.0*t15*t4 - 2.0*t35*t11 - 2.0*t35*t8;
	Double_t t40 = (-t3*t38);
	if (t40<0.) return VALUE;
	t40 = ::sqrt(t40);

	Double_t t43 = x0*curvature;
	Double_t t45 = 2.0*t5 - t35 + t21 + 2.0 - 2.0*t1*t2 -2.0*t43 - 2.0*t43*t5 + t8*t3;
	Double_t t46 = h*cosdipangle*curvature;

	value.first = (-phase + 2.0*atan((-2.0*t1 + 2.0*t2 + t40)/t45))/t46;
	value.second = -(phase + 2.0*atan((2.0*t1 - 2.0*t2 + t40)/t45))/t46;

	//
	//   Solution can be off by +/- one period, select smallest
	//
	Double_t p = fabs(2*TMath::Pi()/(h*curvature*cosdipangle));;
	if (! std::isnan(value.first)) {
		if (fabs(value.first-p) < fabs(value.first)) value.first = value.first-p;
	   	else if (fabs(value.first+p) < fabs(value.first)) value.first = value.first+p;
	}
	if (! std::isnan(value.second)) {
	   	if (fabs(value.second-p) < fabs(value.second)) value.second = value.second-p;
	   	else if (fabs(value.second+p) < fabs(value.second)) value.second = value.second+p;
	}
	
	if (value.first > value.second)
	swap(value.first,value.second);
	return(value);

	
}
//----------------------------------------------------------------------------------------
Int_t Ali_TRD_ST_Analyze::fDCA_Helix_Estimate(Ali_Helix* helixA, Ali_Helix* helixB, Float_t &pathA, Float_t &pathB, Float_t &dcaAB)
{

    // Calculates the 2D crossing point, calculates the corresponding 3D point and returns pathA and pathB

    Double_t helix_point[3];

    Double_t x1 = helixA->getHelix_param(5);
    Double_t y1 = helixA->getHelix_param(0);
    Double_t x2 = helixB->getHelix_param(5);
    Double_t y2 = helixB->getHelix_param(0);
    Double_t c1 = helixA->getHelix_param(4);
    Double_t c2 = helixB->getHelix_param(4);
    Double_t r1 = 0.0;
    Double_t r2 = 0.0;
    if(c1 != 0 && c2 != 0)
    {
        r1 = 1.0/c1;
        r2 = 1.0/c2;
    } else return 0;

    Double_t x1_c = 0.0;
    Double_t y1_c = 0.0;
    Double_t x2_c = 0.0;
    Double_t y2_c = 0.0;

    Int_t bCross_points = fCross_points_Circles(x1,y1,r1,x2,y2,r2,x1_c,y1_c,x2_c,y2_c);

    pathA = 0.0;
    pathB = 0.0;
    dcaAB = 0.0;

    printf("2D circle cross points: {%4.3f, %4.3f}, {%4.3f, %4.3f} \n",x1_c,y1_c,x2_c,y2_c);

    /*
    //cout << "bCross_points = " << bCross_points << ", xyr(1) = {" << x1 << ", " << y1 << ", " << r1
    //    << "}, xyr(2) = {"  << x2 << ", " << y2 << ", " << r2 << "}, p1 = {" << x1_c << ", " << y1_c << "}, p2 = {" << x2_c << ", " << y2_c << "}" << endl;

    if(bCross_points == 0) return 0;

    TVector3 pointA,pointB,pointA1,pointB1,pointA2,pointB2;

    Double_t path_lengthA_c1,path_lengthA_c2,path_lengthB_c1,path_lengthB_c2;

    // first crossing point for helix A
    pair< double, double > path_lengthA = helixA.pathLength(sqrt(x1_c*x1_c+y1_c*y1_c));
    Double_t path_lengthA1 = path_lengthA.first;
    Double_t path_lengthA2 = path_lengthA.second;

    helixA ->Evaluate(path_lengthA1,helix_point);
    pointA1.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
    helixA ->Evaluate(path_lengthA2,helix_point);
    pointA2.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

    if( ((x1_c-pointA1.x())*(x1_c-pointA1.x()) + (y1_c-pointA1.y())*(y1_c-pointA1.y())) <
       ((x1_c-pointA2.x())*(x1_c-pointA2.x()) + (y1_c-pointA2.y())*(y1_c-pointA2.y())))
    {
        path_lengthA_c1 = path_lengthA1;
    }
    else
    {
        path_lengthA_c1 = path_lengthA2;
    }

    // second crossing point for helix A
    path_lengthA = helixA.pathLength(sqrt(x2_c*x2_c+y2_c*y2_c));
    path_lengthA1 = path_lengthA.first;
    path_lengthA2 = path_lengthA.second;

    helixA ->Evaluate(path_lengthA1,helix_point);
    pointA1.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
    helixA ->Evaluate(path_lengthA2,helix_point);
    pointA2.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

    if( ((x2_c-pointA1.x())*(x2_c-pointA1.x()) + (y2_c-pointA1.y())*(y2_c-pointA1.y())) <
       ((x2_c-pointA2.x())*(x2_c-pointA2.x()) + (y2_c-pointA2.y())*(y2_c-pointA2.y())))
    {
        path_lengthA_c2 = path_lengthA1;
    }
    else
    {
        path_lengthA_c2 = path_lengthA2;
    }

    // first crossing point for helix B
    pair< double, double > path_lengthB = helixB.pathLength(sqrt(x1_c*x1_c+y1_c*y1_c));
    Double_t path_lengthB1 = path_lengthB.first;
    Double_t path_lengthB2 = path_lengthB.second;

    helixB ->Evaluate(path_lengthB1,helix_point);
    pointB1.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
    helixB ->Evaluate(path_lengthB2,helix_point);
    pointB2.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

    if( ((x1_c-pointB1.x())*(x1_c-pointB1.x()) + (y1_c-pointB1.y())*(y1_c-pointB1.y())) <
       ((x1_c-pointB2.x())*(x1_c-pointB2.x()) + (y1_c-pointB2.y())*(y1_c-pointB2.y())))
    {
        path_lengthB_c1 = path_lengthB1;
    }
    else
    {
        path_lengthB_c1 = path_lengthB2;
    }

    // second crossing point for helix B
    path_lengthB = helixB.pathLength(sqrt(x2_c*x2_c+y2_c*y2_c));
    path_lengthB1 = path_lengthB.first;
    path_lengthB2 = path_lengthB.second;

    helixB ->Evaluate(path_lengthB1,helix_point);
    pointB1.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
    helixB ->Evaluate(path_lengthB2,helix_point);
    pointB2.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

    if( ((x2_c-pointB1.x())*(x2_c-pointB1.x()) + (y2_c-pointB1.y())*(y2_c-pointB1.y())) <
       ((x2_c-pointB2.x())*(x2_c-pointB2.x()) + (y2_c-pointB2.y())*(y2_c-pointB2.y())))
    {
        path_lengthB_c2 = path_lengthB1;
    }
    else
    {
        path_lengthB_c2 = path_lengthB2;
    }

    helixA ->Evaluate(path_lengthA_c1,helix_point);
    pointA1.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
    helixA ->Evaluate(path_lengthA_c2,helix_point);
    pointA2.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

    helixB ->Evaluate(path_lengthB_c1,helix_point);
    pointB1.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
    helixB ->Evaluate(path_lengthB_c2,helix_point);
    pointB2.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);


    if((pointA1-pointB1).mag() < (pointA2-pointB2).mag())
    {
        pathA = path_lengthA_c1;
        pathB = path_lengthB_c1;
        dcaAB = (pointA1-pointB1).mag();
    }
    else
    {
        pathA = path_lengthA_c2;
        pathB = path_lengthB_c2;
        dcaAB = (pointA2-pointB2).mag();
    }

    */
    return 1;

}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
// V1.0
void Ali_TRD_ST_Analyze::fHelixABdca(Ali_Helix* helixA, Ali_Helix* helixB, Float_t &pathA, Float_t &pathB, Float_t &dcaAB)
{
    //cout << "Standard fHelixABdca called..." << endl;
    Float_t pA[2] = {0.0,0.0};
    Float_t pB[2] = {0.0,-70.0}; // the two start values for pathB, 0.0 is the origin of the helix at the first measured point
    Float_t distarray[2];
    TVector3 testA, testB;
    Double_t helix_point[3];
    for(Int_t r = 0; r < 2; r++)
    {
        helixB ->Evaluate(pB[r],helix_point);  // 3D-vector of helixB point at path pB[r]
        testB.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

        Float_t pathA_dca = -999.0;
        Float_t dcaAB_dca = -999.0;
        fHelixAtoPointdca(testB,helixA,pathA_dca,dcaAB_dca); // new helix to point dca calculation

        helixA ->Evaluate(pathA_dca,helix_point);  // 3D-vector of helixB point at path pB[r]
        testA.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

        distarray[r] = (testA-testB).Mag(); // dca between helixA and helixB
    }
    Int_t loopcounter = 0;
    Float_t scale = 1.0;
    Float_t flip  = 1.0; // checks if the minimization direction changed
    Float_t scale_length = 100.0;
    while(fabs(scale_length) > 0.05 && loopcounter < 100) // stops when the length is too small
    {
        //cout << "n = " << loopcounter << ", pB[0] = " << pB[0]
        //    << ", pB[1] = " << pB[1] << ", d[0] = " << distarray[0]
        //    << ", d[1] = " << distarray[1] << ", flip = " << flip
        //    << ", scale_length = " << scale_length << endl;
        if(distarray[0] > distarray[1])
        {
            if(loopcounter != 0)
            {
                if(flip == 1.0) scale = 0.4; // if minimization direction changes -> go back, but only the way * 0.4
                else scale = 0.7; // go on in this direction but only by the way * 0.7
            }
            scale_length = (pB[1]-pB[0])*scale; // the next length interval
            pB[0]     = pB[1] + scale_length; // the new path
            helixB ->Evaluate(pB[0],helix_point);  // 3D-vector of helixB point at path pB[r]
            testB.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

            Float_t pathA_dca = -999.0;
            Float_t dcaAB_dca = -999.0;
            fHelixAtoPointdca(testB,helixA,pathA_dca,dcaAB_dca); // new helix to point dca calculation
            pA[0] = pathA_dca;
            //pA[0]     = helixA.pathLength(testB); // pathA at dca to helixB

            helixA ->Evaluate(pA[0],helix_point);  // 3D-vector of helixB point at path pB[r]
            testA.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
            distarray[0] = (testA-testB).Mag(); // new dca
            flip = 1.0;
        }
        else
        {
            if(loopcounter != 0)
            {
                if(flip == -1.0) scale = 0.4;
                else scale = 0.7;
            }
            scale_length = (pB[0]-pB[1])*scale;
            pB[1]     = pB[0] + scale_length;
            helixB ->Evaluate(pB[1],helix_point);  // 3D-vector of helixB point at path pB[r]
            testB.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);

            Float_t pathA_dca = -999.0;
            Float_t dcaAB_dca = -999.0;
            fHelixAtoPointdca(testB,helixA,pathA_dca,dcaAB_dca); // new helix to point dca calculation
            pA[1] = pathA_dca;
            //pA[1]     = helixA.pathLength(testB); // pathA at dca to helixB

            helixA ->Evaluate(pA[1],helix_point);  // 3D-vector of helixB point at path pB[r]
            testA.SetXYZ(helix_point[0],helix_point[1],helix_point[2]);
            distarray[1] = (testA-testB).Mag();
            flip = -1.0;
        }
        loopcounter++;
    }
    if(distarray[0] < distarray[1])
    {
        pathB = pB[0];
        pathA = pA[0];
        dcaAB = distarray[0];
    }
    else
    {
        pathB = pB[1];
        pathA = pA[1];
        dcaAB = distarray[1];
    }
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Calculate_secondary_vertices(Int_t graphics)
{
    Double_t helix_pointA[3];
    Double_t helix_pointB[3];
    Double_t vertex_point[3];

    if(graphics) TEveP_sec_vertices = new TEvePointSet();

    Int_t i_vertex = 0;
    printf("N_helices: %d \n",(Int_t)vec_helices.size());
    for(Int_t i_track_A = 0; i_track_A < ((Int_t)vec_helices.size() - 1); i_track_A++)
    {
        for(Int_t i_track_B = (i_track_A+1); i_track_B < (Int_t)vec_helices.size(); i_track_B++)
        {

            Float_t pathA, pathB, dcaAB;
            fHelixABdca(vec_helices[i_track_A],vec_helices[i_track_B],pathA,pathB,dcaAB);
            //printf("track A,B: {%d, %d}, dcaAB: %4.3f \n",i_track_A,i_track_B,dcaAB);
            if(dcaAB < 10.0)
            {
                vec_helices[i_track_A] ->Evaluate(pathA,helix_pointA);
                vec_helices[i_track_B] ->Evaluate(pathB,helix_pointB);
                for(Int_t i_xyz = 0; i_xyz < 3; i_xyz++)
                {
                    vertex_point[i_xyz] = (helix_pointA[i_xyz] + helix_pointB[i_xyz])/2.0;
                }

                Float_t pathA_est, pathB_est, dcaAB_est;
                fDCA_Helix_Estimate(vec_helices[i_track_A],vec_helices[i_track_B],pathA_est,pathB_est,dcaAB_est);
                printf("3D cross point: {%4.3f, %4.3f, %4.3f} \n",vertex_point[0],vertex_point[1],vertex_point[2]);

                Double_t radius_vertex = TMath::Sqrt(vertex_point[0]*vertex_point[0] + vertex_point[1]*vertex_point[1]);
                if(radius_vertex > 240.0 && radius_vertex < 360.0)
                {
                    if(graphics) TEveP_sec_vertices ->SetPoint(i_vertex,vertex_point[0],vertex_point[1],vertex_point[2]);
                    i_vertex++;
                }
            }
        }
    }

    printf("Number of secondary vertices found: %d \n",i_vertex);

    if(graphics)
    {
        TEveP_sec_vertices  ->SetMarkerSize(3);
        TEveP_sec_vertices  ->SetMarkerStyle(20);
        TEveP_sec_vertices  ->SetMarkerColor(kRed);

        gEve->AddElement(TEveP_sec_vertices);
        gEve->Redraw3D(kTRUE);
    }
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::set_Kalman_helix_params(vector<vector<Double_t>> mHelices_kalman_in)
{
    //printf("Ali_TRD_ST_Analyze::set_Kalman_helix_params");
    mHelices_kalman = mHelices_kalman_in;

    vec_helices.clear();
    vec_helices.resize((Int_t)mHelices_kalman.size());

    for(Int_t i_track = 0; i_track < (Int_t)mHelices_kalman.size(); i_track++)
    {
        vec_helices[i_track] = new Ali_Helix();
        vec_helices[i_track] ->setHelix(mHelices_kalman[i_track][0],mHelices_kalman[i_track][1],mHelices_kalman[i_track][2],mHelices_kalman[i_track][3],mHelices_kalman[i_track][4],mHelices_kalman[i_track][5]);
    }

}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::set_single_helix_params(vector<Double_t> vec_params)
{
    for(Int_t i_param = 0; i_param < 6; i_param++)
    {
        aliHelix_params[i_param] = vec_params[i_param];
    }
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Evaluate(Double_t t,Double_t r[3])  //radius vector
{
  //--------------------------------------------------------------------
  // Calculate position of a point on a track and some derivatives at given phase
  //--------------------------------------------------------------------
  float phase=aliHelix_params[4]*t+aliHelix_params[2];
  Double_t sn=sinf(phase), cs=cosf(phase);
  //  Double_t sn=TMath::Sin(phase), cs=TMath::Cos(phase);

  r[0] = aliHelix_params[5] + sn/aliHelix_params[4];
  r[1] = aliHelix_params[0] - cs/aliHelix_params[4];
  r[2] = aliHelix_params[1] + aliHelix_params[3]*t;
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Draw_Kalman_Helix_Tracks()
{
    for(Int_t i_track = 0; i_track < (Int_t)mHelices_kalman.size(); i_track++)
    {
        set_single_helix_params(mHelices_kalman[i_track]);
        Double_t track_pos[3];

        vec_TPL3D_helix_kalman.resize(i_track+1);
        vec_TPL3D_helix_kalman_hull.resize(i_track+1);
        vec_TPL3D_helix_kalman_inner.resize(i_track+1);
        vec_TPL3D_helix_kalman[i_track] = new TEveLine();
        vec_TPL3D_helix_kalman_hull[i_track] = new TEveLine();
        vec_TPL3D_helix_kalman_inner[i_track] = new TEveLine();

        Double_t radius_helix = 0.0;
        for(Double_t track_path = -300.0; track_path < 1000; track_path += 1.0)
        {
            Evaluate(track_path,track_pos);
            radius_helix = TMath::Sqrt( TMath::Power(track_pos[0],2) + TMath::Power(track_pos[1],2) );
			if (!(track_path))
            printf("i_track: %d, track_path: %4.3f, pos: {%4.3f, %4.3f, %4.3f}, radius_helix: %4.3f \n",i_track,track_path,track_pos[0],track_pos[1],track_pos[2],radius_helix);
            if(radius_helix > 370.0) break;
            if(fabs(track_pos[2]) > 320.0) break;
            if(radius_helix > 80.0)
            {
                vec_TPL3D_helix_kalman[i_track]        ->SetNextPoint(track_pos[0],track_pos[1],track_pos[2]);
                vec_TPL3D_helix_kalman_hull[i_track]   ->SetNextPoint(track_pos[0],track_pos[1],track_pos[2]);
            }
        }

        HistName = "track kalman ";
        HistName += i_track;
        vec_TPL3D_helix_kalman[i_track]    ->SetName(HistName.Data());
        vec_TPL3D_helix_kalman[i_track]    ->SetLineStyle(1);
        vec_TPL3D_helix_kalman[i_track]    ->SetLineWidth(3);
        vec_TPL3D_helix_kalman[i_track]    ->SetMainColor(kGreen);
        vec_TPL3D_helix_kalman[i_track]    ->SetMainAlpha(1.0);

        HistName = "track kalman (h) ";
        HistName += i_track;
        vec_TPL3D_helix_kalman_hull[i_track]    ->SetName(HistName.Data());
        vec_TPL3D_helix_kalman_hull[i_track]    ->SetLineStyle(1);
        vec_TPL3D_helix_kalman_hull[i_track]    ->SetLineWidth(8);
        vec_TPL3D_helix_kalman_hull[i_track]    ->SetMainColor(kOrange);
        vec_TPL3D_helix_kalman_hull[i_track]    ->SetMainAlpha(0.3);

        gEve->AddElement(vec_TPL3D_helix_kalman[i_track]);
        gEve->AddElement(vec_TPL3D_helix_kalman_hull[i_track]);
    }

    gEve->Redraw3D(kTRUE);
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Init_tree(TString SEList)
{
    printf("Ali_TRD_ST_Analyze::Init_tree \n");
    TString pinputdir = "./Data/";

    TRD_ST_Tracklet   = new Ali_TRD_ST_Tracklets();
    TRD_ST_TPC_Track  = new Ali_TRD_ST_TPC_Track();
    TRD_ST_Event      = new Ali_TRD_ST_Event();

    // Same event input
    if (!SEList.IsNull())   // if input file is ok
    {
        cout << "Open same event file list " << SEList << endl;
        ifstream in(SEList);  // input stream
        if(in)
        {
            cout << "file list is ok" << endl;
            input_SE  = new TChain( TRD_ST_TREE.Data(), TRD_ST_TREE.Data() );
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
                    input_SE ->AddFile(addfile.Data(),-1, TRD_ST_TREE.Data() );
                    Long64_t file_entries = input_SE->GetEntries();
                    cout << "File added to data chain: " << addfile.Data() << " with " << (file_entries-entries_save) << " entries" << endl;
                    entries_save = file_entries;
                }
            }
            input_SE  ->SetBranchAddress( TRD_ST_BRANCH, &TRD_ST_Event );
        }
        else
        {
            cout << "WARNING: SE file input is problemtic" << endl;
        }
    }

    file_entries_total = input_SE->GetEntries();
    N_Events = file_entries_total;
    cout << "Total number of events in tree: " << file_entries_total << endl;
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Loop_event(Long64_t i_event)
{
    //printf("Ali_TRD_ST_Analyze::Loop_event \n");

    if (!input_SE->GetEntry( i_event )) return 0; // take the event -> information is stored in event


    //--------------------------------------------------
    // Event information (more data members available, see Ali_TRD_ST_Event class definition)
    UShort_t NumTracks            = TRD_ST_Event ->getNumTracks(); // number of tracks in this event
    Int_t    NumTracklets         = TRD_ST_Event ->getNumTracklets();
    Double_t EventVertexX         = TRD_ST_Event ->getx();
    Double_t EventVertexY         = TRD_ST_Event ->gety();
    Double_t EventVertexZ         = TRD_ST_Event ->getz();
    Float_t  V0MEq                = TRD_ST_Event ->getcent_class_V0MEq();
    //--------------------------------------------------

    printf("Event: %lld, TPC tracks: %d, TRD tracklets: %d \n",i_event,NumTracks,NumTracklets);

    //--------------------------------------------------
    // TPC track loop
    for(Int_t i_track = 0; i_track < NumTracks; i_track++)
    {
        TRD_ST_TPC_Track = TRD_ST_Event ->getTrack(i_track);

        Double_t nsigma_TPC_e   = TRD_ST_TPC_Track ->getnsigma_e_TPC();
        Double_t nsigma_TPC_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TPC();
        Double_t nsigma_TPC_p   = TRD_ST_TPC_Track ->getnsigma_p_TPC();
        Double_t nsigma_TOF_e   = TRD_ST_TPC_Track ->getnsigma_e_TOF();
        Double_t nsigma_TOF_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TOF();
        Double_t TRD_signal     = TRD_ST_TPC_Track ->getTRDSignal();
        Double_t TRDsumADC      = TRD_ST_TPC_Track ->getTRDsumADC();
        Double_t dca            = TRD_ST_TPC_Track ->getdca();  // charge * distance of closest approach to the primary vertex
        TLorentzVector TLV_part = TRD_ST_TPC_Track ->get_TLV_part();
        UShort_t NTPCcls        = TRD_ST_TPC_Track ->getNTPCcls();
        UShort_t NTRDcls        = TRD_ST_TPC_Track ->getNTRDcls();
        UShort_t NITScls        = TRD_ST_TPC_Track ->getNITScls();
        Float_t TPCchi2         = TRD_ST_TPC_Track ->getTPCchi2();
        Float_t TPCdEdx         = TRD_ST_TPC_Track ->getTPCdEdx();
        Float_t TOFsignal       = TRD_ST_TPC_Track ->getTOFsignal(); // in ps (1E-12 s)
        Float_t Track_length    = TRD_ST_TPC_Track ->getTrack_length();

        Float_t momentum        = TLV_part.P();
        Float_t eta_track       = TLV_part.Eta();
        Float_t pT_track        = TLV_part.Pt();
        Float_t theta_track     = TLV_part.Theta();
        Float_t phi_track       = TLV_part.Phi();
    }
    //--------------------------------------------------



    //--------------------------------------------------
    // TRD tracklet loop
    TVector3 TV3_offset;
    TVector3 TV3_dir;
    Int_t    i_det;
	
	Tracklets=new Ali_TRD_ST_Tracklets*[NumTracklets];
    Number_Tracklets=NumTracklets;
	
    for(Int_t i_tracklet = 0; i_tracklet < NumTracklets; i_tracklet++)
    {
        TRD_ST_Tracklet = TRD_ST_Event    ->getTracklet(i_tracklet);
        TRD_ST_Tracklet->set_TRD_index(i_tracklet);
        Tracklets[i_tracklet]=TRD_ST_Tracklet;
        TV3_offset      = TRD_ST_Tracklet ->get_TV3_offset();
        TV3_dir         = TRD_ST_Tracklet ->get_TV3_dir();
        i_det           = TRD_ST_Tracklet ->get_TRD_det();
		
        //create "tracklets"
        Int_t i_sector = (Int_t)(i_det/30);
        Int_t i_stack  = (Int_t)(i_det%30/6);
        Int_t i_layer  = i_det%6;
        //Int_t i_det = layer + 6*stack + 30*sector;

        Double_t radius = TMath::Sqrt( TMath::Power(TV3_offset[0],2) + TMath::Power(TV3_offset[1],2) );

        th1d_TRD_layer_radii ->Fill(radius);
    }
    //--------------------------------------------------


    //TCanvas* can_TRD_layer_radii = new TCanvas("can_TRD_layer_radii","can_TRD_layer_radii",10,10,500,500);
    //can_TRD_layer_radii ->cd();
    //th1d_TRD_layer_radii ->DrawCopy("h");
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Draw_event(Long64_t i_event)
{
    //printf("Ali_TRD_ST_Analyze::Draw_event \n");
	
    if (!input_SE->GetEntry( i_event )) return 0; // take the event -> information is stored in event

	
    //--------------------------------------------------
    // Event information (more data members available, see Ali_TRD_ST_Event class definition)
    UShort_t NumTracks            = TRD_ST_Event ->getNumTracks(); // number of tracks in this event
    Int_t    NumTracklets         = TRD_ST_Event ->getNumTracklets();
    Double_t EventVertexX         = TRD_ST_Event ->getx();
    Double_t EventVertexY         = TRD_ST_Event ->gety();
    Double_t EventVertexZ         = TRD_ST_Event ->getz();
    //--------------------------------------------------


    //--------------------------------------------------
    // TPC track loop
    Double_t track_pos[3];
    Double_t radius_helix;
    for(Int_t i_track = 0; i_track < NumTracks; i_track++)
    {
        TRD_ST_TPC_Track = TRD_ST_Event ->getTrack(i_track);

        Double_t nsigma_TPC_e   = TRD_ST_TPC_Track ->getnsigma_e_TPC();
        Double_t nsigma_TPC_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TPC();
        Double_t nsigma_TPC_p   = TRD_ST_TPC_Track ->getnsigma_p_TPC();
        Double_t nsigma_TOF_e   = TRD_ST_TPC_Track ->getnsigma_e_TOF();
        Double_t nsigma_TOF_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TOF();
        Double_t TRD_signal     = TRD_ST_TPC_Track ->getTRDSignal();
        Double_t TRDsumADC      = TRD_ST_TPC_Track ->getTRDsumADC();
        Double_t dca            = TRD_ST_TPC_Track ->getdca();  // charge * distance of closest approach to the primary vertex
        TLorentzVector TLV_part = TRD_ST_TPC_Track ->get_TLV_part();
        UShort_t NTPCcls        = TRD_ST_TPC_Track ->getNTPCcls();
        UShort_t NTRDcls        = TRD_ST_TPC_Track ->getNTRDcls();
        UShort_t NITScls        = TRD_ST_TPC_Track ->getNITScls();
        Float_t TPCchi2         = TRD_ST_TPC_Track ->getTPCchi2();
        Float_t TPCdEdx         = TRD_ST_TPC_Track ->getTPCdEdx();
        Float_t TOFsignal       = TRD_ST_TPC_Track ->getTOFsignal(); // in ps (1E-12 s)
        Float_t Track_length    = TRD_ST_TPC_Track ->getTrack_length();
		
        Float_t momentum        = TLV_part.P();
        Float_t eta_track       = TLV_part.Eta();
        Float_t pT_track        = TLV_part.Pt();
        Float_t theta_track     = TLV_part.Theta();
        Float_t phi_track       = TLV_part.Phi();

        if(momentum < 0.3) continue;

        vec_TPL3D_helix.resize(i_track+1);
        vec_TPL3D_helix_hull.resize(i_track+1);
        vec_TPL3D_helix_inner.resize(i_track+1);
        vec_TPL3D_helix[i_track] = new TEveLine();
        vec_TPL3D_helix_hull[i_track] = new TEveLine();
        vec_TPL3D_helix_inner[i_track] = new TEveLine();
		cout<<i_track<<": ";
		for(Int_t i_param=0;i_param<6;i_param++)
			cout<<TRD_ST_TPC_Track ->getHelix_param(i_param)<<" ";
		cout<<endl;
        for(Double_t track_path = 0.0; track_path < 1000; track_path += 1.0)
        {
            TRD_ST_TPC_Track ->Evaluate(track_path,track_pos);
            radius_helix = TMath::Sqrt( TMath::Power(track_pos[0],2) + TMath::Power(track_pos[1],2) );
            if(radius_helix > 300.0) break;
            if(fabs(track_pos[2]) > 320.0) break;
            if(radius_helix > 80.0)
            {
                vec_TPL3D_helix[i_track]        ->SetNextPoint(track_pos[0],track_pos[1],track_pos[2]);
                vec_TPL3D_helix_hull[i_track]   ->SetNextPoint(track_pos[0],track_pos[1],track_pos[2]);
            }
            if(radius_helix < 80.0)
            {
                vec_TPL3D_helix_inner[i_track] ->SetNextPoint(track_pos[0],track_pos[1],track_pos[2]);
            }


            //if(i_track == 0) printf("track_path: %4.3f, pos: {%4.2f, %4.2f, %4.2f} \n",track_path,track_pos[0],track_pos[1],track_pos[2]);
        }

        HistName = "track ";
        HistName += i_track;
        vec_TPL3D_helix[i_track]    ->SetName(HistName.Data());
        vec_TPL3D_helix[i_track]    ->SetLineStyle(1);
        vec_TPL3D_helix[i_track]    ->SetLineWidth(3);
        vec_TPL3D_helix[i_track]    ->SetMainColor(track_color);
        vec_TPL3D_helix[i_track]    ->SetMainAlpha(1.0);

        HistName = "track (h) ";
        HistName += i_track;
        vec_TPL3D_helix_hull[i_track]    ->SetName(HistName.Data());
        vec_TPL3D_helix_hull[i_track]    ->SetLineStyle(1);
        vec_TPL3D_helix_hull[i_track]    ->SetLineWidth(8);
        vec_TPL3D_helix_hull[i_track]    ->SetMainColor(kWhite);
        vec_TPL3D_helix_hull[i_track]    ->SetMainAlpha(0.3);

        HistName = "track (h) ";
        HistName += i_track;
        vec_TPL3D_helix_inner[i_track]    ->SetName(HistName.Data());
        vec_TPL3D_helix_inner[i_track]    ->SetLineStyle(1);
        vec_TPL3D_helix_inner[i_track]    ->SetLineWidth(2);
        vec_TPL3D_helix_inner[i_track]    ->SetMainColor(kGray);
        vec_TPL3D_helix_inner[i_track]    ->SetMainAlpha(0.8);
        //if(i_track == 3)
        {
            gEve->AddElement(vec_TPL3D_helix[i_track]);
            gEve->AddElement(vec_TPL3D_helix_hull[i_track]);
            gEve->AddElement(vec_TPL3D_helix_inner[i_track]);
        }
    }

    //--------------------------------------------------



    //--------------------------------------------------
    // TRD tracklet loop
    TVector3 TV3_offset;
    TVector3 TV3_dir;
    Int_t    i_det;

    for(Int_t i_tracklet = 0; i_tracklet < NumTracklets; i_tracklet++)
    {
        TRD_ST_Tracklet = TRD_ST_Event    ->getTracklet(i_tracklet);
        TV3_offset      = TRD_ST_Tracklet ->get_TV3_offset();
        TV3_dir         = TRD_ST_Tracklet ->get_TV3_dir();
        i_det           = TRD_ST_Tracklet ->get_TRD_det();

        if(TV3_offset.Mag() > 1000.0) continue;

        //create "tracklets"
        Int_t i_sector = (Int_t)(i_det/30);
        Int_t i_stack  = (Int_t)(i_det%30/6);
        Int_t i_layer  = i_det%6;
        //Int_t i_det = layer + 6*stack + 30*sector;

        vec_TEveLine_tracklets[i_layer].resize(N_tracklets_layers[i_layer]+1);
        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]] = new TEveLine();

        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]] ->SetNextPoint(TV3_offset[0],TV3_offset[1],TV3_offset[2]);
        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]] ->SetNextPoint(TV3_offset[0] + scale_length*TV3_dir[0],TV3_offset[1] + scale_length*TV3_dir[1],TV3_offset[2] + scale_length*TV3_dir[2]);

        Double_t radius = TMath::Sqrt( TMath::Power(TV3_offset[0],2) + TMath::Power(TV3_offset[1],2) );
        //printf("i_tracklet: %d, radius: %4.3f, pos A: {%4.2f, %4.2f, %4.2f}, pos B: {%4.2f, %4.2f, %4.2f} \n",i_tracklet,radius,TV3_offset[0],TV3_offset[1],TV3_offset[2],TV3_offset[0] + scale_length*TV3_dir[0],TV3_offset[1] + scale_length*TV3_dir[1],TV3_offset[2] + scale_length*TV3_dir[2]);

        HistName = "tracklet ";
        HistName += i_tracklet;
        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]]    ->SetName(HistName.Data());
        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]]    ->SetLineStyle(1);
        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]]    ->SetLineWidth(3);
        vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]]    ->SetMainColor(color_layer[i_layer]);
        //if(i_tracklet == 63 || i_tracklet == 67 || i_tracklet == 72 || i_tracklet == 75 || i_tracklet == 83 || i_tracklet == 88)
        {
             gEve->AddElement(vec_TEveLine_tracklets[i_layer][N_tracklets_layers[i_layer]]);
        }

        N_tracklets_layers[i_layer]++;
    }

    gEve->Redraw3D(kTRUE);
    //--------------------------------------------------
}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Do_TPC_TRD_matching(Long64_t i_event, Double_t xy_matching_window, Double_t z_matching_window)
{
    //printf("Ali_TRD_ST_Analyze::Do_TPC_TRD_matching \n");

    if (!input_SE->GetEntry( i_event )) return 0; // take the event -> information is stored in event


    //--------------------------------------------------
    // Event information (more data members available, see Ali_TRD_ST_Event class definition)
    UShort_t NumTracks            = TRD_ST_Event ->getNumTracks(); // number of tracks in this event
    Int_t    NumTracklets         = TRD_ST_Event ->getNumTracklets();
    Double_t EventVertexX         = TRD_ST_Event ->getx();
    Double_t EventVertexY         = TRD_ST_Event ->gety();
    Double_t EventVertexZ         = TRD_ST_Event ->getz();
    Float_t  V0MEq                = TRD_ST_Event ->getcent_class_V0MEq();
    //--------------------------------------------------



    //--------------------------------------------------
    // Store all TRD tracklet in an array to keep it in memory
    // TRD tracklet loop
    vector< vector<TVector3> > vec_TV3_dir_tracklets;
    vector< vector<TVector3> > vec_TV3_offset_tracklets;
    vector< vector<Int_t> > TRD_index_tracklets;
    vec_TV3_dir_tracklets.resize(540);
    vec_TV3_offset_tracklets.resize(540);
	TRD_index_tracklets.resize(540);
	
    Double_t ADC_val[24];

    TVector3 TV3_offset;
    TVector3 TV3_dir;
    Int_t    i_det;

    for(Int_t i_tracklet = 0; i_tracklet < NumTracklets; i_tracklet++)
    {
        TRD_ST_Tracklet = TRD_ST_Event    ->getTracklet(i_tracklet);
        TV3_offset      = TRD_ST_Tracklet ->get_TV3_offset();
        TV3_dir         = TRD_ST_Tracklet ->get_TV3_dir();
        i_det           = TRD_ST_Tracklet ->get_TRD_det();

        //create "tracklets"
        Int_t i_sector = (Int_t)(i_det/30);
        Int_t i_stack  = (Int_t)(i_det%30/6);
        Int_t i_layer  = i_det%6;
        //Int_t i_det = layer + 6*stack + 30*sector;

        vec_TV3_dir_tracklets[i_det].push_back(TV3_dir);
        vec_TV3_offset_tracklets[i_det].push_back(TV3_offset);
		TRD_index_tracklets[i_det].push_back(i_tracklet);
        
    }
    //--------------------------------------------------

    //--------------------------------------------------
    // TPC track loop
    Double_t track_pos[3];
    Double_t radius_helix;
    for(Int_t i_track = 0; i_track < NumTracks; i_track++)
    //for(Int_t i_track = 3; i_track < 4; i_track++)
    {
        TRD_ST_TPC_Track = TRD_ST_Event ->getTrack(i_track);

        Double_t nsigma_TPC_e   = TRD_ST_TPC_Track ->getnsigma_e_TPC();
        Double_t nsigma_TPC_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TPC();
        Double_t nsigma_TPC_p   = TRD_ST_TPC_Track ->getnsigma_p_TPC();
        Double_t nsigma_TOF_e   = TRD_ST_TPC_Track ->getnsigma_e_TOF();
        Double_t nsigma_TOF_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TOF();
        Double_t TRD_signal     = TRD_ST_TPC_Track ->getTRDSignal();
        Double_t TRDsumADC      = TRD_ST_TPC_Track ->getTRDsumADC();
        Double_t dca            = TRD_ST_TPC_Track ->getdca();  // charge * distance of closest approach to the primary vertex
        TLorentzVector TLV_part = TRD_ST_TPC_Track ->get_TLV_part();
        UShort_t NTPCcls        = TRD_ST_TPC_Track ->getNTPCcls();
        UShort_t NTRDcls        = TRD_ST_TPC_Track ->getNTRDcls();
        UShort_t NITScls        = TRD_ST_TPC_Track ->getNITScls();
        Float_t TPCchi2         = TRD_ST_TPC_Track ->getTPCchi2();
        Float_t TPCdEdx         = TRD_ST_TPC_Track ->getTPCdEdx();
        Float_t TOFsignal       = TRD_ST_TPC_Track ->getTOFsignal(); // in ps (1E-12 s)
        Float_t Track_length    = TRD_ST_TPC_Track ->getTrack_length();

        Float_t momentum        = TLV_part.P();
        Float_t eta_track       = TLV_part.Eta();
        Float_t pT_track        = TLV_part.Pt();
        Float_t theta_track     = TLV_part.Theta();
        Float_t phi_track       = TLV_part.Phi();

        if(momentum < 0.3) continue;

        vector<TVector3> vec_TV3_helix_points_at_TRD_layers;
        vec_TV3_helix_points_at_TRD_layers.resize(6);
		matched_tracks.resize(matched_tracks.size() +1);
        

        for(Int_t i_layer = 0; i_layer < 6; i_layer++)
        {
            Double_t radius_layer_center = 0.5*(TRD_layer_radii[i_layer][0] + TRD_layer_radii[i_layer][1]);

            // Find the helix path which touches the first TRD layer
            Double_t track_path_add      = 10.0;
            Double_t track_path_layer0   = 0.0;
            Double_t radius_helix_layer0 = 0.0;
            for(Double_t track_path = TRD_layer_radii[i_layer][0]; track_path < 1000; track_path += track_path_add)
            {
                TRD_ST_TPC_Track ->Evaluate(track_path,track_pos);
                radius_helix = TMath::Sqrt( TMath::Power(track_pos[0],2) + TMath::Power(track_pos[1],2) );
                if(radius_helix < radius_layer_center)
                {
                    track_path_layer0   = track_path;
                    radius_helix_layer0 = radius_helix;
                    //printf("radius_helix_layer0: %4.3f, track_path_add: %4.3f \n",radius_helix_layer0,track_path_add);
                }
                else
                {
                    track_path -= track_path_add;
                    track_path_add *= 0.5;
                }
                if(track_path_add < 1.0)
                {
                    vec_TV3_helix_points_at_TRD_layers[i_layer].SetXYZ(track_pos[0],track_pos[1],track_pos[2]);
                    break;
                }
            }
            //printf("   --> i_layer: %d, track_path_layer0: %4.3f, radius_helix_layer0: %4.3f \n",i_layer,track_path_layer0,radius_helix_layer0);


            TVector3 TV3_diff_vec;
            Double_t dist_min      = 10.0;
            Double_t dist          = 0.0;
            Int_t    det_best      = -1;
            Int_t    tracklet_best = -1;
		
            for(Int_t i_det = 0; i_det < 540; i_det++)
            {
                Int_t i_layer_from_det  = i_det%6;
                if(i_layer_from_det != i_layer) continue;

                for(Int_t i_tracklet = 0; i_tracklet < (Int_t)vec_TV3_offset_tracklets[i_det].size(); i_tracklet++)
                {
                    TV3_diff_vec = vec_TV3_offset_tracklets[i_det][i_tracklet] - vec_TV3_helix_points_at_TRD_layers[i_layer];
                    dist = TV3_diff_vec.Mag();
                    if(dist < dist_min)
                    {
                        dist_min      = dist;
                        det_best      = i_det;
                        tracklet_best = i_tracklet;
                    }
                }
            }
			Int_t tracks_size=matched_tracks.size()-1;
			matched_tracks[tracks_size].push_back(NULL);
            
            if(det_best < 0 || tracklet_best < 0) continue;
			
			Ali_TRD_ST_Tracklets* temp_tracklet=new Ali_TRD_ST_Tracklets();
			temp_tracklet->set_TRD_det(det_best);
			temp_tracklet->set_TV3_offset(vec_TV3_offset_tracklets[det_best][tracklet_best]);
			temp_tracklet->set_TV3_dir(vec_TV3_dir_tracklets[det_best][tracklet_best]);
			temp_tracklet->set_TRD_index(TRD_index_tracklets[det_best][tracklet_best]);	
			matched_tracks[tracks_size][matched_tracks[tracks_size].size()-1]=temp_tracklet;
			
			
            Int_t size_tracklet = (Int_t)vec_TEveLine_tracklets_match[i_layer].size();

            //printf("i_layer: %d, size_tracklet: %d \n",i_layer,size_tracklet);

            vec_TEveLine_tracklets_match[i_layer].resize(size_tracklet+1);
            vec_TEveLine_tracklets_match[i_layer][size_tracklet] = new TEveLine();
            vec_TEveLine_tracklets_match[i_layer][size_tracklet] ->SetNextPoint(vec_TV3_offset_tracklets[det_best][tracklet_best][0],vec_TV3_offset_tracklets[det_best][tracklet_best][1],vec_TV3_offset_tracklets[det_best][tracklet_best][2]);
            vec_TEveLine_tracklets_match[i_layer][size_tracklet] ->SetNextPoint(vec_TV3_offset_tracklets[det_best][tracklet_best][0] + scale_length*vec_TV3_dir_tracklets[det_best][tracklet_best][0],vec_TV3_offset_tracklets[det_best][tracklet_best][1] + scale_length*vec_TV3_dir_tracklets[det_best][tracklet_best][1],vec_TV3_offset_tracklets[det_best][tracklet_best][2] + scale_length*vec_TV3_dir_tracklets[det_best][tracklet_best][2]);

            HistName = "tracklet (m) ";
            HistName += size_tracklet;
            vec_TEveLine_tracklets_match[i_layer][size_tracklet]    ->SetName(HistName.Data());
            vec_TEveLine_tracklets_match[i_layer][size_tracklet]    ->SetLineStyle(1);
            vec_TEveLine_tracklets_match[i_layer][size_tracklet]    ->SetLineWidth(6);
            vec_TEveLine_tracklets_match[i_layer][size_tracklet]    ->SetMainColor(color_layer_match[i_layer]);

            //if(i_tracklet == 63 || i_tracklet == 67 || i_tracklet == 72 || i_tracklet == 75 || i_tracklet == 83 || i_tracklet == 88)
            {
                 gEve->AddElement(vec_TEveLine_tracklets_match[i_layer][size_tracklet]);
            }
        }
    }
    //--------------------------------------------------

    gEve->Redraw3D(kTRUE);
}
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Do_TPC_TRD_matching_allEvents(Double_t xy_matching_window, Double_t z_matching_window)
{

    th1d_offset_diff = new TH1D("th1d_offset_diff","difference in predicted pos vs actual pos of successive TPC matched tracklets",100,-5,150);
    th1d_angle_diff = new TH1D("th1d_angle_diff","difference in angle of successive TPC matched tracklets",100,-5,180);

    for(Long64_t i_event = 0; i_event < file_entries_total; i_event++)
    {
        printf("Ali_TRD_ST_Analyze::Do_TPC_TRD_matching_allEvents: Event %lld \n",i_event);

        if (!input_SE->GetEntry( i_event )) return 0; // take the event -> information is stored in event


        //--------------------------------------------------
        // Event information (more data members available, see Ali_TRD_ST_Event class definition)
        UShort_t NumTracks            = TRD_ST_Event ->getNumTracks(); // number of tracks in this event
        Int_t    NumTracklets         = TRD_ST_Event ->getNumTracklets();
        Double_t EventVertexX         = TRD_ST_Event ->getx();
        Double_t EventVertexY         = TRD_ST_Event ->gety();
        Double_t EventVertexZ         = TRD_ST_Event ->getz();
        Float_t  V0MEq                = TRD_ST_Event ->getcent_class_V0MEq();
        //--------------------------------------------------



        //--------------------------------------------------
        // Store all TRD tracklet in an array to keep it in memory
        // TRD tracklet loop
        vector< vector<TVector3> > vec_TV3_dir_tracklets;
        vector< vector<TVector3> > vec_TV3_offset_tracklets;
        vec_TV3_dir_tracklets.resize(540);
        vec_TV3_offset_tracklets.resize(540);
        
        vector< vector<Int_t> > vec_trkl_ID; //[det][i_trkl(det)] = i_trkl_global
        vec_trkl_ID.resize(540);

        Double_t ADC_val[24];

        TVector3 TV3_offset;
        TVector3 TV3_dir;
        Int_t    i_det;

        for(Int_t i_tracklet = 0; i_tracklet < NumTracklets; i_tracklet++)
        {
            TRD_ST_Tracklet = TRD_ST_Event    ->getTracklet(i_tracklet);
            TV3_offset      = TRD_ST_Tracklet ->get_TV3_offset();
            TV3_dir         = TRD_ST_Tracklet ->get_TV3_dir();
            i_det           = TRD_ST_Tracklet ->get_TRD_det();

            //create "tracklets"
            Int_t i_sector = (Int_t)(i_det/30);
            Int_t i_stack  = (Int_t)(i_det%30/6);
            Int_t i_layer  = i_det%6;
            //Int_t i_det = layer + 6*stack + 30*sector;

            vec_TV3_dir_tracklets[i_det].push_back(TV3_dir);
            vec_TV3_offset_tracklets[i_det].push_back(TV3_offset);

            vec_trkl_ID[i_det].resize(vec_TV3_dir_tracklets[i_det].size());
            vec_trkl_ID[i_det][vec_TV3_dir_tracklets[i_det].size()-1] = i_tracklet;

            for (Int_t i_tbn = 0; i_tbn < 24; i_tbn++)
            {
                ADC_val[i_tbn] = TRD_ST_Tracklet ->get_ADC_val(i_tbn);
                //printf("i_timebin: %d; ADC_val: %4.3f \n \n",i_tbn,ADC_val[i_tbn]);
            }

            TRD_ST_Tracklet_out = TRD_ST_Event_out ->createTracklet(); // rewrite same trkl info in a new Tree 

            TRD_ST_Tracklet_out     ->set_TRD_det(i_det);
            TRD_ST_Tracklet_out     ->set_TV3_offset(TV3_offset);
            TRD_ST_Tracklet_out     ->set_TV3_dir(TV3_dir);
            for (Int_t i_timebin = 0; i_timebin < 24; i_timebin++)
            {
                TRD_ST_Tracklet_out ->set_ADC_val(i_timebin,ADC_val[i_timebin]);
            }
        }
        //--------------------------------------------------



        //--------------------------------------------------
        // TPC track loop
        Double_t track_pos[3];
        Double_t radius_helix;
        for(Int_t i_track = 0; i_track < NumTracks; i_track++)
        //for(Int_t i_track = 3; i_track < 4; i_track++)
        {
            TRD_ST_TPC_Track = TRD_ST_Event ->getTrack(i_track);

            Double_t nsigma_TPC_e   = TRD_ST_TPC_Track ->getnsigma_e_TPC();
            Double_t nsigma_TPC_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TPC();
            Double_t nsigma_TPC_p   = TRD_ST_TPC_Track ->getnsigma_p_TPC();
            Double_t nsigma_TOF_e   = TRD_ST_TPC_Track ->getnsigma_e_TOF();
            Double_t nsigma_TOF_pi  = TRD_ST_TPC_Track ->getnsigma_pi_TOF();
            Double_t TRD_signal     = TRD_ST_TPC_Track ->getTRDSignal();
            Double_t TRDsumADC      = TRD_ST_TPC_Track ->getTRDsumADC();
            Double_t dca            = TRD_ST_TPC_Track ->getdca();  // charge * distance of closest approach to the primary vertex
            TLorentzVector TLV_part = TRD_ST_TPC_Track ->get_TLV_part();
            UShort_t NTPCcls        = TRD_ST_TPC_Track ->getNTPCcls();
            UShort_t NTRDcls        = TRD_ST_TPC_Track ->getNTRDcls();
            UShort_t NITScls        = TRD_ST_TPC_Track ->getNITScls();
            Float_t TPCchi2         = TRD_ST_TPC_Track ->getTPCchi2();
            Float_t TPCdEdx         = TRD_ST_TPC_Track ->getTPCdEdx();
            Float_t TOFsignal       = TRD_ST_TPC_Track ->getTOFsignal(); // in ps (1E-12 s)
            Float_t Track_length    = TRD_ST_TPC_Track ->getTrack_length();

            Float_t momentum        = TLV_part.P();
            Float_t eta_track       = TLV_part.Eta();
            Float_t pT_track        = TLV_part.Pt();
            Float_t theta_track     = TLV_part.Theta();
            Float_t phi_track       = TLV_part.Phi();

            if(momentum < 0.3) continue;

            vector<TVector3> vec_TV3_helix_points_at_TRD_layers;
            vec_TV3_helix_points_at_TRD_layers.resize(6);

            // for deviation histograms
            TVector3 a_offset = {-999, 0, 0};
            TVector3 a_dir;
            TVector3 b_offset;
            TVector3 b_dir;

            for(Int_t i_layer = 0; i_layer < 6; i_layer++)
            {
                Double_t radius_layer_center = 0.5*(TRD_layer_radii[i_layer][0] + TRD_layer_radii[i_layer][1]);

                // Find the helix path which touches the first TRD layer
                Double_t track_path_add      = 10.0;
                Double_t track_path_layer0   = 0.0;
                Double_t radius_helix_layer0 = 0.0;
                for(Double_t track_path = TRD_layer_radii[i_layer][0]; track_path < 1000; track_path += track_path_add)
                {
                    TRD_ST_TPC_Track ->Evaluate(track_path,track_pos);
                    radius_helix = TMath::Sqrt( TMath::Power(track_pos[0],2) + TMath::Power(track_pos[1],2) );
                    if(radius_helix < radius_layer_center)
                    {
                        track_path_layer0   = track_path;
                        radius_helix_layer0 = radius_helix;
                        //printf("radius_helix_layer0: %4.3f, track_path_add: %4.3f \n",radius_helix_layer0,track_path_add);
                    }
                    else
                    {
                        track_path -= track_path_add;
                        track_path_add *= 0.5;
                    }
                    if(track_path_add < 1.0)
                    {
                        vec_TV3_helix_points_at_TRD_layers[i_layer].SetXYZ(track_pos[0],track_pos[1],track_pos[2]);
                        break;
                    }
                }
                //printf("   --> i_layer: %d, track_path_layer0: %4.3f, radius_helix_layer0: %4.3f \n",i_layer,track_path_layer0,radius_helix_layer0);


                TVector3 TV3_diff_vec;
                Double_t dist_min      = 10.0;
                Double_t dist          = 0.0;
                Int_t    det_best      = -1;
                Int_t    tracklet_best = -1;
                for(Int_t i_det = 0; i_det < 540; i_det++)
                {
                    Int_t i_layer_from_det  = i_det%6;
                    if(i_layer_from_det != i_layer) continue;

                    for(Int_t i_tracklet = 0; i_tracklet < (Int_t)vec_TV3_offset_tracklets[i_det].size(); i_tracklet++)
                    {
                        TV3_diff_vec = vec_TV3_offset_tracklets[i_det][i_tracklet] - vec_TV3_helix_points_at_TRD_layers[i_layer];
                        dist = TV3_diff_vec.Mag();
                        if(dist < dist_min)
                        {
                            dist_min      = dist;
                            det_best      = i_det;
                            tracklet_best = i_tracklet;
                        }
                    }
                }

                if(det_best < 0 || tracklet_best < 0) continue;



                // fill hists of deviations
                if (a_offset[0] != -999)
                {
                    b_offset = vec_TV3_offset_tracklets[det_best][tracklet_best];
                    b_dir = vec_TV3_dir_tracklets[det_best][tracklet_best];

                    TVector3 pos;
                    double cylind_radial_dist_a_b = sqrt(pow(a_offset[0], 2) + pow(a_offset[1], 2)) - sqrt(pow(b_offset[0], 2) + pow(b_offset[1], 2));
                    TVector3 cylind_radial_unit_vec_a = (TVector3){a_offset[0], a_offset[1], 0}.Unit();

                    if (a_dir.Mag() != 0)
                    {
                        pos = a_offset - (cylind_radial_dist_a_b) * a_dir * (1 / (a_dir * cylind_radial_unit_vec_a));
                    }

                    double offset_diff = (b_offset - pos).Mag();
                    double angle_diff = abs(b_dir.Angle(a_dir))*TMath::RadToDeg();

                    th1d_offset_diff->Fill(offset_diff);
                    th1d_angle_diff->Fill(angle_diff);

                    a_offset = b_offset;
                    a_dir = b_dir;
                }

                if (a_offset[0] == -999)
                {
                    a_offset = vec_TV3_offset_tracklets[det_best][tracklet_best];
                    a_dir = vec_TV3_dir_tracklets[det_best][tracklet_best];
                }



                Int_t size_tracklet = (Int_t)vec_TEveLine_tracklets_match[i_layer].size();

                //printf("i_layer: %d, size_tracklet: %d \n",i_layer,size_tracklet);

                TRD_ST_Tracklet_out = TRD_ST_Event_out ->getTracklet(vec_trkl_ID[det_best][tracklet_best]); //set match flag 
                TRD_ST_Tracklet_out                    ->set_TPC_match(1);
            }

        }
        //--------------------------------------------------

        //loop again over trkls with TPC match flag == 0; 
        //check where is the nearest trkl
        //check how many trkl within 20 cm distance

        TVector3 TV3_offset_sub;
        TVector3 TV3_dir_sub;
        Int_t    i_det_sub;

        TVector3 TV3_diff_prim_sub;
        Int_t    n_tracklets_around;
        Double_t dist_to_next_trkl;
        Double_t min_dist_to_next_trkl;

        for(Int_t i_tracklet = 0; i_tracklet < NumTracklets; i_tracklet++)
        {
            
            n_tracklets_around    = 0;
            dist_to_next_trkl     = -1.0;
            min_dist_to_next_trkl = 20.0;

            TRD_ST_Tracklet_out = TRD_ST_Event_out   ->getTracklet(i_tracklet);
            UShort_t TPC_match = TRD_ST_Tracklet_out ->get_TPC_match();

            //printf("i_tracklet: %d, TPC_match: %d \n",i_tracklet,TPC_match);

            if (TPC_match == 0) continue; //ignore matched trkls

            TV3_offset      = TRD_ST_Tracklet_out ->get_TV3_offset();
            TV3_dir         = TRD_ST_Tracklet_out ->get_TV3_dir();
            i_det           = TRD_ST_Tracklet_out ->get_TRD_det();

            //create "tracklets"
            Int_t i_sector = (Int_t)(i_det/30);
            Int_t i_stack  = (Int_t)(i_det%30/6);
            Int_t i_layer  = i_det%6;
            //Int_t i_det = layer + 6*stack + 30*sector;

            Int_t i_det_sub_min = i_det-1;
            Int_t i_det_sub_max = i_det+1;

            if (i_det == 0)   i_det_sub_min = 0;
            if (i_det == 540) i_det_sub_min = 540;

            ////////
            for (Int_t i_det_sub = i_det_sub_min; i_det_sub < i_det_sub_max; i_det_sub++)
            {

                for(Int_t i_tracklet_sub = 0; i_tracklet_sub < (Int_t)vec_TV3_offset_tracklets[i_det_sub].size(); i_tracklet_sub++)
                {
                    if (vec_trkl_ID[i_det_sub][i_tracklet_sub] == i_tracklet) continue;
                    
                    TV3_diff_prim_sub = TV3_offset - vec_TV3_offset_tracklets[i_det_sub][i_tracklet_sub];
                    
                    if (TV3_diff_prim_sub.Mag() < 20.0) 
                    {
                        n_tracklets_around++;
                        dist_to_next_trkl = TV3_diff_prim_sub.Mag();

                         //printf("i_tracklet: %d, n_tracklets_around: %d, dist_to_next_trkl: %4.3f \n",i_tracklet,n_tracklets_around,dist_to_next_trkl);

                        if (dist_to_next_trkl < min_dist_to_next_trkl) min_dist_to_next_trkl = dist_to_next_trkl; // save min distance to next tracklet
                    }

                }
            //}
            ////////

            #if 0

            Int_t i_trkl_sub_min = 0;
            Int_t i_trkl_sub_max = NumTracklets;//i_tracklet + 300;

            //if (i_tracklet >= 300) i_trkl_sub_min = i_tracklet - 300;

            //if (i_tracklet < NumTracklets-301) i_trkl_sub_max = NumTracklets;;

            for (Int_t i_tracklet_sub = i_trkl_sub_min; i_tracklet_sub <  i_trkl_sub_max; i_tracklet_sub++)
            {
                if (i_tracklet_sub == i_tracklet) continue;

                TRD_ST_Tracklet_out = TRD_ST_Event_out    ->getTracklet(i_tracklet_sub);

                //UShort_t TPC_match = TRD_ST_Tracklet ->get_TPC_match();

                //if (TPC_match == 0) continue; //ignore matched trkls

                TV3_offset_sub      = TRD_ST_Tracklet_out ->get_TV3_offset();
                TV3_dir_sub         = TRD_ST_Tracklet_out ->get_TV3_dir();
                i_det_sub           = TRD_ST_Tracklet_out ->get_TRD_det();

                Int_t i_sector_sub = (Int_t)(i_det_sub/30);
                Int_t i_stack_sub  = (Int_t)(i_det_sub%30/6);
                Int_t i_layer_sub  = i_det_sub%6;


                if (i_sector != i_sector_sub && i_stack_sub != i_stack && fabs(i_layer - i_layer_sub) > 1) continue;

                TV3_diff_prim_sub = TV3_offset - TV3_offset_sub;

                //printf("TV3_diff_prim_sub.Mag(): %4.3f, \n",TV3_diff_prim_sub.Mag());

                if (TV3_diff_prim_sub.Mag() < 20.0) 
                {
                    n_tracklets_around++;
                    dist_to_next_trkl = TV3_diff_prim_sub.Mag();

                    //printf("i_tracklet: %d, n_tracklets_around: %d, dist_to_next_trkl: %4.3f \n",i_tracklet,n_tracklets_around,dist_to_next_trkl);

                    if (dist_to_next_trkl < min_dist_to_next_trkl) min_dist_to_next_trkl = dist_to_next_trkl; // save min distance to next tracklet
                }

            #endif 


            TRD_ST_Tracklet_out ->set_n_tracklets_around(n_tracklets_around);
            TRD_ST_Tracklet_out ->set_min_dist_to_next_trkl(min_dist_to_next_trkl);
            }
        }
        //--------------------------------------------------

        //gEve->Redraw3D(kTRUE);
        printf("Fill tree \n");
        Tree_TRD_ST_Event_out ->Fill();
        printf("Tree filled for event %lld \n",i_event);

    }

    printf("Write data to file \n");
    outputfile ->cd();
    Tree_TRD_ST_Event_out ->Write();

    printf("All data written \n");

}
//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Do_TRD_self_matching(Long64_t i_event, double offset_window, double angle_window)
{
    printf("Ali_TRD_ST_Analyze::Do_TRD_self_matching \n");

    if (!input_SE->GetEntry( i_event )) return 0; // take the event -> information is stored in event

    UShort_t NumTracks            = TRD_ST_Event ->getNumTracks(); 
    Int_t    NumTracklets         = TRD_ST_Event ->getNumTracklets();


    vector< vector<TVector3> > vec_TV3_dir_tracklets;
    vector< vector<TVector3> > vec_TV3_offset_tracklets;
    vector< vector<int> > tracklet_numbers;

    vec_TV3_dir_tracklets.resize(540);
    vec_TV3_offset_tracklets.resize(540);
    tracklet_numbers.resize(540);


    TVector3 TV3_offset;
    TVector3 TV3_dir;
    Int_t    i_det;

    for(Int_t i_tracklet = 0; i_tracklet < NumTracklets; i_tracklet++)
    {
        TRD_ST_Tracklet = TRD_ST_Event    ->getTracklet(i_tracklet);
        TV3_offset      = TRD_ST_Tracklet ->get_TV3_offset();
        TV3_dir         = TRD_ST_Tracklet ->get_TV3_dir();
        i_det           = TRD_ST_Tracklet ->get_TRD_det();

        vec_TV3_dir_tracklets[i_det].push_back(TV3_dir);
        vec_TV3_offset_tracklets[i_det].push_back(TV3_offset);

        // build vector of tracklet numbers so we can erase from other vectors safely
        tracklet_numbers[i_det].push_back(i_tracklet);
    }


    vector< vector<TVector3> > tracklet_dir = vec_TV3_dir_tracklets;
    vector< vector<TVector3> > tracklet_offset = vec_TV3_offset_tracklets;
    vector< vector< vector<double> > > tracks;
    vector< vector<double> > tracklet_chain;
    vector<double> angle_diffs;
    vector<double> offset_diffs;

    TVector3 a_offset;
    TVector3 a_dir;
    TVector3 b_offset;
    TVector3 b_dir;
    TVector3 c_offset;
    TVector3 c_dir;


    // looking for start of a track. Move in two circles over layers 5 and 4
    for (int i_layer=5; i_layer>3; i_layer--)
    {
        // loop over detectors in correct layer
        for (int a_det=(539-(5-i_layer)); a_det>=0; a_det-=6)
        {
            // if (!h_good_bad_TRD_chambers->GetBinContent(a_det)) {continue;}

            // pick first point for line
            for (int a_trkl=0; a_trkl<tracklet_dir[a_det].size(); a_trkl++)
            {
                a_offset = tracklet_offset[a_det][a_trkl];
                a_dir = tracklet_dir[a_det][a_trkl];

                int b_det = a_det - 1;

                // pick second point for line
                for (int b_trkl=0; b_trkl<tracklet_offset[b_det].size(); b_trkl++)
                {
                    tracklet_chain.clear();

                    b_offset = tracklet_offset[b_det][b_trkl];
                    b_dir = tracklet_dir[b_det][b_trkl];

                    double angle_diff_a_b = abs(b_dir.Angle(a_dir))*TMath::RadToDeg();
                    if (angle_diff_a_b > angle_window) {continue;}

                    // program looks if it can draw a straight line "connecting" three tracklets
                    for (int c_det=b_det-1; c_det>(a_det-6); c_det--)
                    {
                        offset_diffs.clear();
                        angle_diffs.clear();
                    
                        TVector3 line_predicted_point;
                        TVector3 a_b_unit_vec = (b_offset - a_offset).Unit();

                        for (int c_trkl=0; c_trkl<tracklet_offset[c_det].size(); c_trkl++)
                        {
                            c_offset = tracklet_offset[c_det][c_trkl];
                            c_dir = tracklet_dir[c_det][c_trkl];

                            double cylind_radial_dist_b_c = sqrt(pow(b_offset[0], 2) + pow(b_offset[1], 2)) - sqrt(pow(c_offset[0], 2) + pow(c_offset[1], 2));
                            TVector3 cylind_radial_unit_vec_b = (TVector3){b_offset[0], b_offset[1], 0}.Unit();

                            // ? not always an exact solution
                            if (a_b_unit_vec.Mag() != 0 && cylind_radial_unit_vec_b.Mag() != 0)
                            {
                                line_predicted_point = b_offset - (cylind_radial_dist_b_c) * a_b_unit_vec * (1 / (a_b_unit_vec * cylind_radial_unit_vec_b));
                            }

                            // cout << endl << "b radius: " << sqrt(pow(b_offset[0], 2) + pow(b_offset[1], 2)) << endl;
                            // cout << "c radius: " << sqrt(pow(c_offset[0], 2) + pow(c_offset[1], 2)) << endl;
                            // cout << "line pred radius: " << sqrt(pow(line_predicted_point[0], 2) + pow(line_predicted_point[1], 2)) << endl;

                            // ? dont use z coordinate because tracklets are planes
                            // ? calcualte this is pad/timebin coords
                            // ? pad tilting 
                            double offset_diff = (c_offset - line_predicted_point).Mag();
                            double angle_diff_b_c = abs(c_dir.Angle(b_dir))*TMath::RadToDeg();

                            offset_diffs.push_back(offset_diff);
                            angle_diffs.push_back(angle_diff_b_c);
                        }

                        double min_offset_diff = offset_window;
                        double min_angle_diff = angle_window;
                        int best_trkl;

                        // select best tracklet from candidates
                        // ? ought to consider angle as well
                        int count = 0;
                        for (int trkl=0; trkl<offset_diffs.size(); trkl++)
                        {
                            if (offset_diffs[trkl] < min_offset_diff)
                            {
                                min_offset_diff = offset_diffs[trkl];
                                min_angle_diff = angle_diffs[trkl];
                                best_trkl = trkl;
                                
                                c_offset = tracklet_offset[c_det][best_trkl];
                                c_dir = tracklet_dir[c_det][best_trkl];

                                // number of tracklets within acceptance radius
                                count ++;
                            }
                        }

                        // best tracklet will not pass if its angle is bad
                        if (min_offset_diff < offset_window && min_angle_diff < angle_window && count <= 3)
                        {
                            int best_trkl_number = tracklet_numbers[c_det][best_trkl];
                            
                            tracklet_chain.push_back({(double)c_det, (double)best_trkl_number, (double)best_trkl, min_offset_diff, min_angle_diff, c_offset[0], c_offset[1], c_offset[2]});

                            a_offset = b_offset;
                            a_dir = b_dir;

                            b_offset = c_offset;
                            b_dir = c_dir;
                            
                        } else {break;}
                    }

                    // evaluate track as a whole
                    double angle_diff_sum = 0;
                    double offset_diff_sum = 0;

                    for (int t=0; t<tracklet_chain.size(); t++)
                    {
                        offset_diff_sum += tracklet_chain[t][3];
                        angle_diff_sum += tracklet_chain[t][4];
                    }   

                    // ? is it helpful to check these means
                    if (tracklet_chain.size()+2 >= 5 && offset_diff_sum/tracklet_chain.size() < 10 && angle_diff_sum/tracklet_chain.size() < 30)
                    {
                        int b_trkl_number = tracklet_numbers[b_det][b_trkl];
                        tracklet_chain.insert(tracklet_chain.begin(), {(double)b_det, double(b_trkl_number), (double)b_trkl, 0, 0, b_offset[0], b_offset[1], b_offset[2]});

                        int a_trkl_number = tracklet_numbers[a_det][a_trkl];
                        tracklet_chain.insert(tracklet_chain.begin(), {(double)a_det, double(a_trkl_number), (double)a_trkl, 0, 0, a_offset[0], a_offset[1], a_offset[2]});

                        tracks.push_back(tracklet_chain);

                        // erase track tracklets from third point onwards to avoid duplicate tracks
                        for (int t=2; t<tracklet_chain.size(); t++)
                        {
                            int c_det = (int)tracklet_chain[t][0];
                            int best_trkl = (int)tracklet_chain[t][2];

                            tracklet_offset[c_det].erase(tracklet_offset[c_det].begin() + best_trkl);
                            tracklet_dir[c_det].erase(tracklet_dir[c_det].begin() + best_trkl);
                            tracklet_numbers[c_det].erase(tracklet_numbers[c_det].begin() + best_trkl);
                        }   

                        // pick a new starting tracklet
                        break;
                    }
                }
            }
        }
    }

    // print out beautiful tracks
    for (int i=0; i<tracks.size(); i++)
    {
        cout << endl << "track: " << i << endl;
        for (int j=0; j<tracks[i].size(); j++)
        {
            cout << "det: " << tracks[i][j][0] << " | tracklet: " << tracks[i][j][1] << endl;
        }
    }

    // draw matched tracklets
    // ? some not being drawn correctly
    vec_TEveLine_self_matched_tracklets.resize(tracks.size());

    for (int i_track=0; i_track<tracks.size(); i_track++)
    { 
        for (int i_tracklet=0; i_tracklet<tracks[i_track].size(); i_tracklet++)
        {
            int tracklet_number = tracks[i_track][i_tracklet][1];
            
            TRD_ST_Tracklet = TRD_ST_Event ->getTracklet(tracklet_number);
            TVector3 offset = TRD_ST_Tracklet ->get_TV3_offset();
            TVector3 dir = TRD_ST_Tracklet ->get_TV3_dir();

            vec_TEveLine_self_matched_tracklets[i_track].resize(tracks[i_track].size());

            vec_TEveLine_self_matched_tracklets[i_track][i_tracklet] = new TEveLine();
            vec_TEveLine_self_matched_tracklets[i_track][i_tracklet] ->SetNextPoint(offset[0], offset[1], offset[2]);
            vec_TEveLine_self_matched_tracklets[i_track][i_tracklet] ->SetNextPoint(offset[0] + scale_length*dir[0],offset[1] + scale_length*dir[1],offset[2] + scale_length*dir[2]);

            vec_TEveLine_self_matched_tracklets[i_track][i_tracklet] ->SetLineStyle(1);
            vec_TEveLine_self_matched_tracklets[i_track][i_tracklet] ->SetLineWidth(6);
            vec_TEveLine_self_matched_tracklets[i_track][i_tracklet] ->SetMainColor(color_layer_match[i_tracklet]);

            gEve->AddElement(vec_TEveLine_self_matched_tracklets[i_track][i_tracklet]);
        }
    }

    gEve->Redraw3D(kTRUE);
}
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Draw_Kalman_Tracks(vector< vector<Ali_TRD_ST_Tracklets*> > found_tracks){
	vector< vector<TEveLine*> > TEveLine_Kalman_found;
	TEveLine_Kalman_found.resize(found_tracks.size());
	for(Int_t i_Track=0;i_Track<found_tracks.size();i_Track++){
		TEveLine_Kalman_found[i_Track].resize(6);
		
		for(Int_t i_lay=0;i_lay<6;i_lay++){
		    if(found_tracks[i_Track][i_lay]!=NULL){
				TEveLine_Kalman_found[i_Track][i_lay]=new TEveLine();
				TEveLine_Kalman_found[i_Track][i_lay]->SetNextPoint(found_tracks[i_Track][i_lay]->get_TV3_offset()[0],found_tracks[i_Track][i_lay]->get_TV3_offset()[1],found_tracks[i_Track][i_lay]->get_TV3_offset()[2]);
				TEveLine_Kalman_found[i_Track][i_lay]->SetNextPoint(found_tracks[i_Track][i_lay]->get_TV3_offset()[0] + scale_length*found_tracks[i_Track][i_lay]->get_TV3_dir()[0],found_tracks[i_Track][i_lay]->get_TV3_offset()[1] + scale_length*found_tracks[i_Track][i_lay]->get_TV3_dir()[1],found_tracks[i_Track][i_lay]->get_TV3_offset()[2] + scale_length*found_tracks[i_Track][i_lay]->get_TV3_dir()[2]);    
				
		        Int_t i_det           = found_tracks[i_Track][i_lay] ->get_TRD_det();
		        Int_t i_sector = (Int_t)(i_det/30);
		        Int_t i_stack  = (Int_t)(i_det%30/6);
        		Int_t i_layer  = i_det%6;

				
				HistName = "tracklet (kal) ";
				HistName += i_Track;
				HistName += "_";
            	HistName += i_sector;
				HistName += "_";
				HistName += i_stack;
				HistName += "_";
            	HistName += i_lay;
				
				TEveLine_Kalman_found[i_Track][i_lay]	->SetName(HistName.Data());
				TEveLine_Kalman_found[i_Track][i_lay]   ->SetLineStyle(1);
            	TEveLine_Kalman_found[i_Track][i_lay]  	->SetLineWidth(6);
            	TEveLine_Kalman_found[i_Track][i_lay]   ->SetMainColor(kOrange);
				gEve->AddElement(TEveLine_Kalman_found[i_Track][i_lay]);

			}
		}
	}
	gEve->Redraw3D(kTRUE);
}
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
void Ali_TRD_ST_Analyze::Draw_hist_TPC_tracklet_diffs()
{
    printf("Ali_TRD_ST_Analyze::Draw_hist_TPC_tracklet_diffs \n");

    th1d_offset_diff->GetXaxis()->SetTitle("Offset Difference (cm)");
    th1d_offset_diff->GetYaxis()->SetTitle("Number of Tracklets");
    th1d_offset_diff->GetXaxis()->CenterTitle();
    th1d_offset_diff->GetYaxis()->CenterTitle();

    TCanvas *th1d_offset_diff_can = new TCanvas("th1d_offset_diff_can", "Max");
    th1d_offset_diff_can->cd();
    th1d_offset_diff->Draw();

    th1d_angle_diff->GetXaxis()->SetTitle("Angle Difference (deg)");
    th1d_angle_diff->GetYaxis()->SetTitle("Number of Tracklets");
    th1d_angle_diff->GetXaxis()->CenterTitle();
    th1d_angle_diff->GetYaxis()->CenterTitle();

    TCanvas *th1d_angle_diff_can = new TCanvas("th1d_angle_diff_can", "Plateu");
    th1d_angle_diff_can->cd();
    th1d_angle_diff->Draw();
}
//----------------------------------------------------------------------------------------

TH1I* Ali_TRD_ST_Analyze::get_h_good_bad_TRD_chambers(){
	return h_good_bad_TRD_chambers;
}
	