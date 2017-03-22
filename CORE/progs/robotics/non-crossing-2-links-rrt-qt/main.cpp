/*
 *  main.cpp
 *  
 *  Author: zhonghua, Jyh-Ming Lien, Ching-Hsiang (Tom) Hsu
 *  Email: chhsu@nyu.edu
 *  Created on: Nov 26, 2012
 *  Modified: Jyh-Ming Lien, Nov. 23, 2013
 *            Ching-Hsiang (Tom) Hsu, Oct. 31, 2016
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "main.h"

#define mw_out (*window)
MainWindow *window;

FILE *fp;

//environment related parameters
string workingDir;
string cfgName("WAFR2016_Hole-in-Wall_a.cfg");    // Input example name
string inputDir("inputs");              // Path for input files
string fileName("input2.txt");          // Input file name
char cfgNameList[200][200];
int numEg = 0;

CFG start(250,360,0,150,false);
CFG goal(260,80,70,320,false);
int l1,l2;
double thickness;
float scale=1.0;
float deltaX=0.0, deltaY=0.0;
int seed;
double timeout = 60;

int windowPosX = 30;			// X Position of Window
int windowPosY = 10;			// Y Position of Window
float env_width = 512;			// ENV WIDTH
float env_height = 512;			// ENV HEIGHT
float env_TR = 0.001;			// TRANSLATIONAL RESOLUTION
float env_RR = 0.01;			// Rotational RESOLUTION (deg)

//common samping-based motion planner parameters
string method="toggle"; //planner, prm, ...
int SearchType = 0;
unsigned int max_sample_size=10000;

//planner parameters
float rrt_step_size=0.01;
float rrt_bias = 0.1;					// bias towards to the goal
float rrt_close_to_goal = env_TR;

//prm parameters
unsigned int prm_closest_free_k=10;
unsigned int prm_closest_obst_k=2;

//gaussian prm paramters
float gauss_mean_d=0.1;
float gauss_std=0.1;

bool noPath(false);
bool showAnim(true);
bool pauseAnim(false);
bool replayAnim(false);
int path_index(0);
bool rrt_graph(false);
bool prm_graph(true);
int prm_graph_flag(1);
bool prm_graph_mixed(true);
bool prm_graph_free(true);
bool prm_graph_obst(true);
bool prm_graph_edge(true);
bool non_crossing(false);
double bandwidth(0);
int animationSpeed(99);
int animationSpeedScale(5000);
bool showTrace(false);
bool showFilledObstacles(false);

bool ompl;

double elapsedTime, elapsedCPUTime;

void parseExampleList();
void parseExampleFile();
void parseMapFile();
void run();


/* ********************************************************************** */
// skip blanks, tabs, line breaks and comment lines,
//  leaving us at the beginning of a token (or EOF)
//  (This code is taken from CORE lib)
inline int skip_comment_line (std::ifstream & in)
{
      int c;

      do {
        c = in.get();
        while ( c == '#' ) {
          do {// ignore the rest of this line
            c = in.get();
          } while ( c != '\n' );
          c = in.get(); // now, reach the beginning of the next line
        }
      } while (c == ' ' || c == '\t' || c == '\n' || c == '\r'); //ignore white spaces and newlines

      in.putback(c);  // this is non-white and non-comment char!
      return c;

}//skip_comment_line

char cfgPath[256], tmp[256];
void parseExampleList() {
  char sptr[200];
  string cfgDir = workingDir + "/" + inputDir;
  QDir tmpDir(cfgDir.c_str());
  QStringList fileList = tmpDir.entryList();
  while(!fileList.empty()){
      QString fileName_l = fileList.front();
      strcpy(sptr, fileName_l.toStdString().c_str());
      int len = strlen(sptr);
      if(len > 4 && sptr[len-1] == 'g' && sptr[len-2] == 'f' && sptr[len-3] == 'c' && sptr[len-4] == '.'){
          strcpy(cfgNameList[numEg], sptr);
          ++numEg;
      }
      fileList.pop_front();
  }
}

