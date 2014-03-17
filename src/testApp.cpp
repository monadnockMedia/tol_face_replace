#include "testApp.h"
using namespace ofxCv;

char bwShaderSrc [] =
"#extension GL_ARB_texture_rectangle : enable\n"
"uniform sampler2DRect tex;\
void main()\
{\
	//Multiply color by texture\
	vec4 color = gl_Color * texture2DRect(tex, gl_TexCoord[0].xy);\
	float gray = dot(color.rgb, vec3(0.299, 0.587, 0.314));\
    gl_FragColor = vec4(gray, gray, gray, gl_Color.a);\n\
}";
//--------------------------------------------------------------
void testApp::setup(){
    // setup a server with default options on port 9092
    // - pass in true after port to set up with SSL
    //bConnected = server.setup( 9093 );

    ofxLibwebsockets::ServerOptions options = ofxLibwebsockets::defaultServerOptions();
    options.port = 9092;
	options.bUseSSL = false; //ssl not working on Win right now!
    bConnected = server.setup( options );
    
	cout << "CONNECTED? "<<bConnected<<endl;
    
    // this adds your app as a listener for the server
    server.addListener(this);
    
    ofBackground(0);
    ofSetFrameRate(60);
    
    //setup fbos
    canvas.allocate(600, 600);
    srcFbo.allocate(600,600);
    tgFbo.allocate(600,600);
    srcMaskFbo.allocate(600,600);
   
    //setup trackers
    srcTracker.setup();
    srcTracker.setIterations(25);
    srcTracker.setAttempts(4);
    
    tgTracker.setup();
    tgTracker.setIterations(25);
    tgTracker.setAttempts(4);
    
    buildTargets();
    
}

//--------------------------------------------------------------
void testApp::update(){
     if(loadimg) setupSrc(0);
}

//--------------------------------------------------------------
void testApp::draw(){
    

   
    
  //  srcFbo.draw(0,600, 600,600);
    
   // srcFbo.draw(0,0,600,600);
    //tgMaskFbo.draw(600,0,600,600);
  
     //tgFbo.draw(0,0,600,600);
    
    if(cloned){
        //clone.draw(0,0);
        canvas.draw(0,600,600,600);   }
        dbFbo.draw(600,600,600,600);
    
    if ( bConnected ){
        ofDrawBitmapString("WebSocket server setup at "+ofToString( server.getPort() ) + ( server.usingSSL() ? " with SSL" : " without SSL"), 20, 20);
        
        ofSetColor(150);
        
    } else {
        ofDrawBitmapString("WebSocket setup failed :(", 20,20);
    }
    
    int x = 20;
    int y = 100;
    
    
    ofSetColor(255);
    for (int i = messages.size() -1; i >= 0; i-- ){
        ofDrawBitmapString( messages[i], x, y );
        y += 20;
    }
    if (messages.size() > NUM_MESSAGES) messages.erase( messages.begin() );
    //if(loadimg) drawImage();
   
 
    
    
   }

void testApp::buildTargets(){
    for (int i = 0; i< tgNum; i++ ){
        ofLoadImage(tgPixels[i], tgNames[i]+".jpg");
    }
}

void testApp::setupTarget(int tgID){
    
    string fn = tgNames[tgID]+".jpg";
    cout << "loading " << fn<<endl;
    
    tgIMG.setFromPixels(tgPixels[tgID]);
    //tgIMG.loadImage(fn);
    //tgIMG.update();
    tgFbo.allocate(tgIMG.getWidth(),tgIMG.getHeight());
    
    
    tgTracker.update(toCv(tgIMG));
    if(tgTracker.getFound()){
        tgMesh = tgTracker.getImageMesh();
    }else{
        messages.push_back("could not find face in target");
    }

    
    tgFbo.begin();
    tgIMG.draw(0,0);
   // tgMesh.drawWireframe();
  //  tgMesh.drawWireframe();
    tgFbo.end();
}




