// ofApp.h
#pragma once

#include "ofMain.h"
#include "ofxKinectV2.h"
#include "ofxCv.h"
#include "ofxOsc.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp
{
public:
    void setup();
    void update();
    void draw();
    
    ofxOscSender oscSender;
    
    ofxKinectV2 kinect;    
    ofTexture depthTex;
    ofPixels depthPixels;

    ofFbo canvasFbo;
    ofFbo visionFbo;
    ofPixels canvasPixels;
    
    ofxCv::ContourFinder contourFinder;
    
    // GUI Panel
    ofxPanel guiPanel;
    
    // Controls for Kinect
    ofxGuiGroup kinectGuiGroup;
    ofParameter<float> minDepth;
    ofParameter<float> maxDepth;
    ofParameter<int> anchorDepth;
    ofParameter<bool> showDepthMap;
    
    // Controls for Contour Finder
    ofxGuiGroup contourFinderGuiGroup;
    ofParameter<float> minContourArea;
    ofParameter<float> maxContourArea;
    ofParameter<int> persistence;
    
    ofRectangle drawBounds;
};
