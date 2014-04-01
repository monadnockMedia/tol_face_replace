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
 //   ofSetFrameRate(60);
    
    //setup fbos
    canvas.allocate(600, 600);
    srcFbo.allocate(600,600);
    tgFbo.allocate(600,600);
   // srcMaskFbo.allocate(600,600);
   
    //setup trackers
    srcTracker.setup();
    srcTracker.setIterations(25);
    srcTracker.setAttempts(4);
    
   // tgTracker.setup();
   // tgTracker.setIterations(25);
   // tgTracker.setAttempts(4);
    
    buildTargets();
    
}

//--------------------------------------------------------------
void testApp::update(){
     if(loadimg) setupSrcs(0);
}

//--------------------------------------------------------------
void testApp::draw(){
    

   
    
    tgDBFbo.draw(0,0);
    srcDBFbo.draw(600,0,400,400);
    
    if(cloned){
        //clone.draw(0,0);
        canvas.draw(0,400,600,600);
        dbFbo.draw(600,400,600,600);}
    
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
        
    }
    
    string settings = ofFile("targets.json").readToBuffer();
    bool read = reader.parse(settings, tgSettings);
    cout << tgSettings.toStyledString() << endl;
    for (int i = 0; i< tgSettings.size(); i++){  //for each target image
        
        cout << "settings index : " << i <<  "   URI: " << tgSettings[i][0]["img_uri"].asString() <<endl;
        
        vector<ofMesh> tmpBin;
        
        ofLoadImage(tgPixels[i], tgSettings[i][0]["img_uri"].asString());  //load the target image to the pixel array
        
        for (int j = 0; j<tgSettings[i].size(); j++){ //for each face
            ofMesh tmpMesh;
            cout << "\t child index : " << j << endl;
            tmpMesh.load("mesh/"+tgSettings[i][j]["mesh_uri"].asString());
            tmpBin.push_back(tmpMesh);
            tmpMesh.clear();
        }
        tgMeshes.push_back(tmpBin);
        
    }
    
    
}

void testApp::setupTarget(int tgID){
    currentTg  = tgID;
    string fn = tgSettings[tgID][0]["img_uri"].asString();
    cout << "loading " << fn<<endl;
    tgIMG.setFromPixels(tgPixels[tgID]);
    tgFbo.allocate(tgIMG.getWidth(),tgIMG.getHeight());
    
    
    /*tgTracker.update(toCv(tgIMG));
    if(tgTracker.getFound()){
        tgMesh = tgTracker.getImageMesh();
    }else{
        messages.push_back("could not find face in target");
    }*/
    

    
    tgFbo.begin();
    tgIMG.draw(0,0);
    tgFbo.end();
    
    tgDBFbo.allocate(tgIMG.height,tgIMG.width);
    tgDBFbo.begin();
    ofClear(0,255);

    tgIMG.draw(0,0);

    ofNoFill();
    ofSetColor(255, 255, 255);
    tgMesh.drawWireframe();
    ofSetColor(255,255,255);
    tgDBFbo.end();
}




void testApp::setupSrcs(int srcID){
    srcPx.clear();
    
    loadimg = false;
    sources = imageObject["sources"];
    cout <<"///SOURCES"<< sources.toStyledString() << endl;
    cout << "///tgImg "<< sources[srcID]["filename"].asString() << endl;;
    string dir = imageObject["dir"].asString();
   
    string ext = imageObject["ext"].asString();
    
    string time = ofGetTimestampString();
    string tmpfile = dir+time+"tracked."+ext;
    imageObject["origin"] = "cvserver";
    imageObject["processedImage"] = tmpfile;

    int tgID = imageObject["tgImageID"].asInt();
    cout <<"//TARGET ID  "<<tgID<<endl;
    
    srcPx.resize(sources.size());
    
    for(int i = 0; i < sources.size(); i++){
        
        string fname = sources[i]["filename"].asString()+"."+ext;
        string url = dir+fname;
        cout <<"image url "<<url<<endl;
        ofPixels p;
        ofLoadImage(p, url);
        int j = sources[i]["faceID"].asInt();
        cout <<"image index "<<j<<endl;
        srcPx[j] = p;
        cout << "SOURCEPX  :  " << srcPx[0].size() << endl;
     }
    
    
    
    setupTarget(tgID);
    cloneIMGs();
}

void testApp::updateSrc(int srcID){
    srcIMG.setFromPixels(srcPx[srcID]);
    //srcIMG.loadImage(url);
    srcFbo.allocate(srcIMG.height, srcIMG.width);
    srcTracker.update(toCv(srcIMG));
    srcPoints = srcTracker.getImagePoints();
    srcMesh = makeMesh(srcTracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE));
    srcPoints = srcMesh.getTexCoords();
}