void testApp::setupSrc(int srcID){
    loadimg = false;
    
    Json::Value sources = imageObject["sources"];
    cout <<"///SOURCES"<< sources.toStyledString() << endl;
    cout << "///tgImg "<< sources[srcID]["filename"].asString() << endl;;
    string dir = imageObject["dir"].asString();
    string filename = sources[srcID]["filename"].asString();
    string ext = imageObject["ext"].asString();
    string url = dir+filename+"."+ext;
    cout <<"image url "<<url<<endl;
    string time = ofGetTimestampString();
    string tmpfile = dir+filename+time+"tracked."+ext;
    imageObject["origin"] = "cvserver";
    imageObject["processedImage"] = tmpfile;

    int tgID = imageObject["tgImageID"].asInt();
    cout <<"//TARGET ID  "<<tgID<<endl;
    setupTarget(tgID);
    
    srcIMG.loadImage(url);
    srcIMG.update();
    
    ofShader bw;
    bw.setupShaderFromSource(GL_FRAGMENT_SHADER, bwShaderSrc);
    bw.linkProgram();
    
    srcFbo.allocate(srcIMG.height, srcIMG.width);
    srcMaskFbo.allocate(srcIMG.width,srcIMG.height);
    ///////
    
    srcTracker.update(toCv(srcIMG));
    ofMesh srcMesh = srcTracker.getImageMesh();
    srcPoints = srcTracker.getImagePoints();
    
    
    
    
    srcFbo.begin();
    srcIMG.draw(0,0);
    //srcMesh.drawWireframe();
    srcFbo.end();
    
    

    
    srcMaskFbo.begin();
    ofClear(0, 255);
    srcMesh.draw();
    srcMaskFbo.end();
    
    cloneIMGs();
}

ofMesh testApp::makeMesh( vector<ofVec3f> & pts){
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    
    for(int i = 0; i < pts.size(); i++){
        mesh.addVertex(pts[i]);
    }
    return mesh;
}

vector<ofVec2f> vec3D2vec2D(vector<ofVec3f> &v){
    vector<ofVec2f> pts;
    for(int i = 0; i<v.size();i++){
        pts.push_back(ofVec2f(v[i][0],v[i][1]));
    }
    return pts;
}

void testApp::sendImage(){
    string fname = imageObject["processedImage"].asString();
    mashIMG.saveImage(fname);
    client.send(writer.write(imageObject));
    cout << "saved Image  :: " << fname << endl;
    
}

void testApp::cloneIMGs(){
    
    string fname = imageObject["processedImage"].asString();
    
    
    clone.setup(tgIMG.getWidth(), tgIMG.getHeight());
    clone.setStrength(30);
    
    tgMesh.clearTexCoords();
    tgMesh.addTexCoords(srcPoints);
    
    
    vector<ofVec3f> tgPTS = tgTracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE).getVertices();
    vector<ofVec2f> tgPTS2D = vec3D2vec2D(tgPTS);
    ofMesh tgTestMesh = makeMesh(tgPTS);
    
    vector<ofVec3f> srcPTS = srcTracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE).getVertices();
    vector<ofVec2f> srcPTS2D = vec3D2vec2D(srcPTS);

    
    
    
    tgTestMesh.clearTexCoords();
    tgTestMesh.addTexCoords(srcPTS2D);
    
    ofShader bw;
    bw.load("bw");
    //bw.setupShaderFromSource(GL_FRAGMENT_SHADER,bwShaderSrc);
   // bw.setUniformTexture("tex", srcIMG, 0);
   // bw.linkProgram();
    
    dbFbo.allocate(srcIMG.getWidth(), srcIMG.getHeight());
    dbFbo.begin();
  
     bw.begin();
     bw.setUniformTexture("tex", srcIMG.getTextureReference(),1);
     srcIMG.draw(0,0);
     bw.end();
    srcIMG.draw(0,0,200,200);

    dbFbo.end();
    
    srcFbo.allocate(tgIMG.getWidth(), tgIMG.getHeight());
    srcFbo.begin();
    ofClear(0,255);
    srcIMG.bind();
    //tgMesh.draw(); //default
    tgTestMesh.draw();
    srcIMG.unbind();
    srcFbo.end();
    
    
   
    
    
    
    
    
    tgMaskFbo.allocate(tgIMG.getWidth(), tgIMG.getHeight());
    tgMaskFbo.begin();
    ofClear(0, 255);
    //path.draw();
    tgTestMesh.draw();
    //tgMesh.draw(); //this is default
    tgMaskFbo.end();
    
    
    
    
    
    
    
    clone.update(srcFbo.getTextureReference(),tgFbo.getTextureReference(),tgMaskFbo.getTextureReference());

   // dbFbo.allocate(tgIMG.getWidth(), tgIMG.getHeight());
 /*   dbFbo.begin();
    tgIMG.draw(0,0);
    //clone.draw(0,0); //default, only draw needed in the end.
    //path.draw();
    //tgMaskFbo.draw(0,0);
    
    srcIMG.bind();
    //tgMesh.draw(); //default
    tgTestMesh.draw();

    //testMesh.drawWireframe();
    dbFbo.end(); */
    
    canvas.allocate(tgIMG.getWidth(), tgIMG.getHeight());
    canvas.begin();
    clone.draw(0,0);
    mashIMG.grabScreen(0,0,canvas.getWidth(),canvas.getHeight());
    mashIMG.rotate90(2);
    mashIMG.mirror(0,1);

    canvas.end();
    
    
    
    
    
    
    sendImage();
    cloned = true;
    
    
}



