// ofApp.cpp
#include "ofApp.h"

const int KINECT_DEPTH_WIDTH = 512;
const int KINECT_DEPTH_HEIGHT = 424;

void ofApp::setup()
{
    int width = ofGetScreenWidth();
    int height = ofGetScreenHeight();
    ofSetWindowShape(width, height);
    
    // Set up canvas
    ofBackground(255);
    
    drawBounds.set(0, 0, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
    drawBounds.scaleTo(ofGetCurrentViewport(), OF_SCALEMODE_FILL);
    
    // Set up frame buffer
    ofFboSettings fboSettings;
    fboSettings.width = width;
    fboSettings.height = height;
    canvasFbo.allocate(fboSettings);
    
    // Set up GUI panel
    guiPanel.setup("DEPTH_DOTS", "settings.json");
    minIR.set("Min IR value", 0.5f, 0.f, 1.f);
    maxIR.set("Max IR value", 0.5f, 0.f, 1.f);
    minDepth.set("Min depth", 0.5f, 0.5f, 8.f);
    maxDepth.set("Max depth", 1.3f, 0.5f, 8.f);
    millisToClear.set("Ms to clear", 200, 200, 10000);
    anchorDepth.set("Base pixel size", 1.f, 1.f, 10.f);
    
    showDepthMap.set("Show Kinect Depth Map", false);
    showIRMap.set("Show Kinect IR Map", false);
    
    guiPanel.add(showDepthMap);
    guiPanel.add(showIRMap);
    guiPanel.add(minDepth);
    guiPanel.add(maxDepth);
    guiPanel.add(minIR);
    guiPanel.add(maxIR);
    guiPanel.add(anchorDepth);
    guiPanel.add(millisToClear);
    guiPanel.loadFromFile("settings.json");
    
    // Set up Kinect
    ofxKinectV2::Settings kinectSettings;
    kinectSettings.enableRGB = false;
    kinectSettings.enableIR = true;
    kinectSettings.enableRGBRegistration = false;
    kinectSettings.config.MaxDepth = maxDepth;
    kinectSettings.config.MinDepth = minDepth;
    kinect.open(0, kinectSettings);
}

void ofApp::update()
{
    kinect.update();
    
    // Only load the data if there is a new frame to process.
    if (kinect.isFrameNew())
    {
        depthPixels = kinect.getDepthPixels();
        depthTex.loadData(depthPixels);
        
        irPixels = kinect.getIRPixels();
        irTex.loadData(irPixels);
        
        canvasFbo.begin();
        ofEnableAlphaBlending();
        if (ofGetElapsedTimeMillis() - ellapsedMillisSinceClear > millisToClear) {
            ellapsedMillisSinceClear = ofGetElapsedTimeMillis();
            ofClear(255);
            cout << "Clear" << endl;
        } else {
            ofSetColor(ofColor::black);
            ofFill();
            
            for (int y = 0; y < depthPixels.getHeight(); y++) {
                for (int x = 0; x < depthPixels.getWidth(); x++) {
                    float dist = kinect.getDistanceAt(x, y);
                    float ir = irPixels[x + (irTex.getHeight() * y)];
                    
                    if (dist > minDepth && dist < maxDepth) {
                        float radius = ofMap(dist, minDepth, maxDepth, anchorDepth * 2, 0);
                        ofSetColor(ofColor::black);
                        ofFill();
                        ofDrawCircle(x + anchorDepth, y + anchorDepth, radius);
                    }
                }
            }
        }
        ofEnableAlphaBlending();
        ofSetColor(255, 2);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        canvasFbo.end();
        
        ofPixels canvasPixels;
        canvasFbo.readToPixels(canvasPixels);
        canvasTexture.loadData(canvasPixels);
    }
    
}

void ofApp::draw()
{
    if (showIRMap) {
        ofSetColor(255);
        ofFill();
        
        irTex.draw(0, 0, ofGetScreenWidth(), ofGetScreenHeight());
    } else if (showDepthMap) {
        depthTex.draw(0, 0, ofGetScreenWidth(), ofGetScreenHeight());
    }
    else {
        canvasFbo.draw(drawBounds);
    }
    
    guiPanel.draw();
}