vector<ofVec2f> vec3D2vec2D(vector<ofVec3f> &v){
    vector<ofVec2f> pts;
    for(int i = 0; i<v.size();i++){
        pts.push_back(ofVec2f(v[i][0],v[i][1]));
    }
    pts.push_back(ofVec2f(v[0][0],v[0][1]));
    return pts;
}

/*ofMesh testApp::makeMesh(ofPolyline line){
    
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);
    triangle.clear();
    triangle.triangulate(line.getVertices());
    cout << "triangulated" <<endl;
    
   //for(int i = triangle.nTriangles; i>=0;  i--){
    for(int i = 0; i< triangle.nTriangles;  i++){
        
        mesh.addVertex(ofVec2f(triangle.triangles[i].a.x ,triangle.triangles[i].a.y));
       mesh.addTexCoord(ofVec2f(triangle.triangles[i].a.x ,triangle.triangles[i].a.y));
        
        mesh.addVertex(ofVec2f(triangle.triangles[i].b.x ,triangle.triangles[i].b.y));
      mesh.addTexCoord(ofVec2f(triangle.triangles[i].b.x ,triangle.triangles[i].b.y));

        mesh.addVertex(ofVec2f(triangle.triangles[i].c.x ,triangle.triangles[i].c.y));
      mesh.addTexCoord(ofVec2f(triangle.triangles[i].c.x ,triangle.triangles[i].c.y));
  

    }
    return mesh;

}*/

ofMesh testApp::makeMesh(ofPolyline line){
    
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    //for(int i = triangle.nTriangles; i>=0;  i--){
    int l = line.size();
    for(int i = 0; i< l;  i++){
        
        int j = (i +(18) )%l;
        
        mesh.addVertex(line.getPointAtIndexInterpolated(j));
        mesh.addTexCoord(line.getPointAtIndexInterpolated(j));
        
     
        
        
    }
    return mesh;
    
}



void testApp::sendImage(){
    string fname = imageObject["processedImage"].asString();
    mashIMG.saveImage(fname);
    client.send(writer.write(imageObject));
    cout << "saved Image  :: " << fname << endl;
    
}

void testApp::cloneIMGs(){
    
    
    clone.setup(tgIMG.getWidth(), tgIMG.getHeight());
    clone.setStrength(30);
  
    
    tgMesh = tgMeshes[currentTg][0];
    updateSrc(0);
    
    tgMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    tgMesh.clearTexCoords();
    tgMesh.addTexCoords(srcPoints);
    
    srcDBFbo.allocate(srcIMG.width, srcIMG.height);
    srcDBFbo.begin();
    
    ofClear(0,255);
    ofSetColor(255,255,255);
    srcIMG.bind();
   
    srcMesh.draw();
    srcIMG.unbind();
    srcMesh.drawWireframe();
    ofSetColor(255,0,0);
    
    vector<ofVec3f> srcV = srcMesh.getVertices();
    
    for(int i = 0; i< srcV.size(); i++){
        ofDrawBitmapString(ofToString(i), srcV[i].x, srcV[i].y );
    }
    ofSetColor(255,255,255);
    srcDBFbo.end();
    
    
    //vector<ofVec3f> tgPTS = tgTracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE).getVertices();
    //vector<ofVec2f> tgPTS2D = vec3D2vec2D(tgPTS);
    //ofMesh tgTestMesh = makeMesh(tgPTS);
    
   // vector<ofVec3f> srcPTS = srcTracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE).getVertices();
    //vector<ofVec2f> srcPTS2D = vec3D2vec2D(srcPTS);
    
    
    
    //ofMesh tgTestMesh = tgMesh;
    //tgTestMesh.clearTexCoords();
    //tgTestMesh.addTexCoords(srcPoints);
    
    ofShader bw;
    bw.load("bw");
    
    srcFbo.allocate(tgIMG.getWidth(), tgIMG.getHeight());
    srcFbo.begin();
    bw.begin();
    bw.setUniformTexture("tex", srcIMG.getTextureReference(),1);
    ofClear(0,255);
    srcIMG.bind();
    //tgMesh.draw(); //default
    //tgTestMesh.draw();
    tgMesh.draw();
    srcIMG.unbind();
    bw.end();
    //tgMesh.drawWireframe();
    srcFbo.end();
    
    
   
    
    
    
    
    
    tgMaskFbo.allocate(tgIMG.getWidth(), tgIMG.getHeight());
    tgMaskFbo.begin();
    ofClear(0, 255);
    //path.draw();
    //tgTestMesh.draw();
    tgMesh.draw();
    //tgMesh.draw(); //this is default
    tgMaskFbo.end();
    
    
    
    
    
    
    
    clone.update(srcFbo.getTextureReference(),tgFbo.getTextureReference(),tgMaskFbo.getTextureReference());


    
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