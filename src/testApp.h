#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "Clone.h"
#include "ofxFaceTracker.h"
#include "ofxLibwebsockets.h"

#include "json.h"



#define NUM_MESSAGES 30 // how many past messages we want to keep

class testApp : public ofBaseApp{
    int debug = 1;
	public:
		void setup();
		void update();
		void draw();
    void drawImage();
    void returnImage(string fname);
    void loadPoints(string filename);
    
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
    
        void setupSrc(int srcID);
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
    ofMesh makeMesh( vector<ofVec3f> & pts );

    ofImage tgIMG, srcIMG, mashIMG;
    bool loadimg = false;
    bool cloned = false;
    Json::Value imageObject;
    Json::FastWriter writer;
    ofxLibwebsockets::Connection client;
 
    int tgNum = 6;
    string tgNames [6] = {"0_lifering", "1_chef","2_crew","3_walk","4_couples","5_kids" };
    ofPixels tgPixels [6];
    
    ofxFaceTracker srcTracker,tgTracker;
	ofImage src;
	vector<ofVec2f> srcPoints, tgPoints;
    
    ofMesh tgMesh, srcMesh;
    ofFbo canvas, srcFbo, tgFbo, srcMaskFbo,tgMaskFbo, dbFbo;
    Clone clone;
    
};