void parseExampleFile() {

    sprintf(cfgPath, "%s/%s", inputDir.c_str(), cfgName.c_str());
    FILE *fptr = fopen(cfgPath, "r");
    if (fptr == NULL) return ;

    while (fgets(tmp, 256, fptr) != NULL){
        char *sptr = strtok(tmp, "=: \t\n");

        if (sptr == NULL) {
            continue;
        }
        // comments
        if (strcmp(sptr, "#") == 0) {
            continue;
        }

        // start configuration
        if (strcmp(sptr, "startX") == 0) {
            sptr = strtok(NULL, "=: \t");
            start.x = atof(sptr);
        }
        if (strcmp(sptr, "startY") == 0) {
            sptr = strtok(NULL, "=: \t");
            start.y = atof(sptr);
        }
        if (strcmp(sptr, "startTheta1") == 0) {
            sptr = strtok(NULL, "=: \t");
            start.t1 = atof(sptr);
        }
        if (strcmp(sptr, "startTheta2") == 0) {
            sptr = strtok(NULL, "=: \t");
            start.t2 = atof(sptr);
        }

        // goal configuration
        if (strcmp(sptr, "goalX") == 0) {
            sptr = strtok(NULL, "=: \t");
            goal.x = atof(sptr);
        }
        if (strcmp(sptr, "goalY") == 0) {
            sptr = strtok(NULL, "=: \t");
            goal.y = atof(sptr);
        }
        if (strcmp(sptr, "goalTheta1") == 0) {
            sptr = strtok(NULL, "=: \t");
            goal.t1 = atof(sptr);
        }
        if (strcmp(sptr, "goalTheta2") == 0) {
            sptr = strtok(NULL, "=: \t");
            goal.t2 = atof(sptr);
        }

        if (strcmp(sptr, "len1") == 0) {
            sptr = strtok(NULL, "=: \t");
            l1 = atoi(sptr);
        }
        if (strcmp(sptr, "len2") == 0) {
            sptr = strtok(NULL, "=: \t");
            l2 = atoi(sptr);
        }
        if (strcmp(sptr, "thickness") == 0) {
            sptr = strtok(NULL, "=: \t");
            thickness = atof(sptr);
        }

        if (strcmp(sptr, "inputDir") == 0) {
            sptr = strtok(NULL, "=: #\t");
            inputDir = sptr;
        }
        if (strcmp(sptr, "fileName") == 0) {
            sptr = strtok(NULL, "=: #\t");
            fileName = sptr;
        }

        // box width and height
        if (strcmp(sptr, "boxWidth") == 0) {
            sptr = strtok(NULL, "=: \t");
            env_width = atof(sptr);
        }
        if (strcmp(sptr, "boxHeight") == 0) {
            sptr = strtok(NULL, "=: \t");
            env_height = atof(sptr);
        }

//        // windows position
//        if (strcmp(sptr, "windowPosX") == 0) {
//            sptr = strtok(NULL, "=: \t");
//            windowPosX = atoi(sptr);
//        }
//        if (strcmp(sptr, "windowPosY") == 0) {
//            sptr = strtok(NULL, "=: \t");
//            windowPosY = atoi(sptr);
//        }

        if (strcmp(sptr, "method") == 0) {
            sptr = strtok(NULL, "=: \t\n");
            method = sptr;
        }

        if (strcmp(sptr, "seed") == 0) {
            sptr = strtok(NULL, "=: \t");
            seed = atoi(sptr);
        }

        if (strcmp(sptr, "timeout") == 0) {
            sptr = strtok(NULL, "=: \t");
            timeout = atof(sptr);
        }

        if (strcmp(sptr, "xtrans") == 0) {
            sptr = strtok(NULL, "=: \t");
            deltaX = atof(sptr);
        }

        if (strcmp(sptr, "ytrans") == 0) {
            sptr = strtok(NULL, "=: \t");
            deltaY = atof(sptr);
        }

        if (strcmp(sptr, "scale") == 0) {
            sptr = strtok(NULL, "=: \t");
            scale = atof(sptr);
        }

        if (strcmp(sptr, "maxSampleSize") == 0) {
            sptr = strtok(NULL, "=: \t");
            max_sample_size = atoi(sptr);
        }

        if (strcmp(sptr, "prmClosestK") == 0) {
            sptr = strtok(NULL, "=: \t");
            prm_closest_free_k = atoi(sptr);
        }
        if (strcmp(sptr, "prmClosestKFree") == 0) {
            sptr = strtok(NULL, "=: \t");
            prm_closest_free_k = atoi(sptr);
        }
        if (strcmp(sptr, "prmClosestKObst") == 0) {
            sptr = strtok(NULL, "=: \t");
            prm_closest_obst_k = atoi(sptr);
        }

        if (strcmp(sptr, "GaussianMean") == 0) {
            sptr = strtok(NULL, "=: \t");
            gauss_mean_d = atof(sptr);
        }

        if (strcmp(sptr, "GaussianStd") == 0) {
            sptr = strtok(NULL, "=: \t");
            gauss_std = atof(sptr);
        }

        if (strcmp(sptr, "rrtStepSize") == 0) {
            sptr = strtok(NULL, "=: \t");
            rrt_step_size = atof(sptr);
        }

        if (strcmp(sptr, "rrtBias") == 0) {
            sptr = strtok(NULL, "=: \t");
            rrt_bias = atof(sptr);
        }

        if (strcmp(sptr, "rrtClose2Goal") == 0) {
            sptr = strtok(NULL, "=: \t");
            rrt_close_to_goal = atof(sptr);
        }

        if(strcmp(sptr, "non-crossing") == 0) {
            sptr = strtok(NULL, "=: \t");
            non_crossing = atoi(sptr);
        }
        if(strcmp(sptr, "bandwidth") == 0) {
            sptr = strtok(NULL, "=: \t");
            bandwidth = atof(sptr);
        }
        if (strcmp(sptr, "ompl") == 0) {
            sptr = strtok(NULL, "=: \t");
            ompl = atoi(sptr);
        }
//        if (strcmp(sptr, "safeAngle") == 0) {
//            sptr = strtok(NULL, "=: \t");
//            safeAngle = atoi(sptr);
//        }

        if (strcmp(sptr, "animationSpeed") == 0) {
            sptr = strtok(NULL, "=: \t");
            animationSpeed = atoi(sptr);
        }
        if (strcmp(sptr, "animationSpeedScale") == 0) {
            sptr = strtok(NULL, "=: \t");
            animationSpeedScale = atoi(sptr);
        }
    }


    // OMPL
    if(ompl){
        double degToRad = PI/180.0f;
        start.x = start.x - l1*cos((start.t1)*degToRad)*0.5f;
        start.y = start.y - l1*sin((start.t1)*degToRad)*0.5f;
        goal.x = goal.x - l1*cos((goal.t1)*degToRad)*0.5f;
        goal.y = goal.y - l1*sin((goal.t1)*degToRad)*0.5f;
    }
}