void testApp::loadPoints(string filename) {
	ofFile file;
	file.open(ofToDataPath(filename), ofFile::ReadWrite, false);
	ofBuffer buff = file.readToBuffer();
	
	// Discard the header line.
	if (!buff.isLastLine()) buff.getNextLine();
	
	srcPoints = vector<ofVec2f>();
	
	while (!buff.isLastLine()) {
		string line = buff.getNextLine();
		vector<string> tokens = ofSplitString(line, "\t");
		srcPoints.push_back(ofVec2f(ofToFloat(tokens[0]), ofToFloat(tokens[1])));
	}
	cout << "Read " << filename << "." << endl;
}


//--------------------------------------------------------------
void testApp::onConnect( ofxLibwebsockets::Event& args ){
    cout<<"on connected"<<endl;
}

//--------------------------------------------------------------
void testApp::onOpen( ofxLibwebsockets::Event& args ){
    cout<<"new connection open"<<endl;
    messages.push_back("New connection from " + args.conn.getClientIP() + ", " + args.conn.getClientName() );
}

//--------------------------------------------------------------
void testApp::onClose( ofxLibwebsockets::Event& args ){
    cout<<"on close"<<endl;
    messages.push_back("Connection closed");
}

//--------------------------------------------------------------
void testApp::onIdle( ofxLibwebsockets::Event& args ){
    cout<<"on idle"<<endl;
}

//--------------------------------------------------------------
void testApp::onMessage( ofxLibwebsockets::Event& args ){
    cout<<"got message "<<args.message<<endl;
    
    // trace out string messages or JSON messages!
    if ( !args.json.isNull() ){
        cout<<"///got JSON "<<args.message<<endl;
        imageObject = args.json;
        messages.push_back(imageObject["command"].asString());
        loadimg = true;
        client = args.conn;
        loadimg = true;
      //  messages.push_back("New json message: " + args.json.toStyledString() + " from " + args.conn.getClientName() );
    } else {
        messages.push_back("New message: " + args.message + " from " + args.conn.getClientName() );
    }
        
    // echo server = send message right back!
   // args.conn.send( args.message );
}

//--------------------------------------------------------------
void testApp::onBroadcast( ofxLibwebsockets::Event& args ){
    cout<<"got broadcast "<<args.message<<endl;    
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    // do some typing!
    if ( key != OF_KEY_RETURN ){
        if ( key == OF_KEY_BACKSPACE ){
            if ( toSend.length() > 0 ){
                toSend.erase(toSend.end()-1);
            }
        } else {
            toSend += key;
        }
    } else {
        // send to all clients
        server.send( toSend );
        messages.push_back("Sent: '" + toSend + "' to "+ ofToString(server.getConnections().size())+" websockets" );
        toSend = "";
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    string url = "http";
    if ( server.usingSSL() ){
        url += "s";
    }
    url += "://localhost:" + ofToString( server.getPort() );
    ofLaunchBrowser(url);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}