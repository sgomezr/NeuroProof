#include "../FeatureManager/FeatureMgr.h"
#include "../BioPriors/BioStackController.h"
#include "../BioPriors/StackLearnAlgs.h"

#include "../Utilities/ScopeTime.h"
#include "../Utilities/OptionParser.h"

#include <iostream>

using std::cerr; using std::cout; using std::endl;
using std::string;
using namespace NeuroProof;

static const char * SEG_DATASET_NAME = "stack";
static const char * PRED_DATASET_NAME = "volume/predictions";

struct LearnOptions
{
    LearnOptions(int argc, char** argv) : classifier_filename("classifier.xml"),
                strategy_type(1), num_iterations(1)
    {
        OptionParser parser("Program that learns agglomeration classifier from an initial segmentation");

        // positional arguments
        parser.add_positional(watershed_filename, "watershed-file",
                "gala h5 file with label volume (z,y,x) and body mappings (assume indivisible labels)"); 
        parser.add_positional(prediction_filename, "prediction-file",
                "ilastik h5 file (x,y,z,ch) that has pixel predictions"); 
        parser.add_positional(groundtruth_filename, "groundtruth-file",
                "h5 file with groundtruth label volume (z,y,x) and body mappings"); 

        parser.add_option(classifier_filename, "classifier-name",
                "opencv or vigra agglomeration classifier name"); 
        parser.add_option(strategy_type, "strategy-type",
                "learning strategy to use (1: only misclassified; 2: all; 3: LASH)");
        parser.add_option(num_iterations, "num-iterations",
                "number of iterations used for learning");

        parser.parse_options(argc, argv);
    }

    // manadatory positionals
    string watershed_filename;
    string prediction_filename;
    string groundtruth_filename;
   
    // optional (with default values) 
    string classifier_filename;
    int strategy_type;
    int num_iterations;
};

bool endswith(string filename, string extn){
    unsigned found = filename.find_last_of(".");
    string fextn = filename.substr(found);	
    if (fextn.compare(extn) == 0 )
	return true;
    else return false;	  
}


void run_learning(LearnOptions& options)
{
    int strategy = 1;

    vector<VolumeProbPtr> prob_list = VolumeProb::create_volume_array(
        options.prediction_filename.c_str(), PRED_DATASET_NAME);
    
    VolumeLabelPtr watershed_data = VolumeLabelData::create_volume(
            options.watershed_filename.c_str(), SEG_DATASET_NAME);

    VolumeLabelPtr groundtruth_data = VolumeLabelData::create_volume(
            options.groundtruth_filename.c_str(), SEG_DATASET_NAME);
    
    const double threshold = 0.2;

    EdgeClassifier* eclfr;
    if (endswith(options.classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier();	
    else if (endswith(options.classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier();	

    BioStack stack(watershed_data); 

    FeatureMgrPtr feature_manager(new FeatureMgr(prob_list.size()));
    stack.set_prob_list(prob_list);
    feature_manager->set_basic_features(); 
    feature_manager->set_classifier(eclfr);

    stack.set_feature_manager(feature_manager);
    stack.set_gt_labelvol(groundtruth_data);	

    BioStackController controller(&stack);

    UniqueRowFeature_Label all_features;
    vector<int> all_labels;	
    
    for(int itr = 0; itr < options.num_iterations; ++itr){
        cout << endl << " ** Learning iteration " << itr+1 << "  **" << endl << endl;
        feature_manager->clear_features();
	
	cout << "Learn edge classifier ..." << endl; 
	if (itr == 0) {
	    learn_edge_classifier_flat(controller, threshold, all_features,
                    all_labels, true); // # iteration, threshold, clfr_filename
	} else{
	    if (options.strategy_type == 1){ //accumulate only misclassified 
		cout << "cumulative learning, only misclassified" << endl;
	   	learn_edge_classifier_queue(controller, threshold, all_features,
                        all_labels, false, true); // # iteration, threshold, clfr_filename	
	    } else if (options.strategy_type == 2){ //accumulate all 
		cout << "cumulative learning, all\n" << endl;
	   	learn_edge_classifier_queue(controller, threshold, all_features,
                        all_labels, true, true); // # iteration, threshold, clfr_filename	
            } else if (options.strategy_type == 3){ // lash	
		cout << "learning by LASH" << endl;
	   	learn_edge_classifier_lash(controller, threshold, all_features,
                        all_labels, true); // # iteration, threshold, clfr_filename	
	    }
	}

	cout << "done with "<< controller.get_num_labels()<< " nodes" << endl;	
    }

    eclfr->save_classifier(options.classifier_filename.c_str());  	
    delete eclfr;
}


int main(int argc, char** argv) 
{
    LearnOptions options(argc, argv);
    ScopeTime timer;

    run_learning(options);

    return 0;
}