void parseMapFile() {

    free(planner);

    if(method=="prm" || method=="PRM" || method=="Prm"){
        SearchType = 0;
        planner = new PRM(env_width, env_height, env_TR, env_RR, max_sample_size,prm_closest_free_k);
    }
    else if(method=="gauss" || method=="GAUSS" || method=="Gauss"){
        SearchType = 1;
        planner = new GaussianPRM(env_width, env_height, env_TR, env_RR, max_sample_size,prm_closest_free_k,gauss_mean_d, gauss_std);
    }
    else if(method=="rrt" || method=="RRT" || method=="Rrt"){
        SearchType = 2;
        //float w, float h, float tr, float rr, unsigned int max_sample, float expand_step, float bias, float close_to_goal
        planner = new RRT(env_width, env_height, env_TR, env_RR, max_sample_size, rrt_step_size,rrt_bias,rrt_close_to_goal);
    }
    else if(method=="toggle" || method=="Toggle" || method=="TOGGLE"){
        SearchType = 3;
        planner = new TOGGLEPRM(env_width, env_height, env_TR, env_RR, max_sample_size,prm_closest_free_k,prm_closest_obst_k);
    }
    else if(method=="lazytoggle" || method=="LazyToggle" || method=="LAZYTOGGLE"){
        SearchType = 4;
        planner = new LAZYTOGGLEPRM(env_width, env_height, env_TR, env_RR, max_sample_size,prm_closest_free_k,prm_closest_obst_k);
    }

    if(planner == NULL) return;

    string fullName = inputDir+"/"+fileName;
    ifstream fin;
    fin.open(fullName.c_str());

    char buf[1024];
    int size = -1;
    int left = -1;
    int num_of_polygons = -1;
    vector<float> pts;
    while(skip_comment_line(fin)!=EOF) {
        fin.getline(buf, 1024);
        string lline(buf);
        if(buf[0] == '#' || lline.size() == 0) continue;

        //remove trailing white space chars and checking "\\"
        do{
            while(lline.at(lline.size()-1) == ' ') {
                lline.erase(lline.size()-2, lline.size()-1);
            }
            if(lline.at(lline.size()-1) == '\\'){ //there is "\\" at the end... read more
                lline.erase(lline.size()-2, lline.size()-1);
                fin.getline(buf, 1024);
                string append(buf);
                lline.append(append.begin(),append.end());
            }
            else break;
        }while(true);

        stringstream ss(lline);
        if(size == -1) {
            ss >> size;		//read point size
            left = size*2;
        }
        else if(left != 0)
        {
            float p;
            while(ss>>p) {
                pts.push_back(p);
                left--;
            }
        }
        else if(num_of_polygons == -1)
        {
            ss>>num_of_polygons;
        }
        // polygon
        else if(num_of_polygons>0)
        {
            c_ply ply(c_ply::POUT);
            ply.beginPoly();
            int index;

            while(ss>>index) {
                index--;
                ply.addVertex(pts[index*2]* scale + deltaX, pts[index*2+1]* scale + deltaY, false);
            }
            ply.endPoly(false);
            planner->addObj(ply);
            num_of_polygons--;
        }
        //
        //sstream
    }
}

