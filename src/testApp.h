#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "Clone.h"
#include "ofxFaceTracker.h"
#include "ofxLibwebsockets.h"

#include "json.h"
#include "ofxTriangle.h"


#define NUM_MESSAGES 30 // how many past messages we want to keep

class testApp : public ofBaseApp{
    bool debug = true;
	public:
		void setup();
		void update();
		void draw();
    void drawImage();
    void returnImage(string fname);
    void loadPoints(string filename);
    bool updateSrc(int srcID);
    void keyPressed  (int key);
    void sendImage();
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        void cloneIMGs();

        void setupSrcs(int srcID);
        void setupTarget(int tgID);
        void buildTargets();
    
        ofxLibwebsockets::Server server;
    
        bool bConnected;
    
        //queue of rec'd messages
        vector<string> messages;
    
        //string to send to clients
        string toSend;
    
        // websocket methods
        void onConnect( ofxLibwebsockets::Event& args );
        void onOpen( ofxLibwebsockets::Event& args );
        void onClose( ofxLibwebsockets::Event& args );
        void onIdle( ofxLibwebsockets::Event& args );
        void onMessage( ofxLibwebsockets::Event& args );
        void onBroadcast( ofxLibwebsockets::Event& args );
    ofMesh makeMesh( ofPolyline line );

    ofImage tgIMG, srcIMG, mashIMG;
    bool loadimg = false;
    bool cloned = false;
    Json::Value imageObject,tgSettings,sources;
    Json::FastWriter writer;
    Json::Reader reader;
    
    ofxLibwebsockets::Connection client;
 
    int tgNum = 6, currentTg;
    string tgNames [6] = {"0_lifering", "1_chef","2_crew","3_walk","4_couples","5_kids" };
    ofPixels tgPixels [6];
    vector<ofPixels> srcPx;
    
    
    ofxFaceTracker srcTracker,tgTracker;
	ofImage src;
	vector<ofVec2f> srcPoints, tgPoints;
    
    ofMesh tgMesh, srcMesh;
    vector<vector <ofMesh> > tgMeshes;
    vector<ofFbo> DBFbos;
    ofFbo canvas, srcFbo, tgFbo, srcMaskFbo,tgMaskFbo, dbFbo, tgDBFbo, srcDBFbo;
    Clone clone;
    ofxTriangle triangle;
    
    
};
