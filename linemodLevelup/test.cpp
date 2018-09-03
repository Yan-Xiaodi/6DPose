#include "linemodLevelup.h"
#include <memory>
#include <iostream>
#include "linemod_icp.h"
#include <assert.h>
#include <chrono>  // for high_resolution_clock
#include <opencv2/rgbd.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/cudaimgproc.hpp>
using namespace std;
using namespace cv;

static std::string prefix = "/home/meiqua/6DPose/linemodLevelup/test/case1/";
// for test
std::string type2str(int type) {
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}
void train_test(){
    Mat rgb = cv::imread("/home/meiqua/6DPose/linemodLevelup/test/667/rgb.png");
    Mat depth = cv::imread("/home/meiqua/6DPose/linemodLevelup/test/667/dep.png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
    Mat mask = cv::imread("/home/meiqua/6DPose/linemodLevelup/test/667/mask.png");
    cvtColor(mask, mask, cv::COLOR_RGB2GRAY);

    vector<Mat> sources;
    sources.push_back(rgb);
    sources.push_back(depth);
    auto detector = linemodLevelup::Detector(20, {4, 8}, 16);
    detector.addTemplate(sources, "06_template", mask);
    detector.writeClasses(prefix+"writeClasses/%s.yaml");
    cout << "break point line: train_test" << endl;
}

void detect_test(){
    // test case1
    /*
     * (x=327, y=127, float similarity=92.66, class_id=06_template, template_id=424)
     * render K R t:
  cam_K: [572.41140000, 0.00000000, 325.26110000, 0.00000000, 573.57043000, 242.04899000, 0.00000000, 0.00000000, 1.00000000]
  cam_R_w2c: [0.34768538, 0.93761126, 0.00000000, 0.70540612, -0.26157897, -0.65877056, -0.61767070, 0.22904489, -0.75234390]
  cam_t_w2c: [0.00000000, 0.00000000, 1000.00000000]

  gt K R t:
- cam_R_m2c: [0.09506610, 0.98330897, -0.15512900, 0.74159598, -0.17391300, -0.64791101, -0.66407597, -0.05344890, -0.74575198]
  cam_t_m2c: [71.62781422, -158.20064191, 1050.77777823]
  obj_bb: [331, 130, 65, 64]
  obj_id: 6
  */

    Mat rgb = cv::imread(prefix+"0000_rgb.png");
    Mat rgb_half = cv::imread(prefix+"0000_rgb_half.png");
    Mat depth = cv::imread(prefix+"0000_dep.png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
    Mat depth_half = cv::imread(prefix+"0000_dep_half.png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
    vector<Mat> sources, src_half;
    sources.push_back(rgb); src_half.push_back(rgb_half);
    sources.push_back(depth); src_half.push_back(depth_half);


//    rectangle(rgb, Point(326,132), Point(347, 197), Scalar(255, 255, 255), -1);
//    rectangle(depth, Point(326,132), Point(347, 197), Scalar(255, 255, 255), -1);
//    imwrite(prefix+"0000_dep_half.png", depth);
//    imwrite(prefix+"0000_rgb_half.png", rgb);
//    imshow("rgb", rgb);
//    waitKey(0);

    Mat K_ren = (Mat_<float>(3,3) << 572.4114, 0.0, 325.2611, 0.0, 573.57043, 242.04899, 0.0, 0.0, 1.0);
    Mat R_ren = (Mat_<float>(3,3) << 0.34768538, 0.93761126, 0.00000000, 0.70540612,
                 -0.26157897, -0.65877056, -0.61767070, 0.22904489, -0.75234390);
    Mat t_ren = (Mat_<float>(3,1) << 0.0, 0.0, 1000.0);

    vector<string> classes;
    classes.push_back("06_template");

    auto detector = linemodLevelup::Detector(20,{4, 8});
    detector.readClasses(classes, prefix + "%s.yaml");

    auto start_time = std::chrono::high_resolution_clock::now();
    vector<linemodLevelup::Match> matches = detector.match(sources, 65, 0.6f, classes);
    auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
    cout << "match time: " << elapsed_time.count()/1000000000.0 <<"s" << endl;

    vector<Rect> boxes;
    vector<float> scores;
    vector<int> idxs;
    for(auto match: matches){
        Rect box;
        box.x = match.x;
        box.y = match.y;
        box.width = 40;
        box.height = 40;
        boxes.push_back(box);
        scores.push_back(match.similarity);
    }
    cv::dnn::NMSBoxes(boxes, scores, 0, 0.4, idxs);

    Mat draw = rgb;
    for(auto idx : idxs){
        auto match = matches[idx];
        int r = 40;
        cout << "x: " << match.x << "\ty: " << match.y << "\tid: " << match.template_id
             << "\tsimilarity: "<< match.similarity <<endl;
        cv::circle(draw, cv::Point(match.x+r,match.y+r), r, cv::Scalar(255, 0 ,255), 2);
        cv::putText(draw, to_string(int(round(match.similarity))),
                    Point(match.x+r-10, match.y-3), FONT_HERSHEY_PLAIN, 1.4, Scalar(0,255,255));

    }
    imshow("rgb", draw);
//    imwrite(prefix+"result/depth600_hist.png", draw);
    waitKey(0);
}

void dataset_test(){
    string pre = "/home/meiqua/6DPose/public/datasets/hinterstoisser/test/06/";
    int i=0;
    for(;i<1000;i++){
        auto i_str = to_string(i);
        for(int pad=4-i_str.size();pad>0;pad--){
            i_str = '0'+i_str;
        }
        Mat rgb = cv::imread(pre+"rgb/"+i_str+".png");
        Mat depth = cv::imread(pre+"depth/"+i_str+".png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
        vector<Mat> sources;
        sources.push_back(rgb);
        sources.push_back(depth);
        auto detector = linemodLevelup::Detector(16, {4, 8});

        std::vector<int> dep_anchors = {600, 660, 726, 798, 878, 966, 1062, 1169, 1286};
        int dep_range = 200;
        vector<string> classes;
        for(int dep: dep_anchors){
            classes.push_back("06_template_"+std::to_string(dep));
        }
        detector.readClasses(classes, prefix + "%s.yaml");

        auto start_time = std::chrono::high_resolution_clock::now();
        vector<linemodLevelup::Match> matches = detector.match(sources, 70, 0.6f, classes, dep_anchors, dep_range);
        auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
        cout << "match time: " << elapsed_time.count()/1000000000.0 <<"s" << endl;

        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<int> idxs;
        for(auto match: matches){
            Rect box;
            box.x = match.x;
            box.y = match.y;
            box.width = 40;
            box.height = 40;
            boxes.push_back(box);
            scores.push_back(match.similarity);
        }
        cv::dnn::NMSBoxes(boxes, scores, 0, 0.4, idxs);

        cv::Mat draw = rgb;
        for(auto idx : idxs){
            auto match = matches[idx];

//            auto templ = detector.getTemplates(match.class_id, match.template_id);
//            cv::Mat show_templ = cv::Mat(templ[0].height+1, templ[0].width+1, CV_8UC3, cv::Scalar(0));
//            for(auto f: templ[0].features){
//                cv::circle(show_templ, {f.x, f.y}, 1, {0, 0, 255}, -1);
//            }
//            for(auto f: templ[1].features){
//                cv::circle(show_templ, {f.x, f.y}, 1, {0, 255, 0}, -1);
//            }
//            cv::imshow("templ", show_templ);
//            cv::waitKey(0);

            int r = 40;
            cout << "\nx: " << match.x << "\ty: " << match.y
                 << "\tsimilarity: "<< match.similarity <<endl;
            cout << "class_id: " << match.class_id << "\ttemplate_id: " << match.template_id <<endl;
            cv::circle(draw, cv::Point(match.x+r,match.y+r), r, cv::Scalar(255, 0 ,255), 2);
            cv::putText(draw, to_string(int(round(match.similarity))),
                        cv::Point(match.x+r-10, match.y-3), FONT_HERSHEY_PLAIN, 1.4, cv::Scalar(0,255,255));
        }
        std::cout << "i: " << i << std::endl;
        imshow("rgb", draw);

        waitKey(0);
//        imwrite(prefix+"scaleTest/" +i_str+ ".png", draw);
    }
    cout << "dataset_test end line" << endl;
}

int main(){

//    train_test();
//    detect_test();
    dataset_test();
    return 0;
}