int main(int argc, char** argv) {
    bool foundFiles = false;
    workingDir = QDir::currentPath().toStdString();

    // Test if the build directory is non-crossing-2-links-rrt-qt. If so,
    // the path to the current working directory will
    // include /non-crossing-2-links-rrt-qt
    unsigned long indexOfDesiredDir = workingDir.rfind("/non-crossing-2-links-rrt-qt/");
    if (indexOfDesiredDir != std::string::npos) {
        workingDir = workingDir.substr(0, indexOfDesiredDir + 5);

        // Set current working directory to non-crossing-2-links-rrt-qt
        QDir::setCurrent(workingDir.c_str());
        foundFiles = true;
    }

    // Test if program was downloaded from Github, and is the build directory.
    // Downloading it from Github will result in the folder having the name
    // /non-crossing-2-links-rrt-qt-master instead of /non-crossing-2-links-rrt-qt
    if (!foundFiles && (indexOfDesiredDir = workingDir.rfind("/non-crossing-2-links-rrt-qt/")) != std::string::npos) {
        workingDir = workingDir.substr(0, indexOfDesiredDir + 12);

        // Set current working directory to /non-crossing-2-links-rrt-qt-master
        QDir::setCurrent(workingDir.c_str());
        foundFiles = true;
    }

    // Test if a build directory (/build-non-crossing-2-links-rrt-qt-...) was created. This directory
    // will reside in the same directory as /non-crossing-2-links-rrt-qt
    if (!foundFiles && (indexOfDesiredDir = workingDir.rfind("/build-non-crossing-2-links-rrt-qt")) != std::string::npos) {
        QDir dir(workingDir.substr(0, indexOfDesiredDir).c_str());

        if (dir.exists("non-crossing-2-links-rrt-qt/non-crossing-2-links-rrt-qt.pro")) { // Test if /non-crossing-2-links-rrt-qt exists
            workingDir = workingDir.substr(0, indexOfDesiredDir) + "/non-crossing-2-links-rrt-qt";

            // Set current working directory to non-crossing-2-links-rrt-qt
            QDir::setCurrent(workingDir.c_str());
            foundFiles = true;
        }
    }

    // /non-crossing-2-links-rrt-qt could not be found
    if (!foundFiles) {
        std::cerr << std::endl << "!! WARNING !!\n"
        << "The program may not work correctly or at all because the folder "
        "containing the program's files cannot be found.\n"
        "Make sure that the program is inside of a folder named \"non-crossing-2-links-rrt-qt\".\n";
    }

    QApplication app(argc, argv);
    fp = fopen("debug.txt", "w");
    if(fp == NULL) return 0;

    parseExampleList();
    parseExampleFile();
    parseMapFile();
    window = new MainWindow();
    srand(seed);
    run();

    window->show();

    return app.exec();
}

