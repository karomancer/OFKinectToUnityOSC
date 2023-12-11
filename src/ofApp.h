// ofApp.h
#pragma once

#include "ofMain.h"
#include "ofxKinectV2.h"
#include "ofxGui.h"
#include "ofxCv.h"

class ofApp : public ofBaseApp
{
public:
    void setup();
    void update();
    void updateBlobs();
    void updateCanvas();
    void updateOutlines();
    void pruneTrackingObjects();
    void draw();
    
    ofxKinectV2 kinect;
    ofTexture irTex;
    ofPixels irPixels;
    ofTexture depthTex;
    ofPixels depthPixels;
    
    ofRectangle drawBounds;
    ofRectangle drawBoundsTopLeft, drawBoundsTopRight, drawBoundsBottomLeft, drawBoundsBottomRight;
    
    ofFbo canvasFbo;
    ofFbo visionFbo;
    ofFbo irFbo;
    ofFbo depthFbo;
    ofPixels blobPixels;
    ofImage blobImage;
    
    // For now, just one
    std::map<int, cv::Rect> prevNagiTrackingMap;
    std::map<int, cv::Rect> nagiTrackingMap;
    // We're going to be using these to determine
    // which objects we're still tracking
    // (and idsToDelete is to send a delete message to the other side)
    std::vector<int> presentIds = {};
    std::vector<int> idsToDelete = {};
    
    ofxCv::ContourFinder contourFinder;
    
    ofParameter<bool> showDebugGrid;
    ofParameter<bool> showIntermediary;
    
    // Controls for Kinect
    ofxGuiGroup kinectGuiGroup;
    ofParameter<bool> showDepthMap;
    ofParameter<float> minDepth;
    ofParameter<float> maxDepth;
    ofParameter<int> anchorDepth;
    ofParameter<bool> showIRMap;
    ofParameter<int> minIR;
    ofParameter<int> maxIR;
    
    // Controls for Contour Finder
    ofxGuiGroup contourFinderGuiGroup;
    ofParameter<bool> showContours;
    ofParameter<float> minContourArea;
    ofParameter<float> maxContourArea;
    ofParameter<int> persistence;
    
    ofParameter<int> millisToClear;
    int ellapsedMillisSinceClear;
    
    ofxPanel guiPanel;
    
    
};
