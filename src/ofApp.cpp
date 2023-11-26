// ofApp.cpp
#include "ofApp.h"

void ofApp::setup()
{
    int width = ofGetScreenWidth();
    int height = ofGetScreenHeight();
    ofSetWindowShape(width, height);
    
    // Set up canvas
    ofBackground(255);
    
    // Kinect GUI options
    kinectGuiGroup.setup("Kinect");
    kinectGuiGroup.add(showDepthMap.set("Show Kinect Depth Map", false));
    kinectGuiGroup.add(minDepth.set("Min depth", 0.5f, 0.5f, 10.f));
    kinectGuiGroup.add(maxDepth.set("Max depth", 1.3f, 0.5f, 10.f));
    kinectGuiGroup.add(anchorDepth.set("Base pixel size", 1, 1, 5));
    
    // Set up Kinect
    ofxKinectV2::Settings kinectSettings;
    kinectSettings.enableRGB = false;
    kinectSettings.enableIR = false;
    kinectSettings.enableRGBRegistration = false;
    kinectSettings.config.MinDepth = minDepth;
    kinectSettings.config.MaxDepth = maxDepth;
    kinect.open(0, kinectSettings);
    
    // Contour finder GUI options
    contourFinderGuiGroup.setup("Contour Finder");
    contourFinderGuiGroup.add(minContourArea.set("Min area", 0.01f, 0, 0.5f));
    contourFinderGuiGroup.add(maxContourArea.set("Max area", 0.05f, 0, 0.5f));
    
    // Set up GUI panel
    guiPanel.setup("People Detection Settings", "settings.json");
    guiPanel.add(&kinectGuiGroup);
    guiPanel.add(&contourFinderGuiGroup);
}

void ofApp::update()
{
    kinect.update();
    
    // Only load the data if there is a new frame to process.
    if (kinect.isFrameNew())
    {
        depthPixels = kinect.getDepthPixels();
        depthTex.loadData(depthPixels);
        
        int depthWidth = depthPixels.getWidth();
        int depthHeight = depthPixels.getHeight();
        
        ofFboSettings fboSettings;
        fboSettings.width = depthWidth;
        fboSettings.height = depthHeight;
        visionFbo.allocate(fboSettings);
        canvasFbo.allocate(fboSettings);
        
        drawBounds.set(0, 0, depthWidth, depthHeight);
        drawBounds.scaleTo(ofGetCurrentViewport(), OF_SCALEMODE_FILL);
        
        // Drawing the canvas
        canvasFbo.begin();
        ofSetColor(ofColor(0, 0, 0));
        ofFill();
        for (int y = 0; y < depthPixels.getHeight(); y += anchorDepth) {
            for (int x = 0; x < depthPixels.getWidth(); x += anchorDepth) {
                float dist = kinect.getDistanceAt(x, y);
                
                if (dist > minDepth && dist < maxDepth) {
                    float radius = ofMap(dist, minDepth, maxDepth, anchorDepth, 0.5f);
                    ofDrawCircle(x + anchorDepth, y + anchorDepth, radius);
                }
            }
        }
        canvasFbo.end();
        
        // Drawing the contour
        contourFinder.setMinAreaNorm(minContourArea);
        contourFinder.setMaxAreaNorm(maxContourArea);
        contourFinder.findContours(depthPixels);
        
        // Draw the contour in its own FBO
        visionFbo.begin();
        ofSetColor(ofColor::red);
        contourFinder.draw();
        ofSetColor(255);
        visionFbo.end();
    }
}

void ofApp::draw()
{
    if (showDepthMap) {
        ofSetColor(255);
        ofFill();
        depthTex.draw(drawBounds);
        
        //   Get the point distance using the SDK function (in meters).
        float distAtMouse = kinect.getDistanceAt(ofGetMouseX(), ofGetMouseY());
        ofDrawBitmapStringHighlight(ofToString(distAtMouse, 3), ofGetMouseX(), ofGetMouseY() - 10);
        
        // Get the point depth using the texture directly (in millimeters).
        const ofFloatPixels& rawDepthPix = kinect.getRawDepthPixels();
        int depthAtMouse = rawDepthPix.getColor(ofGetMouseX(), ofGetMouseY()).r;
        ofDrawBitmapStringHighlight(ofToString(depthAtMouse), ofGetMouseX() + 16, ofGetMouseY() + 10);
    }
    canvasFbo.draw(drawBounds);
    visionFbo.draw(drawBounds);
    guiPanel.draw();
}
