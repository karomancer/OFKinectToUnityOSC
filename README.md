# OFKinectToUnityOSC
Seeing Machines Homework #4 for ITP: Taking sensor data from OpenFrameworks and using OpenCV to detect blobs then sending blog movement data to Unity via OSC.

Here's it all working together.

https://github.com/karomancer/OFKinectToUnityOSC/assets/482817/cf04b0b6-323c-49c0-b6fd-c5091ec94a20

### OpenFrameworks x Kinect
On the OF side
* Kinect depth data is drawn onto a frame buffer object (FBO) with less static and in a way that is more responsive to changing min and max depth
* This FBO is then fed into OpenCV's ContourFinder to identify blobs.
* If a blob doesn't exist yet (or was lost), save blob data as a Player
* Compare future readings against the player reference to determine whether the player is moving left, right, jumping, or crouching ("sliding"). 
* Send OSC messages out

You can see the OSC messages in the console at the bottom of this video:

https://github.com/karomancer/OFKinectToUnityOSC/assets/482817/08a2b291-6d39-4291-be94-66c5a002d59e

### Unity
I'm not great at Unity so I simply tweaked the [Endless Runner Sample Game from the Unity Asset Store](https://assetstore.unity.com/packages/templates/tutorials/endless-runner-sample-game-87901) to take in OSC messages as input.

Namely in [`CharacterInputController.cs`](https://github.com/Unity-Technologies/EndlessRunnerSampleGame/blob/master/Assets/Scripts/Characters/CharacterInputController.cs), initialized an OSCreceiver using [extOSC](https://github.com/Iam1337/extOSC) and created methods that call existing methods to move the characters

```cs
public void Init()
{
  ...

  oscReceiver = gameObject.AddComponent<OSCReceiver>();
  oscReceiver.LocalPort = 1337;    

  oscReceiver.Bind( "/player/move" , ChangeLaneOSC );
  oscReceiver.Bind("/player/jump", JumpOSC);
  oscReceiver.Bind("/player/slide", SlideOSC);
}

...

// The OSC message includes the direction exactly as the game expects
// Pass that through
void ChangeLaneOSC(OSCMessage message) {
    ChangeLane(message.Values[0].IntValue);
}

void JumpOSC(OSCMessage message) {
    Jump();
}

void SlideOSC(OSCMessage message) {
    Slide();
}

```

Then, in the [original file](https://github.com/Unity-Technologies/EndlessRunnerSampleGame/blob/master/Assets/Scripts/Characters/CharacterInputController.cs#L184-L200), they checked whether the tutorial windows were open before allowing the move to occur, but I  moved that check to within the `ChangeLane`, `Jump`, and `Slide` methods themselves.

I had to move it else the `GetKeyDown` would never be hit and the receiver binding doesn't make sense to do in `Update()`.

So it ends up looking like this:

```cs
protected void Update() {
  ...
  if (Input.GetKeyDown(KeyCode.LeftArrow)) // The check for TutorialMoveCheck(0) used to be here
  {
      ChangeLane(-1);
  }
  ...
}

public void ChangeLane(int direction)
{
  TutorialMoveCheck(0); // but I moved it to here
  ...
}
```

--

## V0.1
This project started fairly differently than it ended!

I originally wanted to do an overhead projection and have an overhead experience with colliding circles. You can see the OSC messages are more granular with movement and it simply just spoke to another OF project.

All these messages are still in the project as well and can be toggled on with the GUI checkbox in the top left. Can switch between the two types of messages!

https://github.com/karomancer/OFKinectToUnityOSC/assets/482817/075611cc-2610-47c6-8331-907d57da6158

Going from OF to OF, though fulfilling the assignment, seemed like a cop out so decided to do something a little more clever!

