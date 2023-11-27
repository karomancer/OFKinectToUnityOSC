// ofApp.cpp
#include "ofApp.h"
#include "ofxCv.h"

const int KINECT_DEPTH_WIDTH = 512;
const int KINECT_DEPTH_HEIGHT = 424;

void ofApp::setup()
{
    ofSetWindowShape(ofGetScreenWidth(), ofGetScreenHeight());
    
    // Set up canvas
    ofBackground(255);
    
    // Set up drawing bounds for mapping 512x424 to full screen later
    drawBounds.set(0, 0, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
    drawBounds.scaleTo(ofGetCurrentViewport(), OF_SCALEMODE_FILL);
    
    // Set up OSC sender
    oscSender.setup("localhost", 1337);
    
    guiPanel.setup("SETTINGS", "settings.json");
    guiPanel.add(sendObjectMsgs.set("Send /object/ msgs", true));
    guiPanel.add(sendPlayerMsgs.set("Send /player/ msgs", true));
    
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
    contourFinderGuiGroup.add(persistence.set("Persistence", 15, 0, 1000));
    
    player = {0, -1, -1};
    playerMovementGuiGroup.setup("Player Movement");
    playerMovementGuiGroup.add(moveThreshold.set("Move threshold", 10, 0, 100));
    playerMovementGuiGroup.add(jumpThreshold.set("Jump threshold", 10, 0, 300));
    playerMovementGuiGroup.add(slideThreshold.set("Slide threshold", 10, 0, 300));
    
    // Set up GUI panel groups
    guiPanel.add(&kinectGuiGroup);
    guiPanel.add(&contourFinderGuiGroup);
    guiPanel.add(&playerMovementGuiGroup);
    guiPanel.loadFromFile("settings.json");
    
    // Set up Kinect
    ofxKinectV2::Settings kinectSettings;
    kinectSettings.enableRGB = false;
    kinectSettings.enableIR = false;
    kinectSettings.enableRGBRegistration = false;
    kinect.open(0, kinectSettings);
}

void ofApp::updateCanvas() {
    // Allocate space for frame buffers
    // We're going to draw in the frame buffers off screen first
    // then draw to the screen
    canvasFbo.allocate(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
    
    // Drawing the Kinect depth data into a gray scale figure on white
    // using all the parameters set in the GUI
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
    
    // Reading the frame buffer object (FBO) to pixels for
    // the contour finder to compare against
    canvasFbo.readToPixels(canvasPixels);
}

void ofApp::updateOutlines() {
    visionFbo.allocate(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT);
    
    // Using OpenCV's contour finder to find the different bounding boxes
    contourFinder.setMinAreaRadius(minContourArea);
    contourFinder.setMaxAreaRadius(maxContourArea);
    contourFinder.getTracker().setPersistence(persistence);
    contourFinder.findContours(showDepthMap ? depthPixels : canvasPixels);
    std::vector<cv::Rect> blobs = contourFinder.getBoundingRects();
    
    // Draw the contour in its own FBO to render later
    visionFbo.begin();
    ofClear(255);
    ofSetColor(ofColor::white);
    ofNoFill();
    for(int i = 0; i < blobs.size(); i++) {
        int label = contourFinder.getLabel(i);
        ofColor color = ofColor::red;
        cv::Rect boundingRect = blobs[i];
        
        color.setHueAngle(color.getHueAngle() + label * 5);
        ofSetColor(color);
        
        ofDrawRectangle(boundingRect.x, boundingRect.y, boundingRect.width, boundingRect.height);
        
        idTrackingMap[label] = boundingRect;
        if (sendObjectMsgs) {
            sendObjectMoveMsg(label, boundingRect, color);
        }
        presentIds.push_back(label);
    }
    visionFbo.end();
}

void ofApp::update()
{
    kinect.update();
    
    // Only load the data if there is a new frame to process.
    if (kinect.isFrameNew())
    {
        presentIds.clear();
        idsToDelete.clear();
        
        depthPixels = kinect.getDepthPixels();
        depthTex.loadData(depthPixels);
        
        updateCanvas();
        updateOutlines();
        
        // 0th is player
        if (sendPlayerMsgs && contourFinder.getBoundingRects().size() > 0) {
            cv::Rect blobRect;
            
            auto it = std::find(presentIds.begin(), presentIds.end(), player.label);
            if (it != presentIds.end()) {
                cv::Rect blobRect = idTrackingMap[player.label];
                int x = (blobRect.x + blobRect.width) / 2;
                int y = (blobRect.x + blobRect.width) / 2;
                
                if (x < player.x - moveThreshold) {
                    cout << "MOVE LEFT" << endl;
                    // moveLeft
                } else if (x > player.x + moveThreshold) {
                    cout << "MOVE RIGHT" << endl;
                    // moveRight
                } else if (y < player.y - slideThreshold) {
                    cout << "SLIDE!" << endl;
                    // Jump
                } else if (y > player.y + jumpThreshold) {
                    cout << "JUMP!" << endl;
                    // Slide
                } else {
                    cout << "--" << endl;
                }
                
                player.x = x;
                player.y = y;
            } else {
                blobRect = contourFinder.getBoundingRect(0);
                player.label = contourFinder.getLabel(0);
                
                player.x = (blobRect.x + blobRect.width) / 2;
                player.y = (blobRect.y + blobRect.height) / 2;
                
                cout << "Setting player to " << player.label << endl;
            }
            
            
        }
        
        
        if (sendObjectMsgs) {
            pruneTrackingObjects();
        }
    }
}

void ofApp::draw()
{
    // If the showDepthMap checkbox is checked, show that instead of
    // the drawn version I made
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

void ofApp::pruneTrackingObjects() {
    // Map is from tracking object ID -> true (dumb, I know)
    // Go through that and see if anything needs to be deleted
    // Could this be more efficient? Yes!
    // It's 3AM and I don't want to think about it anymore lol
    for (auto it = idTrackingMap.begin(); it != idTrackingMap.end(); ++it) {
        int label = it->first;
        auto findIt = std::find(std::begin(presentIds), std::end(presentIds), label);
        
        if (findIt == std::end(presentIds)) {
            sendObjectDeleteMsg(label);
        }
        
        idsToDelete.push_back(label);
    }
    
    for (int i = 0; i < idsToDelete.size(); i++) {
        idTrackingMap.erase(idsToDelete[i]);
    }
}

/**
 * Custom methods for sending messages
 */
void ofApp::sendObjectMoveMsg(int label, cv::Rect boundingRect, ofColor color) {
    if (idTrackingMap.find(label) == idTrackingMap.end()) {
        // Don't mind me...just sending messages in a drawing function shhhh
        ofxOscMessage msg;
        msg.setAddress("/object/move");
        msg.addIntArg(label);
        msg.addIntArg(boundingRect.x);
        msg.addIntArg(boundingRect.y);
        msg.addIntArg(color.getHex());
        oscSender.sendMessage(msg);
        
        cout << "Sending /object/move " << label << " " << boundingRect.x << " " << boundingRect.y << " " << color.getHex() << endl;
    }
}

void ofApp::sendObjectDeleteMsg(int label) {
    ofxOscMessage msg;
    msg.setAddress("/object/delete");
    msg.addIntArg(label);
    oscSender.sendMessage(msg);
    
    cout << "Sending /object/delete " << label << endl;
}
