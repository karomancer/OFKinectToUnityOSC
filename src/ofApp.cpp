// ofApp.cpp
#include "ofApp.h"

const int KINECT_DEPTH_WIDTH = 512;
const int KINECT_DEPTH_HEIGHT = 424;
const int BITMAP_STRING_PADDING = 10;

void ofApp::setup()
{
    int width = ofGetScreenWidth();
    int height = ofGetScreenHeight();
    ofSetWindowShape(width, height);
    
    // Set up canvas
    ofBackground(255);
    ofSetFrameRate(200);
    
    // Set up drawing bounds for mapping 512x424 to full screen later
    drawBounds.set(0, 0, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
    drawBounds.scaleTo(ofGetCurrentViewport(), OF_SCALEMODE_FILL);
    
    int halfWidth = ofGetWidth() / 2;
    int halfHeight = ofGetHeight() / 2;
    int halfKinectWidth = KINECT_DEPTH_WIDTH / 2;
    int halfKinectHeight = KINECT_DEPTH_HEIGHT / 2;
    drawBoundsTopLeft.set(0, 0, halfWidth, halfHeight);
    drawBoundsTopRight.set(0, 0, halfWidth, halfHeight);
    drawBoundsBottomLeft.set(0, 0, halfWidth, halfHeight);
    drawBoundsBottomRight.set(0, 0, halfWidth, halfHeight);
    
    drawBoundsTopLeft.scaleTo(ofRectangle(0, 0, halfWidth, halfHeight), OF_SCALEMODE_FILL);
    drawBoundsTopRight.scaleTo(ofRectangle(halfWidth, 0, halfWidth, halfHeight), OF_SCALEMODE_FILL); // I have no idea why -320 is neeeded
    drawBoundsBottomLeft.scaleTo(ofRectangle(0, halfHeight, halfWidth, halfHeight), OF_SCALEMODE_FILL);
    drawBoundsBottomRight.scaleTo(ofRectangle(halfWidth, halfHeight, halfWidth, halfHeight), OF_SCALEMODE_FILL);
    
    // Set up frame buffer
    ofFboSettings fboSettings;
    fboSettings.width = width;
    fboSettings.height = height;
    canvasFbo.allocate(fboSettings);
    
    // Set up GUI panel
    guiPanel.setup("BRUSH_STROKES", "settings.json");
    guiPanel.add(showDebugGrid.set("Show debug grid", false));
    guiPanel.add(millisToClear.set("ms before clear", 600, 0, 10000));
    
    // Kinect GUI options
    kinectGuiGroup.setup("Kinect");
    kinectGuiGroup.add(minDepth.set("Min depth", 0.5f, 0.5f, 8.f));
    kinectGuiGroup.add(maxDepth.set("Max depth", 5.f, 0.5f, 8.f));
    kinectGuiGroup.add(minIR.set("Min IR value", 0, 0, 255));
    kinectGuiGroup.add(maxIR.set("Max IR value", 255, 255, 255));
    kinectGuiGroup.add(anchorDepth.set("Base pixel size", 1, 1, 50));
    guiPanel.add(&kinectGuiGroup);
    
    // Contour finder GUI options
    contourFinderGuiGroup.setup("Contour Finder");
    contourFinderGuiGroup.add(showContours.set("Show contours", false));
    contourFinderGuiGroup.add(minContourArea.set("Min area", 0.01f, 0, 300.f));
    contourFinderGuiGroup.add(maxContourArea.set("Max area", 0.4f, 0, 300.f));
    contourFinderGuiGroup.add(persistence.set("Persistence", 15, 0, 1000));
    guiPanel.add(&contourFinderGuiGroup);
    
    guiPanel.loadFromFile("settings.json");
    
    // Set up Kinect
    ofxKinectV2::Settings kinectSettings;
    kinectSettings.enableRGB = false;
    kinectSettings.enableIR = true;
    kinectSettings.enableRGBRegistration = false;
    //    kinectSettings.config.MinDepth = minDepth;
    //    kinectSettings.config.MaxDepth = maxDepth;
    kinect.open(0, kinectSettings);
    kinect.irExposure = 0.1f; // this was totally not documented
    
    ellapsedMillisSinceClear = 0;
}

void ofApp::update()
{
    kinect.update();
    
    // Only load the data if there is a new frame to process.
    if (kinect.isFrameNew())
    {
        presentIds.clear();
        idsToDelete.clear();
        
        // Depth code
        depthPixels = kinect.getDepthPixels();
        depthTex.loadData(depthPixels);
        
        // Infrared luminosity code
        irPixels = kinect.getIRPixels();
        irTex.loadData(irPixels);
        
        // Update blobs, outlines, and canvas FBOs
        updateBlobs();
        updateOutlines();
        updateCanvas();
        pruneTrackingObjects();
    }
}

void ofApp::updateBlobs() {
    blobPixels.allocate(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, OF_IMAGE_GRAYSCALE);
    
    for (int y = 0; y < irPixels.getHeight(); y++) {
        for (int x = 0; x < irPixels.getWidth(); x++) {
            float ir = irPixels.getColor(x, y).r;
            blobPixels.setColor(x, y, ir > minIR ? ofColor::white :  ofColor::black);
        }
    }
    
    blobImage.setFromPixels(blobPixels);
}

void ofApp::pruneTrackingObjects() {
    // Map is from tracking object ID -> true (dumb, I know)
    // Go through that and see if anything needs to be deleted
    // Could this be more efficient? Yes!
    // It's 3AM and I don't want to think about it anymore lol
    for (auto it = nagiTrackingMap.begin(); it != nagiTrackingMap.end(); ++it) {
        idsToDelete.push_back(it->first);
    }
    
    for (int i = 0; i < idsToDelete.size(); i++) {
        nagiTrackingMap.erase(idsToDelete[i]);
    }
}

void ofApp::updateCanvas() {
    canvasFbo.begin();
    ofSetColor(ofColor::black);
    ofFill();
    float xMultiplier = (float) ofGetScreenWidth() / depthPixels.getWidth();
    float yMultiplier = (float) ofGetScreenHeight() / depthPixels.getHeight();
    
    int i = 0;
    for (auto it = nagiTrackingMap.begin(); it != nagiTrackingMap.end(); ++it) {
        int label = it->first;        
        ofSetColor(ofColor(i % 2 != 0 ? label : 0, 0, i % 2 == 0 ? label : 0));
        ofFill();
        cv::Rect boundingRect = it->second;
        
        // if not found
        if (prevNagiTrackingMap.find(label) == prevNagiTrackingMap.end()) {
            prevNagiTrackingMap[label] = boundingRect;
            ofDrawCircle(boundingRect.x, boundingRect.y, anchorDepth);
        } else {
            cv::Rect prevBoundingRect = prevNagiTrackingMap[label];
            int x = ofLerp(prevBoundingRect.x, boundingRect.x, 0.1);
            int y = ofLerp(prevBoundingRect.y, boundingRect.y, 0.1);
            int width = ofLerp(prevBoundingRect.width, boundingRect.width, 0.1);
            int height = ofLerp(prevBoundingRect.height, boundingRect.height, 0.1);
            
            
            ofVec2f p1(boundingRect.x, boundingRect.y);
            ofVec2f p2(prevBoundingRect.x, prevBoundingRect.y);
            float velocity = abs(p1.distance(p2));
            float radius = ofMap(velocity, 0, 200, anchorDepth, 1);
            float randomPaintSplatter = ofRandom(0, 5);
            
            prevNagiTrackingMap[label] = cv::Rect(x, y, width, height);
            
            ofDrawCircle(x * xMultiplier, y * yMultiplier, radius + randomPaintSplatter);
        }
        i++;
    }
    
    if (ofGetElapsedTimeMillis() - ellapsedMillisSinceClear < millisToClear) {
        ofEnableAlphaBlending();
        ofSetColor(255, 2);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    } else {
        ellapsedMillisSinceClear = ofGetElapsedTimeMillis();
        ofBackground(255);
    }
    
    canvasFbo.end();
}

void ofApp::updateOutlines() {
    visionFbo.allocate(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
    
    // Using OpenCV's contour finder to find the different bounding boxes
    contourFinder.setMinAreaRadius(minContourArea);
    contourFinder.setMaxAreaRadius(maxContourArea);
    contourFinder.getTracker().setPersistence(persistence);
    contourFinder.findContours(showDepthMap ? depthPixels : irPixels);
    std::vector<cv::Rect> blobs = contourFinder.getBoundingRects();
    std::vector<ofPolyline> polylines = contourFinder.getPolylines();
    
    // Draw the contour in its own FBO to render later
    visionFbo.begin();
    ofClear(255);
    ofSetColor(ofColor::white);
    ofNoFill();
    for(int i = 0; i < blobs.size(); i++) {
        int label = contourFinder.getLabel(i);
        ofColor color = ofColor::red;
        cv::Rect boundingRect = blobs[i];
        nagiTrackingMap[label] = boundingRect;
        presentIds.push_back(label);
        
        color.setHueAngle(color.getHueAngle() + label * 5);
        ofSetColor(color);
        
        ofDrawRectangle(boundingRect.x, boundingRect.y, boundingRect.width, boundingRect.height);
    }
    
    visionFbo.end();
}

void ofApp::draw()
{
    ofBackground(ofColor::white);
    if (showDebugGrid) {
        int width = ofGetWidth();
        int height = ofGetHeight();
        
        depthTex.draw(drawBoundsTopLeft);
        irTex.draw(drawBoundsTopRight);
        blobImage.draw(drawBoundsBottomLeft);
        visionFbo.draw(drawBoundsBottomLeft);
        canvasFbo.draw(drawBoundsBottomRight);
        
        ofDrawBitmapStringHighlight("Kinect Depth Camera", BITMAP_STRING_PADDING, height/2 + BITMAP_STRING_PADDING);
        ofDrawBitmapStringHighlight("Kinect IR Camera", width/2 + BITMAP_STRING_PADDING, height/2 + BITMAP_STRING_PADDING);
        ofDrawBitmapStringHighlight("Masked IR + CV Contours", BITMAP_STRING_PADDING, height - BITMAP_STRING_PADDING);
        ofDrawBitmapStringHighlight("Output", width/2 + BITMAP_STRING_PADDING, height - BITMAP_STRING_PADDING);
        
        // Get the point distance using the SDK function (in meters).
        float mouseX = ofGetMouseX();
        float mouseY = ofGetMouseY();
        float mappedY = ofMap(mouseY, 0, ofGetHeight()/2, 0, KINECT_DEPTH_HEIGHT);
        // If cursor is in depth map
        if (drawBoundsTopLeft.inside(mouseX, mouseY)) {
            float mappedX = ofMap(mouseX, 0, ofGetWidth()/2, 0, KINECT_DEPTH_WIDTH);
            float distAtMouse = kinect.getDistanceAt(mappedX, mappedY);
            ofDrawBitmapStringHighlight("Depth: " + ofToString(distAtMouse, 3), mouseX, mouseY  + BITMAP_STRING_PADDING);
        }
        // If cursor is in IR map
        else if (drawBoundsTopRight.inside(mouseX, mouseY)) {
            float mappedX = ofMap(mouseX, ofGetWidth()/2, ofGetWidth(), 0, KINECT_DEPTH_WIDTH);
            float ir = irPixels.getColor(mappedX, mappedY).r;
            ofDrawBitmapStringHighlight("IR: " + ofToString(ir, 3), mouseX, mouseY  + BITMAP_STRING_PADDING);
        }
    } else {
        canvasFbo.draw(0, 0);
        
        if (showContours) {
            visionFbo.draw(drawBounds);
        }
    }
    
    guiPanel.draw();
}
