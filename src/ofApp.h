// ofApp.h
#pragma once

#include "ofMain.h"
#include "ofxKinectV2.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp
{
public:
    void setup();
    void update();
    void draw();
    
    ofxKinectV2 kinect;
    ofTexture irTex;    
    ofFloatPixels irPixels;
    ofTexture depthTex;    
    ofPixels depthPixels;
    
    ofRectangle drawBounds;
    
    ofFbo canvasFbo;
    ofTexture canvasTexture;    
    
    ofParameter<float> minDepth;
    ofParameter<float> maxDepth;
    ofParameter<float> minIR;
    ofParameter<float> maxIR;
    ofParameter<float> anchorDepth;
    ofParameter<int> millisToClear;
    
    ofParameter<bool> showDepthMap;
    ofParameter<bool> showIRMap;
    
    ofxPanel guiPanel;
    
    int ellapsedMillisSinceClear;
};