int runCount = 1;
void run() {

    mw_out<<"Run No. "<<runCount++<<"\n";
    mw_out<<"- method="<<method<<"\n";
    mw_out<<"- non-crossing="<<non_crossing<<"\n";
    mw_out<<"- seed="<<seed<<"\n";
    mw_out<<"- start="<<start.x<<" "<<start.y<<" "<<start.t1<<" "<<start.t2<<"\n";
    mw_out<<"- goal="<<goal.x<<" "<<goal.y<<" "<<goal.t1<<" "<<goal.t2<<"\n";
    mw_out<<"- planner max sample size="<<(int)max_sample_size<<"\n";

    if(method=="prm" || method=="PRM" || method=="Prm"){
        mw_out<<"- prm connection k="<<(int)prm_closest_free_k<<"\n";
    }
    else if(method=="gauss" || method=="GAUSS" || method=="Gauss") {
        mw_out<<"- gaussian mean d="<<gauss_mean_d<<"\n";
        mw_out<<"- gaussian std="<<gauss_std<<"\n";
    }
    else if(method=="toggle" || method=="TOGGLE" || method=="Toggle") {
        mw_out<<"- toggle prm connection free k="<<(int)prm_closest_free_k<<"\n";
        mw_out<<"- toggle prm connection obst k="<<(int)prm_closest_obst_k<<"\n";
    }
    else if(method=="lazytoggle" || method=="LAZYTOGGLE" || method=="LazyToggle") {
        mw_out<<"- lazy toggle prm connection free k="<<(int)prm_closest_free_k<<"\n";
        mw_out<<"- lazy toggle prm connection obst k="<<(int)prm_closest_obst_k<<"\n";
    }
    else if(method=="rrt" || method=="RRT" || method=="Rrt") {
        mw_out<<"- rrt step size="<<rrt_step_size<<"\n";
        mw_out<<"- rrt goal bias="<<rrt_bias<<"\n";
        mw_out<<"- rrt close to goal="<<rrt_close_to_goal<<"\n";
    }
    mw_out<<"- l1="<<l1<<"\n";
    mw_out<<"- l2="<<l2<<"\n";
    mw_out<<"- thickness="<<thickness<<"\n";

    Timer t; t.start();
    // init robot
    planner->getRobot().init(l1, l2, thickness, start);
    // find the path
    start = planner->to_parametric(start);
    goal = planner->to_parametric(goal);
    noPath = !planner->findPath(start, goal);
    elapsedTime = t.getElapsedMilliseconds();
    elapsedCPUTime = t.getElapsedCPUMilliseconds();
    window->reportTime();

    if(!noPath) {
        planner->getRobot().setCFG(start);
        vector<CFG> path = planner->getPath();
        mw_out<<"- Path found ("<<"length = "<<(int)path.size()<<")\n";
    }
    else {
        mw_out<<"! Path not found\n";
    }
    mw_out<<"end\n\n";
}
