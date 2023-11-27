// ofApp.h
#pragma once

#include "ofMain.h"
#include "ofxKinectV2.h"
#include "ofxCv.h"
#include "ofxOsc.h"
#include "ofxGui.h"

struct Player {
    int label;
    int x;
    int y;
};

class ofApp : public ofBaseApp
{
public:
    void setup();
    void update();
    void draw();
    void updateCanvas();
    void updateOutlines();
    void pruneTrackingObjects();
    void sendObjectMoveMsg(int label, cv::Rect boundingRect, ofColor color);
    void sendObjectDeleteMsg(int label);
    
    Player player;
    
    ofxOscSender oscSender;
    
    ofxKinectV2 kinect;    
    ofTexture depthTex;
    ofPixels depthPixels;

    ofFbo canvasFbo;
    ofFbo visionFbo;
    ofPixels canvasPixels;
    
    ofxCv::ContourFinder contourFinder;
    ofxCv::RectTracker tracker;
    std::map<int, cv::Rect> idTrackingMap;
    
    // GUI Panel
    ofxPanel guiPanel;
    
    ofParameter<bool> sendObjectMsgs;
    ofParameter<bool> sendPlayerMsgs;
    
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
    
    // Controls of Player thresholds
    ofxGuiGroup playerMovementGuiGroup;
    ofParameter<int> jumpThreshold;
    ofParameter<int> slideThreshold;
    ofParameter<int> moveThreshold;
    
    ofRectangle drawBounds;
    
    // We're going to be using these to determine
    // which objects we're still tracking
    // (and idsToDelete is to send a delete message to the other side)
    std::vector<int> presentIds = {};
    std::vector<int> idsToDelete = {};
};
