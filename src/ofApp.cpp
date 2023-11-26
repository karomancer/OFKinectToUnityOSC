// ofApp.cpp
#include "ofApp.h"
#include "ofxCv.h"

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

    // Set up OSC sender
    oscSender.setup("localhost", 1337);

    // Kinect GUI options
    kinectGuiGroup.setup("Kinect");
    kinectGuiGroup.add(showDepthMap.set("Show Kinect Depth Map", false));
    kinectGuiGroup.add(minDepth.set("Min depth", 0.5f, 0.5f, 8.f));
    kinectGuiGroup.add(maxDepth.set("Max depth", 5.f, 0.5f, 8.f));
    kinectGuiGroup.add(anchorDepth.set("Base pixel size", 1, 1, 5));

    // Contour finder GUI options
    contourFinderGuiGroup.setup("Contour Finder");
    contourFinderGuiGroup.add(minContourArea.set("Min area", 0.01f, 0, 100.f));
    contourFinderGuiGroup.add(maxContourArea.set("Max area", 0.4f, 0, 300.f));
    contourFinderGuiGroup.add(persistence.set("Persistence", 15, 0, 300));

    // Set up GUI panel
    guiPanel.setup("SETTINGS", "settings.json");
    guiPanel.add(&kinectGuiGroup);
    guiPanel.add(&contourFinderGuiGroup);
    guiPanel.loadFromFile("settings.json");
    
    // Set up Kinect
    ofxKinectV2::Settings kinectSettings;
    kinectSettings.enableRGB = false;
    kinectSettings.enableIR = false;
    kinectSettings.enableRGBRegistration = false;
//    kinectSettings.config.MinDepth = minDepth;
//    kinectSettings.config.MaxDepth = maxDepth;
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
        
        canvasFbo.allocate(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
        visionFbo.allocate(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);

        // Drawing the canvas
        canvasFbo.begin();
        for (int y = 0; y < depthPixels.getHeight(); y+=2) {
            for (int x = 0; x < depthPixels.getWidth(); x+=2) {
                float dist = kinect.getDistanceAt(x, y);

                if (dist > minDepth && dist < maxDepth) {
                    int grayInt = ofMap(dist, minDepth, maxDepth, 255, 0);
                    float radius = ofMap(dist, minDepth, maxDepth, anchorDepth * 2, 0);
                    ofSetColor(ofColor(grayInt, grayInt, grayInt));
                    ofFill();
                    ofDrawCircle(x + anchorDepth, y + anchorDepth, radius);
                }
            }
        }
        canvasFbo.end();

        canvasFbo.readToPixels(canvasPixels);

        // Drawing the contour
        contourFinder.setMinAreaRadius(minContourArea);
        contourFinder.setMaxAreaRadius(maxContourArea);
        contourFinder.getTracker().setPersistence(persistence);
        contourFinder.findContours(showDepthMap ? depthPixels : canvasPixels);
        std::vector<cv::Rect> blobs = contourFinder.getBoundingRects();
        cout << "num blobs " << blobs.size() << endl;
        
        // Draw the contour in its own FBO
        visionFbo.begin();
        ofClear(255);
        ofSetColor(ofColor::white);
        ofNoFill();
        for(int i = 0; i < blobs.size(); i++) {
            ofColor color = ofColor::red;
            color.setHueAngle(color.getHueAngle() + i * 30);
            ofSetColor(color);
            cv::Rect boundingRect = blobs[i];
            ofDrawRectangle(boundingRect.x, boundingRect.y, boundingRect.width, boundingRect.height);
//            cout << "What the hell " << boundingRect.x << " " << boundingRect.y << endl;
        }
        visionFbo.end();
    }
}

void ofApp::draw()
{
    if (showDepthMap) {
        ofSetColor(255);
        ofFill();
        depthTex.draw(drawBounds);
    } else {
        canvasFbo.draw(drawBounds);
    }
    
    visionFbo.draw(drawBounds);
    guiPanel.draw();
}
